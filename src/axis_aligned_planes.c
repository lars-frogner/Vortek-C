#include "axis_aligned_planes.h"

#include "gl_includes.h"
#include "error.h"
#include "extra_math.h"
#include "geometry.h"
#include "transformation.h"

#include <stdlib.h>
#include <math.h>


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
    size_t vertex_buffer_size;
    size_t face_buffer_size;
    unsigned int face_set_size_lookup[3];
    unsigned int face_set_offset_lookup[3][2];
    unsigned int active_axis;
    unsigned int active_order;
    GLuint vertex_array_object_id;
    GLuint vertex_buffer_id;
    GLuint face_buffer_id;
    int update_requested;
} PlaneSet;


static void create_plane_buffers(unsigned int n_x_planes, unsigned int n_y_planes, unsigned int n_z_planes);

static void update_plane_buffer_data(float halfwidth, float halfheight, float halfdepth);

static void update_face_set_lookup_tables(void);

static void update_active_plane_set_dimension_and_order(void);


static PlaneSet plane_set;


void create_planes(size_t size_x, size_t size_y, size_t size_z,
                   float extent_x, float extent_y, float extent_z,
                   float spacing_multiplier)
{
    assert(spacing_multiplier > 0);

    if (extent_x <= 0 || extent_y <= 0 || extent_z <= 0)
        print_severe_message("Cannot create new planes with zero or negative extent along any axis.");

    const float max_extent = fmaxf(extent_x, fmaxf(extent_y, extent_z));
    const float scale = 1.0f/max_extent;
    const float halfwidth  = scale*extent_x;
    const float halfheight = scale*extent_y;
    const float halfdepth  = scale*extent_z;

    const unsigned int n_x_planes = (unsigned int)(size_x/spacing_multiplier);
    const unsigned int n_y_planes = (unsigned int)(size_y/spacing_multiplier);
    const unsigned int n_z_planes = (unsigned int)(size_z/spacing_multiplier);

    if (n_x_planes < 2 || n_y_planes < 2 || n_z_planes < 2)
        print_severe_message("Cannot create fewer than two planes along any axis.");

    create_plane_buffers(n_x_planes, n_y_planes, n_z_planes);

    update_plane_buffer_data(halfwidth, halfheight, halfdepth);

    update_face_set_lookup_tables();
}

void load_planes(void)
{
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
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)plane_set.vertex_buffer_size, (GLvoid*)plane_set.vertex_buffer, GL_STATIC_DRAW);
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)plane_set.face_buffer_size, (GLvoid*)plane_set.face_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not bind IBO to VAO");

    glBindVertexArray(0);

    sync_planes();
}

void sync_planes(void)
{
    plane_set.update_requested = 1;
}

