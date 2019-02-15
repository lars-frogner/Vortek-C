#include "renderer.h"

#include "gl_includes.h"
#include "transform.h"
#include "utils.h"
#include "io.h"
#include "extra_math.h"

#include <stdlib.h>
#include <math.h>
#include <time.h>


typedef struct WindowShape
{
    int width;
    int height;
} WindowShape;

typedef struct PlaneCorners
{
    Vector4f xyzw[4];
} PlaneCorners;

typedef struct PlaneTextureCoords
{
    Vector3f uvw[4];
} PlaneTextureCoords;

typedef struct PlaneFaces
{
    GLuint indices[6];
} PlaneFaces;

typedef struct AxisPlaneSet
{
    PlaneCorners* corners;
    PlaneTextureCoords* texture_coords;
    PlaneFaces* faces_low_to_high;
    PlaneFaces* faces_high_to_low;
    unsigned int n_planes;
} AxisPlaneSet;

typedef struct PlaneSet
{
    AxisPlaneSet x;
    AxisPlaneSet y;
    AxisPlaneSet z;
    unsigned int n_planes_total;
    void* vertex_buffer;
    void* face_buffer;
    unsigned int face_set_size_lookup[3];
    unsigned int face_set_offset_lookup[3][2];
    unsigned int active_axis;
    unsigned int active_order;
    GLuint vertex_array_object_id;
    GLuint vertex_buffer_id;
    GLuint face_buffer_id;
} PlaneSet;

typedef struct ShaderProgram
{
    GLuint id;
    GLuint vertex_shader_id;
    GLuint fragment_shader_id;
} ShaderProgram;

typedef struct Transformation
{
    Matrix4f matrix;
    GLuint uniform_location;
} Transformation;


static void create_planes(unsigned int data_size_x, unsigned int data_size_y, unsigned int data_size_z, float spacing);
static void draw_planes();
static void destroy_planes();

static void update_active_plane_set_dimension_and_order();

static void initialize_rendering_settings();
static void initialize_transform_matrices();

static void create_shader_program();
static void destroy_shader_program();

static void set_matrix_uniform_locations();

static void update_device_model_matrix();
static void update_device_view_matrix();
static void update_device_projection_matrix();

static void update_perspective_projection_matrix();


static WindowShape window_shape;

static PlaneSet plane_set = {{NULL, NULL, NULL, NULL, 0},
                             {NULL, NULL, NULL, NULL, 0},
                             {NULL, NULL, NULL, NULL, 0},
                             0,
                             NULL, NULL,
                             {0}, {0},
                             0, 0,
                             0, 0, 0};

static ShaderProgram shader_program = {0};

static Transformation model_transform;
static Transformation view_transform;
static Transformation projection_transform;


void initialize_renderer()
{
    print_info("OpenGL Version: %s", glGetString(GL_VERSION));

    glGetError();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    initialize_rendering_settings();

    create_planes(128, 128, 128, 6.0f);

    create_shader_program();

    initialize_transform_matrices();

    update_active_plane_set_dimension_and_order();
}

void cleanup_renderer()
{
    destroy_shader_program();
    destroy_planes();
}

void update_renderer_window_size_in_pixels(int width, int height)
{
    window_shape.width = width;
    window_shape.height = height;
    glViewport(0, 0, width, height);
}

void renderer_update_callback()
{
    glClear(GL_COLOR_BUFFER_BIT);
    draw_planes();
}

void renderer_resize_callback(int width, int height)
{
    update_renderer_window_size_in_pixels(width, height);
    update_perspective_projection_matrix();
}

void renderer_key_action_callback() {}

void apply_model_rotation_about_axis(const Vector3f* axis, float angle)
{
    apply_rotation_about_axis(&model_transform.matrix, axis, angle);
    update_device_model_matrix();
    update_active_plane_set_dimension_and_order();
}

void apply_model_scaling(float scale)
{
    apply_scaling(&model_transform.matrix, scale, scale, scale);
    update_device_model_matrix();
}

static void initialize_rendering_settings()
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    abort_on_GL_error("Could not set face culling options");
}

static void initialize_transform_matrices()
{
    set_matrix_uniform_locations();

    model_transform.matrix = IDENTITY_MATRIX4F;
    update_device_model_matrix();

    const float camera_distance = 2.0f;
    view_transform.matrix = IDENTITY_MATRIX4F;
    apply_translation(&view_transform.matrix, 0, 0, -camera_distance);
    update_device_view_matrix();

    update_perspective_projection_matrix();
}