void draw_planes()
{
    glBindVertexArray(plane_set.vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing");

    if (plane_set.update_requested)
        update_active_plane_set_dimension_and_order();

    glDrawElements(GL_TRIANGLES,
                   (GLsizei)plane_set.face_set_size_lookup[plane_set.active_axis],
                   GL_UNSIGNED_INT,
                   (GLvoid*)(uintptr_t)plane_set.face_set_offset_lookup[plane_set.active_axis][plane_set.active_order]);
    abort_on_GL_error("Could not draw planes");

    glBindVertexArray(0);
}

void cleanup_planes(void)
{
    if (!plane_set.vertex_buffer || !plane_set.face_buffer)
        print_severe_message("Cannot destroy planes before they are created.");

    glDeleteBuffers(1, &plane_set.face_buffer_id);
    glDeleteBuffers(1, &plane_set.vertex_buffer_id);
    glDeleteVertexArrays(1, &plane_set.vertex_array_object_id);
    abort_on_GL_error("Could not destroy buffer objects");

    free(plane_set.vertex_buffer);
    free(plane_set.face_buffer);

    plane_set.vertex_buffer = NULL;
    plane_set.face_buffer = NULL;

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

    plane_set.update_requested = 0;
}

static void create_plane_buffers(unsigned int n_x_planes, unsigned int n_y_planes, unsigned int n_z_planes)
{
    if (plane_set.vertex_buffer || plane_set.face_buffer)
        print_severe_message("Cannot create new planes before old ones are destroyed.");

    plane_set.x.n_planes = n_x_planes;
    plane_set.y.n_planes = n_y_planes;
    plane_set.z.n_planes = n_z_planes;
    plane_set.n_planes_total = plane_set.x.n_planes + plane_set.y.n_planes + plane_set.z.n_planes;

    plane_set.vertex_buffer_size = plane_set.n_planes_total*(sizeof(PlaneCorners) + sizeof(PlaneTextureCoords));

    // Allocate contiguous memory block for all vertices, and store pointers to relevant segments
    plane_set.vertex_buffer = malloc(plane_set.vertex_buffer_size);

    if (!plane_set.vertex_buffer)
        print_severe_message("Could not allocate memory for plane vertices.");

    PlaneCorners* const all_corners =  (PlaneCorners*)plane_set.vertex_buffer;
    PlaneTextureCoords* const all_texture_coords = (PlaneTextureCoords*)(all_corners + plane_set.n_planes_total);

    plane_set.x.corners = all_corners;
    plane_set.y.corners = plane_set.x.corners + plane_set.x.n_planes;
    plane_set.z.corners = plane_set.y.corners + plane_set.y.n_planes;

    plane_set.x.texture_coords = all_texture_coords;
    plane_set.y.texture_coords = plane_set.x.texture_coords + plane_set.x.n_planes;
    plane_set.z.texture_coords = plane_set.y.texture_coords + plane_set.y.n_planes;

    plane_set.face_buffer_size = 2*plane_set.n_planes_total*sizeof(PlaneFaces);

    // Do the same for face indices
    plane_set.face_buffer = malloc(plane_set.face_buffer_size);

    if (!plane_set.face_buffer)
        print_severe_message("Could not allocate memory for plane faces.");

    plane_set.x.faces_low_to_high = (PlaneFaces*)plane_set.face_buffer;
    plane_set.x.faces_high_to_low = plane_set.x.faces_low_to_high + plane_set.x.n_planes;
    plane_set.y.faces_low_to_high = plane_set.x.faces_high_to_low + plane_set.x.n_planes;
    plane_set.y.faces_high_to_low = plane_set.y.faces_low_to_high + plane_set.y.n_planes;
    plane_set.z.faces_low_to_high = plane_set.y.faces_high_to_low + plane_set.y.n_planes;
    plane_set.z.faces_high_to_low = plane_set.z.faces_low_to_high + plane_set.z.n_planes;
}

static void update_plane_buffer_data(float halfwidth, float halfheight, float halfdepth)
{
    check(plane_set.x.n_planes > 1);
    check(plane_set.y.n_planes > 1);
    check(plane_set.z.n_planes > 1);

    const float dx = 2*halfwidth/(plane_set.x.n_planes - 1);
    const float dy = 2*halfheight/(plane_set.y.n_planes - 1);
    const float dz = 2*halfdepth/(plane_set.z.n_planes - 1);

    const float du = 1.0f/(plane_set.x.n_planes - 1);
    const float dv = 1.0f/(plane_set.y.n_planes - 1);
    const float dw = 1.0f/(plane_set.z.n_planes - 1);

    unsigned int i;
    float position_offset;
    float texture_offset;
    GLuint low_to_high_index_offset;
    GLuint high_to_low_index_offset;

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
}

static void update_face_set_lookup_tables()
{
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
}

static void update_active_plane_set_dimension_and_order(void)
{
    const Matrix4f view_matrix = get_view_transform_matrix();
    const Matrix4f model_matrix = get_model_transform_matrix();

    // Basis vectors of view matrix give orientation of the camera x- y- and z-axes
    Vector3f view_right_axis, view_up_axis, view_look_axis;
    get_matrix4f_x_y_z_basis_vectors(&view_matrix, &view_right_axis, &view_up_axis, &view_look_axis);
    normalize_vector3f(&view_look_axis);

    // Basis vectors of model matrix give orientation of the model frame x- y- and z-axes
    Vector3f model_x_axis, model_y_axis, model_z_axis;
    get_matrix4f_x_y_z_basis_vectors(&model_matrix, &model_x_axis, &model_y_axis, &model_z_axis);
    normalize_vector3f(&model_x_axis);
    normalize_vector3f(&model_y_axis);
    normalize_vector3f(&model_z_axis);

    // Compute the components of the model axes along the camera look axis
    float look_components[3] = {dot3f(&model_x_axis, &view_look_axis), dot3f(&model_y_axis, &view_look_axis), dot3f(&model_z_axis, &view_look_axis)};

    // The plane set to draw is determined by the maximum absolute component
    plane_set.active_axis = argfmax3(fabsf(look_components[0]), fabsf(look_components[1]), fabsf(look_components[2]));

    // The order to draw the planes is determined by the sign of the
    plane_set.active_order = (look_components[plane_set.active_axis] >= 0) ? 0 : 1;

    plane_set.update_requested = 0;
}