static void create_planes(unsigned int data_size_x, unsigned int data_size_y, unsigned int data_size_z, float spacing)
{
    if (plane_set.vertex_buffer || plane_set.face_buffer)
    {
        print_error("Can not create new planes before old ones are destroyed.");
        exit(EXIT_FAILURE);
    }

    const unsigned int max_size = umax(data_size_x, umax(data_size_y, data_size_z));
    const float scale = 1.0f/max_size;
    const float halfwidth  = scale*data_size_x;
    const float halfheight = scale*data_size_y;
    const float halfdepth  = scale*data_size_z;

    plane_set.x.n_planes = (unsigned int)(data_size_x/spacing);
    plane_set.y.n_planes = (unsigned int)(data_size_y/spacing);
    plane_set.z.n_planes = (unsigned int)(data_size_z/spacing);
    plane_set.n_planes_total = plane_set.x.n_planes + plane_set.y.n_planes + plane_set.z.n_planes;

    if (plane_set.x.n_planes < 2 || plane_set.y.n_planes < 2 || plane_set.z.n_planes < 2)
    {
        print_error("Can not create fewer than two planes along any axis.");
        exit(EXIT_FAILURE);
    }

    const float dx = 2*halfwidth/(plane_set.x.n_planes - 1);
    const float dy = 2*halfheight/(plane_set.y.n_planes - 1);
    const float dz = 2*halfdepth/(plane_set.z.n_planes - 1);

    const float du = 1.0f/(plane_set.x.n_planes - 1);
    const float dv = 1.0f/(plane_set.y.n_planes - 1);
    const float dw = 1.0f/(plane_set.z.n_planes - 1);

    const size_t vertex_array_bytes = plane_set.n_planes_total*(sizeof(PlaneCorners) + sizeof(PlaneTextureCoords));

    // Allocate contiguous memory block for all vertices, and store pointers to relevant segments
    plane_set.vertex_buffer = malloc(vertex_array_bytes);

    PlaneCorners* const all_corners =  (PlaneCorners*)plane_set.vertex_buffer;
    PlaneTextureCoords* const all_texture_coords = (PlaneTextureCoords*)(all_corners + plane_set.n_planes_total);

    plane_set.x.corners = all_corners;
    plane_set.y.corners = plane_set.x.corners + plane_set.x.n_planes;
    plane_set.z.corners = plane_set.y.corners + plane_set.y.n_planes;

    plane_set.x.texture_coords = all_texture_coords;
    plane_set.y.texture_coords = plane_set.x.texture_coords + plane_set.x.n_planes;
    plane_set.z.texture_coords = plane_set.y.texture_coords + plane_set.y.n_planes;

    const size_t face_array_bytes = 2*plane_set.n_planes_total*sizeof(PlaneFaces);

    // Do the same for face indices
    plane_set.face_buffer = malloc(face_array_bytes);
    plane_set.x.faces_low_to_high = (PlaneFaces*)plane_set.face_buffer;
    plane_set.x.faces_high_to_low = plane_set.x.faces_low_to_high + plane_set.x.n_planes;
    plane_set.y.faces_low_to_high = plane_set.x.faces_high_to_low + plane_set.x.n_planes;
    plane_set.y.faces_high_to_low = plane_set.y.faces_low_to_high + plane_set.y.n_planes;
    plane_set.z.faces_low_to_high = plane_set.y.faces_high_to_low + plane_set.y.n_planes;
    plane_set.z.faces_high_to_low = plane_set.z.faces_low_to_high + plane_set.z.n_planes;

    // Store total number of integers specifying the faces of a plane set for each dimension
    plane_set.face_set_size_lookup[0] = plane_set.x.n_planes*6;
    plane_set.face_set_size_lookup[1] = plane_set.y.n_planes*6;
    plane_set.face_set_size_lookup[2] = plane_set.z.n_planes*6;

    // Store byte offsets to each set of faces. First index is dimension and second is whether
    // the order is from low to high (0) or high to low (1).
    plane_set.face_set_offset_lookup[0][0] = 0;
    plane_set.face_set_offset_lookup[0][1] = plane_set.face_set_offset_lookup[0][0] + plane_set.x.n_planes*sizeof(PlaneFaces);
    plane_set.face_set_offset_lookup[1][0] = plane_set.face_set_offset_lookup[0][1] + plane_set.x.n_planes*sizeof(PlaneFaces);
    plane_set.face_set_offset_lookup[1][1] = plane_set.face_set_offset_lookup[1][0] + plane_set.y.n_planes*sizeof(PlaneFaces);
    plane_set.face_set_offset_lookup[2][0] = plane_set.face_set_offset_lookup[1][1] + plane_set.y.n_planes*sizeof(PlaneFaces);
    plane_set.face_set_offset_lookup[2][1] = plane_set.face_set_offset_lookup[2][0] + plane_set.z.n_planes*sizeof(PlaneFaces);

    if (!plane_set.vertex_buffer || !plane_set.face_buffer)
    {
        print_error("Could not allocate memory for planes.");
        exit(EXIT_FAILURE);
    }

    int i;
    float position_offset;
    float texture_offset;
    int low_to_high_index_offset;
    int high_to_low_index_offset;

    for (i = 0; i < plane_set.x.n_planes; i++)
    {
        // Specify the four corners of the i'th x-plane (lying in the yz-plane)
        position_offset = i*dx - halfwidth;
        set_vector4f_elements(plane_set.x.corners[i].xyzw,     position_offset, -halfheight, -halfdepth, 1.0f); // 3-----2 z
        set_vector4f_elements(plane_set.x.corners[i].xyzw + 1, position_offset,  halfheight, -halfdepth, 1.0f); // |     | ^
        set_vector4f_elements(plane_set.x.corners[i].xyzw + 2, position_offset,  halfheight,  halfdepth, 1.0f); // |     |
        set_vector4f_elements(plane_set.x.corners[i].xyzw + 3, position_offset, -halfheight,  halfdepth, 1.0f); // 0-----1 >y

        texture_offset = i*du;
        set_vector3f_elements(plane_set.x.texture_coords[i].uvw,     texture_offset, 0.0f, 0.0f);
        set_vector3f_elements(plane_set.x.texture_coords[i].uvw + 1, texture_offset, 1.0f, 0.0f);
        set_vector3f_elements(plane_set.x.texture_coords[i].uvw + 2, texture_offset, 1.0f, 1.0f);
        set_vector3f_elements(plane_set.x.texture_coords[i].uvw + 3, texture_offset, 0.0f, 1.0f);

        // Specify the two face triangles making up the i'th x-plane.
        // These will be used when looking along the negative x-direction (drawing back to front).
        // Each entry is an index into the full vertex array, and three consecutive entries define
        // a triangle.
        low_to_high_index_offset = 4*i;
        plane_set.x.faces_low_to_high[i].indices[0] = low_to_high_index_offset;     //     2
        plane_set.x.faces_low_to_high[i].indices[1] = low_to_high_index_offset + 1; //     |
        plane_set.x.faces_low_to_high[i].indices[2] = low_to_high_index_offset + 2; // 0---1

        plane_set.x.faces_low_to_high[i].indices[3] = low_to_high_index_offset + 2; // 3---2
        plane_set.x.faces_low_to_high[i].indices[4] = low_to_high_index_offset + 3; // |
        plane_set.x.faces_low_to_high[i].indices[5] = low_to_high_index_offset;     // 0

        // Specify the corresponding triangles for the i'th x-plane counting from the opposite end.
        // These will be used when looking along the positive x-direction (drawing back to front).
        // They must have to opposite orientation to get the visible side towards the viewer.
        high_to_low_index_offset = 4*(plane_set.x.n_planes - 1 - i);
        plane_set.x.faces_high_to_low[i].indices[0] = high_to_low_index_offset + 2; //     2
        plane_set.x.faces_high_to_low[i].indices[1] = high_to_low_index_offset + 1; //     |
        plane_set.x.faces_high_to_low[i].indices[2] = high_to_low_index_offset;     // 0---1

        plane_set.x.faces_high_to_low[i].indices[3] = high_to_low_index_offset;     // 3---2
        plane_set.x.faces_high_to_low[i].indices[4] = high_to_low_index_offset + 3; // |
        plane_set.x.faces_high_to_low[i].indices[5] = high_to_low_index_offset + 2; // 0
    }

    for (i = 0; i < plane_set.y.n_planes; i++)
    {
        // Specify the four corners of the i'th y-plane (lying in the xz-plane)
        position_offset = i*dy - halfheight;
        set_vector4f_elements(plane_set.y.corners[i].xyzw,     -halfwidth, position_offset, -halfdepth, 1.0f); // 3-----2 z
        set_vector4f_elements(plane_set.y.corners[i].xyzw + 1, -halfwidth, position_offset,  halfdepth, 1.0f); // |     | ^
        set_vector4f_elements(plane_set.y.corners[i].xyzw + 2,  halfwidth, position_offset,  halfdepth, 1.0f); // |     |
        set_vector4f_elements(plane_set.y.corners[i].xyzw + 3,  halfwidth, position_offset, -halfdepth, 1.0f); // 0-----1 >x

        texture_offset = i*dv;
        set_vector3f_elements(plane_set.y.texture_coords[i].uvw,     0.0f, texture_offset, 0.0f);
        set_vector3f_elements(plane_set.y.texture_coords[i].uvw + 1, 0.0f, texture_offset, 1.0f);
        set_vector3f_elements(plane_set.y.texture_coords[i].uvw + 2, 1.0f, texture_offset, 1.0f);
        set_vector3f_elements(plane_set.y.texture_coords[i].uvw + 3, 1.0f, texture_offset, 0.0f);

        // Specify the two face triangles making up the i'th y-plane
        low_to_high_index_offset = 4*(plane_set.x.n_planes + i);
        plane_set.y.faces_low_to_high[i].indices[0] = low_to_high_index_offset;     //     2
        plane_set.y.faces_low_to_high[i].indices[1] = low_to_high_index_offset + 1; //     |
        plane_set.y.faces_low_to_high[i].indices[2] = low_to_high_index_offset + 2; // 0---1

        plane_set.y.faces_low_to_high[i].indices[3] = low_to_high_index_offset + 2; // 3---2
        plane_set.y.faces_low_to_high[i].indices[4] = low_to_high_index_offset + 3; // |
        plane_set.y.faces_low_to_high[i].indices[5] = low_to_high_index_offset;     // 0

        // Specify the corresponding triangles for the i'th y-plane counting from the opposite end
        high_to_low_index_offset = 4*(plane_set.x.n_planes + plane_set.y.n_planes - 1 - i);
        plane_set.y.faces_high_to_low[i].indices[0] = high_to_low_index_offset + 2; //     2
        plane_set.y.faces_high_to_low[i].indices[1] = high_to_low_index_offset + 1; //     |
        plane_set.y.faces_high_to_low[i].indices[2] = high_to_low_index_offset;     // 0---1

        plane_set.y.faces_high_to_low[i].indices[3] = high_to_low_index_offset ;    // 3---2
        plane_set.y.faces_high_to_low[i].indices[4] = high_to_low_index_offset + 3; // |
        plane_set.y.faces_high_to_low[i].indices[5] = high_to_low_index_offset + 2; // 0
    }

    for (i = 0; i < plane_set.z.n_planes; i++)
    {
        // Specify the four corners of the i'th z-plane (lying in the yz-plane)
        position_offset = i*dz - halfdepth;
        set_vector4f_elements(plane_set.z.corners[i].xyzw,     -halfwidth, -halfheight, position_offset, 1.0f); // 3-----2 y
        set_vector4f_elements(plane_set.z.corners[i].xyzw + 1,  halfwidth, -halfheight, position_offset, 1.0f); // |     | ^
        set_vector4f_elements(plane_set.z.corners[i].xyzw + 2,  halfwidth,  halfheight, position_offset, 1.0f); // |     |
        set_vector4f_elements(plane_set.z.corners[i].xyzw + 3, -halfwidth,  halfheight, position_offset, 1.0f); // 0-----1 >x

        texture_offset = i*dw;
        set_vector3f_elements(plane_set.z.texture_coords[i].uvw,     0.0f, 0.0f, texture_offset);
        set_vector3f_elements(plane_set.z.texture_coords[i].uvw + 1, 1.0f, 0.0f, texture_offset);
        set_vector3f_elements(plane_set.z.texture_coords[i].uvw + 2, 1.0f, 1.0f, texture_offset);
        set_vector3f_elements(plane_set.z.texture_coords[i].uvw + 3, 0.0f, 1.0f, texture_offset);

        // Specify the two face triangles making up the i'th z-plane
        low_to_high_index_offset = 4*(plane_set.x.n_planes + plane_set.y.n_planes + i);
        plane_set.z.faces_low_to_high[i].indices[0] = low_to_high_index_offset;     //     2
        plane_set.z.faces_low_to_high[i].indices[1] = low_to_high_index_offset + 1; //     |
        plane_set.z.faces_low_to_high[i].indices[2] = low_to_high_index_offset + 2; // 0---1

        plane_set.z.faces_low_to_high[i].indices[3] = low_to_high_index_offset + 2; // 3---2
        plane_set.z.faces_low_to_high[i].indices[4] = low_to_high_index_offset + 3; // |
        plane_set.z.faces_low_to_high[i].indices[5] = low_to_high_index_offset;     // 0

        // Specify the corresponding triangles for the i'th z-plane counting from the opposite end
        high_to_low_index_offset = 4*(plane_set.x.n_planes + plane_set.y.n_planes + plane_set.z.n_planes - 1 - i);
        plane_set.z.faces_high_to_low[i].indices[0] = high_to_low_index_offset + 2; //     2
        plane_set.z.faces_high_to_low[i].indices[1] = high_to_low_index_offset + 1; //     |
        plane_set.z.faces_high_to_low[i].indices[2] = high_to_low_index_offset;     // 0---1

        plane_set.z.faces_high_to_low[i].indices[3] = high_to_low_index_offset;     // 3---2
        plane_set.z.faces_high_to_low[i].indices[4] = high_to_low_index_offset + 3; // |
        plane_set.z.faces_high_to_low[i].indices[5] = high_to_low_index_offset + 2; // 0
    }

    // Generate vertex array object for keeping track of vertex attributes
    glGenVertexArrays(1, &plane_set.vertex_array_object_id);
    abort_on_GL_error("Could not generate VAO");
    glBindVertexArray(plane_set.vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO");

    // Generate buffer object for vertices
    glGenBuffers(1, &plane_set.vertex_buffer_id);
    abort_on_GL_error("Could not generate vertex buffer object");

    // Store buffer of all vertices on device
    glBindBuffer(GL_ARRAY_BUFFER, plane_set.vertex_buffer_id);
    glBufferData(GL_ARRAY_BUFFER, vertex_array_bytes, plane_set.vertex_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not bind VBO to VAO");

    // Specify vertex attribute pointer for corners
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    abort_on_GL_error("Could not set VAO corner attributes");

    // Specify vertex attribute pointer for texture coordinates
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(plane_set.n_planes_total*sizeof(PlaneCorners)));
    glEnableVertexAttribArray(1);
    abort_on_GL_error("Could not set VAO texture coordinate attributes");

    // Generate buffer object for face indices
    glGenBuffers(1, &plane_set.face_buffer_id);
    abort_on_GL_error("Could not generate face buffer object");

    // Store buffer of all face indices on device
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_set.face_buffer_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, face_array_bytes, plane_set.face_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not bind IBO to VAO");

    glBindVertexArray(0);
}

static void draw_planes()
{
    glUseProgram(shader_program.id);
    abort_on_GL_error("Could not use shader program");

    glBindVertexArray(plane_set.vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing");

    glDrawElements(GL_TRIANGLES,
                   plane_set.face_set_size_lookup[plane_set.active_axis],
                   GL_UNSIGNED_INT,
                   (GLvoid*)plane_set.face_set_offset_lookup[plane_set.active_axis][plane_set.active_order]);
    abort_on_GL_error("Could not draw planes");

    glBindVertexArray(0);
    glUseProgram(0);
}

static void destroy_planes()
{
    if (!plane_set.vertex_buffer || !plane_set.face_buffer)
    {
        print_error("Can not destroy planes before they are created.");
        exit(EXIT_FAILURE);
    }

    glDeleteBuffers(1, &plane_set.face_buffer_id);
    glDeleteBuffers(1, &plane_set.vertex_buffer_id);
    glDeleteVertexArrays(1, &plane_set.vertex_array_object_id);
    abort_on_GL_error("Could not destroy buffer objects");

    free(plane_set.vertex_buffer);
    free(plane_set.face_buffer);

    plane_set.x.corners = NULL;
    plane_set.x.texture_coords = NULL;
    plane_set.x.faces_low_to_high = NULL;
    plane_set.x.faces_high_to_low = NULL;
    plane_set.x.n_planes = 0;

    plane_set.y.corners = NULL;
    plane_set.y.texture_coords = NULL;
    plane_set.y.faces_low_to_high = NULL;
    plane_set.y.faces_high_to_low = NULL;
    plane_set.y.n_planes = 0;

    plane_set.z.corners = NULL;
    plane_set.z.texture_coords = NULL;
    plane_set.z.faces_low_to_high = NULL;
    plane_set.z.faces_high_to_low = NULL;
    plane_set.z.n_planes = 0;

    plane_set.n_planes_total = 0;
}

static void update_active_plane_set_dimension_and_order()
{
    // Basis vectors of view matrix give orientation of the camera x- y- and z-axes
    Vector3f view_right_axis, view_up_axis, view_look_axis;
    get_matrix4f_x_y_z_basis_vectors(&view_transform.matrix, &view_right_axis, &view_up_axis, &view_look_axis);
    normalize_vector3f(&view_look_axis);

    // Basis vectors of model matrix give orientation of the model frame x- y- and z-axes
    Vector3f model_x_axis, model_y_axis, model_z_axis;
    get_matrix4f_x_y_z_basis_vectors(&model_transform.matrix, &model_x_axis, &model_y_axis, &model_z_axis);
    normalize_vector3f(&model_x_axis);
    normalize_vector3f(&model_y_axis);
    normalize_vector3f(&model_z_axis);

    // Compute the components of the model axes along the camera look axis
    float look_components[3] = {dot3f(&model_x_axis, &view_look_axis), dot3f(&model_y_axis, &view_look_axis), dot3f(&model_z_axis, &view_look_axis)};

    // The plane set to draw is determined by the maximum absolute component
    plane_set.active_axis = argfmax3(fabs(look_components[0]), fabs(look_components[1]), fabs(look_components[2]));

    // The order to draw the planes is determined by the sign of the
    plane_set.active_order = (look_components[plane_set.active_axis] >= 0) ? 0 : 1;
}

static void create_shader_program()
{
    shader_program.id = glCreateProgram();
    abort_on_GL_error("Could not create shader program");

    shader_program.vertex_shader_id = load_shader_from_file("shaders/simple.vertex.glsl", GL_VERTEX_SHADER);
    shader_program.fragment_shader_id = load_shader_from_file("shaders/simple.fragment.glsl", GL_FRAGMENT_SHADER);
    glAttachShader(shader_program.id, shader_program.vertex_shader_id);
    glAttachShader(shader_program.id, shader_program.fragment_shader_id);

    glLinkProgram(shader_program.id);
    abort_on_GL_error("Could not link shader program");
}

static void destroy_shader_program()
{
    glDetachShader(shader_program.id, shader_program.vertex_shader_id);
    glDetachShader(shader_program.id, shader_program.fragment_shader_id);
    glDeleteShader(shader_program.vertex_shader_id);
    glDeleteShader(shader_program.fragment_shader_id);
    glDeleteProgram(shader_program.id);
    abort_on_GL_error("Could not destroy shader program");
}

static void set_matrix_uniform_locations()
{
    model_transform.uniform_location = glGetUniformLocation(shader_program.id, "model_matrix");
    view_transform.uniform_location = glGetUniformLocation(shader_program.id, "view_matrix");
    projection_transform.uniform_location = glGetUniformLocation(shader_program.id, "projection_matrix");
    abort_on_GL_error("Could not get transform matrix uniform locations");
}

static void update_device_model_matrix()
{
    glUseProgram(shader_program.id);
    abort_on_GL_error("Could not use shader program");
    glUniformMatrix4fv(model_transform.uniform_location, 1, GL_TRUE, model_transform.matrix.a);
    glUseProgram(0);
}

static void update_device_view_matrix()
{
    glUseProgram(shader_program.id);
    abort_on_GL_error("Could not use shader program");
    glUniformMatrix4fv(view_transform.uniform_location, 1, GL_TRUE, view_transform.matrix.a);
    glUseProgram(0);
}

static void update_device_projection_matrix()
{
    glUseProgram(shader_program.id);
    abort_on_GL_error("Could not use shader program");
    glUniformMatrix4fv(projection_transform.uniform_location, 1, GL_TRUE, projection_transform.matrix.a);
    glUseProgram(0);
}

static void update_perspective_projection_matrix()
{
    const float field_of_view_y = 60.0f; // [degrees]
    const float near_plane = 0.001f;
    const float far_plane = 100.0f;
    const float aspect = (float)window_shape.width/window_shape.height;

    projection_transform.matrix = create_perspective_transform(field_of_view_y, aspect, near_plane, far_plane);
    update_device_projection_matrix();
}
