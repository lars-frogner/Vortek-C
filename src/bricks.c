/*
 * The volume data is subdivided into separate "bricks" before being transferred
 * to the GPU. This improves data locality on the GPU, and by alternating the
 * orientation of the bricks the memory access pattern can be made more or less
 * view independent (Weiskopf et al. (2004) "Maintaining constant frame rates in
 * 3D texture-based volume rendering").

 * By storing bricks in a space-partitioning tree, they can be efficiently sorted
 * in back to front order when drawing, and invisible bricks can be skipped
 * (Salama and Kolb, 2005). Each brick can additionally be subdivided into even
 * smaller parts, so that most of the empty regions can be skipped (see Ruijters
 * and Vilanova (2006) "Optimizing GPU Volume Rendering").
 */

#include "bricks.h"

#include "error.h"
#include "extra_math.h"
#include "colors.h"
#include "dynamic_string.h"
#include "transformation.h"

#include <stdlib.h>
#include <math.h>


#define MIN_PADDED_BRICK_SIZE 8
#define BOUNDARY_INDICATOR_ALPHA 0.15f


typedef struct NodeIndices
{
    size_t idx[3];
} NodeIndices;

typedef struct Configuration
{
    size_t requested_brick_size;
    unsigned int kernel_size;
    unsigned int sub_brick_size_limit;
    int create_field_boundary_indicator;
    int create_brick_boundary_indicator;
    int create_sub_brick_boundary_indicator;
} Configuration;


static void copy_subarray_with_cycled_layout(const float* full_input_array,
                                             size_t full_input_size_x, size_t full_input_size_y,
                                             size_t input_offset_x, size_t input_offset_y, size_t input_offset_z,
                                             float* output_array,
                                             size_t output_size_x, size_t output_size_y, size_t output_size_z,
                                             unsigned int cycle);

static void create_brick_tree(BrickedField* bricked_field);
static BrickTreeNode* create_brick_tree_nodes(BrickedField* bricked_field, unsigned int level,
                                              NodeIndices start_indices, NodeIndices end_indices);
static BrickTreeNode* create_brick_tree_leaf_node(BrickedField* bricked_field, NodeIndices indices);

static void create_sub_brick_tree(Brick* brick, const Field* field);
static SubBrickTreeNode* create_sub_brick_tree_nodes(const Brick* brick, const Field* field, unsigned int level,
                                                     NodeIndices start_indices, NodeIndices end_indices);
static SubBrickTreeNode* create_sub_brick_tree_node(const Brick* brick, const Field* field,
                                                    NodeIndices start_indices, NodeIndices end_indices);

static void destroy_brick_tree(BrickedField* bricked_field);
static void destroy_brick_tree_node(BrickTreeNode* node);

static void destroy_sub_brick_tree(Brick* brick);
static void destroy_sub_brick_tree_node(SubBrickTreeNode* node);

static void create_boundary_indicator_for_field(BrickedField* bricked_field);
static void create_boundary_indicator_for_bricks(BrickedField* bricked_field);
static void create_boundary_indicator_for_sub_bricks(BrickedField* bricked_field);

static void set_sub_brick_boundary_indicator_data(Indicator* indicator, SubBrickTreeNode* node, size_t* running_vertex_idx, size_t* running_index_idx);

static void draw_brick_boundaries(const BrickTreeNode* node);
static void draw_sub_brick_boundaries(const SubBrickTreeNode* node);


static Configuration configuration;

// Sets of faces adjacent to each cube corner                      //    2----------5
static const unsigned int adjacent_cube_faces[8][3] = {{0, 2, 4},  //   /|         /|
                                                       {1, 2, 4},  //  / |       3/ |
                                                       {0, 3, 4},  // 6----------7 1|
                                                       {0, 2, 5},  // |  | 4   5 |  |
                                                       {1, 2, 5},  // |0 0-------|--1
                                                       {1, 3, 4},  // | /2       | /
                                                       {0, 3, 5},  // |/         |/
                                                       {1, 3, 5}}; // 3----------4

// Sign of the normal direction of each cube face
static const int cube_face_normal_signs[6] = {-1, 1, -1, 1, -1, 1};

static Color field_boundary_color;
static Color brick_boundary_color;
static Color sub_brick_boundary_color;

static unsigned int field_boundary_indicator_count;
static unsigned int brick_boundary_indicator_count;
static unsigned int sub_brick_boundary_indicator_count;


void initialize_bricks(void)
{
    configuration.requested_brick_size = 64;
    configuration.kernel_size = 2;
    configuration.sub_brick_size_limit = 2*6;
    configuration.create_field_boundary_indicator = 1;
    configuration.create_brick_boundary_indicator = 0;
    configuration.create_sub_brick_boundary_indicator = 0;

    field_boundary_color = create_standard_color(COLOR_WHITE, BOUNDARY_INDICATOR_ALPHA);
    brick_boundary_color = create_standard_color(COLOR_YELLOW, BOUNDARY_INDICATOR_ALPHA);
    sub_brick_boundary_color = create_standard_color(COLOR_CYAN, BOUNDARY_INDICATOR_ALPHA);

    field_boundary_indicator_count = 0;
    brick_boundary_indicator_count = 0;
    sub_brick_boundary_indicator_count = 0;
}

void reset_bricked_field(BrickedField* bricked_field)
{
    bricked_field->field = NULL;
    bricked_field->bricks = NULL;
    bricked_field->tree = NULL;
    bricked_field->n_bricks = 0;
    bricked_field->n_bricks_x = 0;
    bricked_field->n_bricks_y = 0;
    bricked_field->n_bricks_z = 0;
    bricked_field->brick_size = 0;
    bricked_field->texture_unit = 0;
    bricked_field->field_boundary_indicator_name = NULL;
    bricked_field->brick_boundary_indicator_name = NULL;
    bricked_field->sub_brick_boundary_indicator_name = NULL;
}

void set_brick_size_exponent(unsigned int brick_size_exponent)
{
    configuration.requested_brick_size = pow2_size_t(brick_size_exponent);
}

void set_bricked_field_kernel_size(unsigned int kernel_size)
{
    check(kernel_size > 0);
    configuration.kernel_size = kernel_size;
}

void set_min_sub_brick_size(unsigned int min_sub_brick_size)
{
    check(min_sub_brick_size > 0);
    configuration.sub_brick_size_limit = 2*min_sub_brick_size;
}

void set_field_boundary_indicator_creation(int state)
{
    check(state == 0 || state == 1);
    configuration.create_field_boundary_indicator = state;
}

void set_brick_boundary_indicator_creation(int state)
{
    check(state == 0 || state == 1);
    configuration.create_brick_boundary_indicator = state;
}

void set_sub_brick_boundary_indicator_creation(int state)
{
    check(state == 0 || state == 1);
    configuration.create_sub_brick_boundary_indicator = state;
}

void create_bricked_field(BrickedField* bricked_field, Field* field)
{
    check(bricked_field);
    check(field);
    check(field->data);

    if (field->type != SCALAR_FIELD)
        print_severe_message("Bricking is only supported for scalar fields.");

    float* const field_data = field->data;
    const size_t field_size_x = field->size_x;
    const size_t field_size_y = field->size_y;
    const size_t field_size_z = field->size_z;

    size_t pad_size = configuration.kernel_size - 1; // The number of voxels to pad on each side is one less than the size of the interpolation kernel

    // In the special case of using only one brick that exactly fits the field, no padding is needed
    if (configuration.requested_brick_size == field_size_x &&
        configuration.requested_brick_size == field_size_y &&
        configuration.requested_brick_size == field_size_z)
        pad_size = 0;

    size_t padded_brick_size = max_size_t(configuration.requested_brick_size, MIN_PADDED_BRICK_SIZE);

    // Make sure that the brick size without padding will never be smaller than the pad size
    if (padded_brick_size < 3*pad_size)
        padded_brick_size = closest_ge_pow2_size_t(3*pad_size);

    const size_t brick_size = padded_brick_size - 2*pad_size;

    if (brick_size > field_size_x || brick_size > field_size_y || brick_size > field_size_z)
        print_severe_message("Brick dimensions (%d, %d, %d) exceed field dimensions of (%d, %d, %d).",
                             brick_size, brick_size, brick_size, field_size_x, field_size_y, field_size_z);

    const size_t n_full_bricks_x = field_size_x/brick_size;
    const size_t n_full_bricks_y = field_size_y/brick_size;
    const size_t n_full_bricks_z = field_size_z/brick_size;

    // When the volume cannot be evenly divided by the brick size, we will simply append extra smaller bricks
    const size_t has_remaining_x = (field_size_x - n_full_bricks_x*brick_size) > 0;
    const size_t has_remaining_y = (field_size_y - n_full_bricks_y*brick_size) > 0;
    const size_t has_remaining_z = (field_size_z - n_full_bricks_z*brick_size) > 0;

    const size_t n_bricks_x = n_full_bricks_x + has_remaining_x;
    const size_t n_bricks_y = n_full_bricks_y + has_remaining_y;
    const size_t n_bricks_z = n_full_bricks_z + has_remaining_z;

    const size_t n_bricks = n_bricks_x*n_bricks_y*n_bricks_z;

    Brick* const bricks = (Brick*)malloc(sizeof(Brick)*n_bricks);
    check(bricks);

    const size_t new_data_size_x = field_size_x + 2*pad_size*(n_bricks_x - 1);
    const size_t new_data_size_y = field_size_y + 2*pad_size*(n_bricks_y - 1);
    const size_t new_data_size_z = field_size_z + 2*pad_size*(n_bricks_z - 1);
    const size_t new_data_length = new_data_size_x*new_data_size_y*new_data_size_z;

    // Values for all the bricks are stored in the same array, which will be the one pointed to by the first brick
    float* const new_data = (float*)calloc(new_data_length, sizeof(float));
    check(new_data);

    size_t i, j, k;
    Brick* brick;
    size_t brick_idx;
    size_t data_offset = 0;
    unsigned int cycle;
    size_t unpadded_brick_offset_x;
    size_t unpadded_brick_offset_y;
    size_t unpadded_brick_offset_z;
    size_t unpadded_brick_size_x;
    size_t unpadded_brick_size_y;
    size_t unpadded_brick_size_z;
    size_t padded_brick_size_x;
    size_t padded_brick_size_y;
    size_t padded_brick_size_z;
    size_t field_offset_x;
    size_t field_offset_y;
    size_t field_offset_z;

    const unsigned int permutations[3][3] = {{0, 1, 2}, {2, 0, 1}, {1, 2, 0}};

    for (k = 0; k < n_bricks_z; k++)
    {
        for (j = 0; j < n_bricks_y; j++)
        {
            for (i = 0; i < n_bricks_x; i++)
            {
                brick_idx = (k*n_bricks_y + j)*n_bricks_x + i;
                brick = bricks + brick_idx;

                brick->data = new_data + data_offset;

                // This cycling of the brick orientation ensures that no direct neighbors have the same orientation
                cycle = (i + j + k) % 3;
                brick->orientation = (enum brick_orientation)cycle;

                unpadded_brick_offset_x = i*brick_size;
                unpadded_brick_offset_y = j*brick_size;
                unpadded_brick_offset_z = k*brick_size;

                // Truncate the brick size if it reaches the upper edges of the field
                unpadded_brick_size_x = min_size_t(brick_size, field_size_x - unpadded_brick_offset_x);
                unpadded_brick_size_y = min_size_t(brick_size, field_size_y - unpadded_brick_offset_y);
                unpadded_brick_size_z = min_size_t(brick_size, field_size_z - unpadded_brick_offset_z);

                // Only the edges of the brick interior to the field are padded
                padded_brick_size_x = unpadded_brick_size_x + (i > 0)*pad_size + (i < n_bricks_x - 1)*pad_size;
                padded_brick_size_y = unpadded_brick_size_y + (j > 0)*pad_size + (j < n_bricks_y - 1)*pad_size;
                padded_brick_size_z = unpadded_brick_size_z + (k > 0)*pad_size + (k < n_bricks_z - 1)*pad_size;

                // The padded dimensions of the brick are listed from fastest to slowest varying
                brick->padded_size[permutations[cycle][0]] = padded_brick_size_x;
                brick->padded_size[permutations[cycle][1]] = padded_brick_size_y;
                brick->padded_size[permutations[cycle][2]] = padded_brick_size_z;

                brick->offset_x = unpadded_brick_offset_x + (i == 0)*pad_size;
                brick->offset_y = unpadded_brick_offset_y + (j == 0)*pad_size;
                brick->offset_z = unpadded_brick_offset_z + (k == 0)*pad_size;

                brick->size_x = unpadded_brick_size_x - (i == 0)*pad_size - (i == n_bricks_x - 1)*pad_size;
                brick->size_y = unpadded_brick_size_y - (j == 0)*pad_size - (j == n_bricks_y - 1)*pad_size;
                brick->size_z = unpadded_brick_size_z - (k == 0)*pad_size - (k == n_bricks_z - 1)*pad_size;

                set_vector3f_elements(&brick->spatial_offset,
                                      (float)brick->offset_x*field->voxel_width  - field->halfwidth,
                                      (float)brick->offset_y*field->voxel_height - field->halfheight,
                                      (float)brick->offset_z*field->voxel_depth  - field->halfdepth);

                set_vector3f_elements(&brick->spatial_extent,
                                      (float)brick->size_x*field->voxel_width,
                                      (float)brick->size_y*field->voxel_height,
                                      (float)brick->size_z*field->voxel_depth);

                set_vector3f_elements(&brick->pad_fractions,
                                      (float)pad_size/(float)padded_brick_size_x,
                                      (float)pad_size/(float)padded_brick_size_y,
                                      (float)pad_size/(float)padded_brick_size_z);

                data_offset += padded_brick_size_x*padded_brick_size_y*padded_brick_size_z;

                // Decrease offset into the original data array to include the padding data (unless we are at a lower edge)
                field_offset_x = unpadded_brick_offset_x - (i > 0)*pad_size;
                field_offset_y = unpadded_brick_offset_y - (j > 0)*pad_size;
                field_offset_z = unpadded_brick_offset_z - (k > 0)*pad_size;

                copy_subarray_with_cycled_layout(field_data,
                                                 field_size_x, field_size_y,
                                                 field_offset_x, field_offset_y, field_offset_z,
                                                 brick->data,
                                                 padded_brick_size_x, padded_brick_size_y, padded_brick_size_z,
                                                 cycle);

                brick->texture_id = 0;
            }
        }
    }

    bricked_field->field = field;

    bricked_field->bricks = bricks;
    bricked_field->n_bricks = n_bricks;
    bricked_field->n_bricks_x = n_bricks_x;
    bricked_field->n_bricks_y = n_bricks_y;
    bricked_field->n_bricks_z = n_bricks_z;

    bricked_field->brick_size = brick_size;

    create_brick_tree(bricked_field);

    if (configuration.create_field_boundary_indicator)
        create_boundary_indicator_for_field(bricked_field);
    else
        bricked_field->field_boundary_indicator_name = NULL;

    if (configuration.create_brick_boundary_indicator)
        create_boundary_indicator_for_bricks(bricked_field);
    else
        bricked_field->brick_boundary_indicator_name = NULL;

    if (configuration.create_sub_brick_boundary_indicator)
        create_boundary_indicator_for_sub_bricks(bricked_field);
    else
        bricked_field->sub_brick_boundary_indicator_name = NULL;
}

void draw_field_boundary_indicator(const BrickedField* bricked_field, unsigned int reference_corner_idx, enum indicator_drawing_pass pass)
{
    assert(bricked_field);
    assert(reference_corner_idx < 8);

    if (!bricked_field->field_boundary_indicator_name)
        return;

    Indicator* const indicator = get_indicator(bricked_field->field_boundary_indicator_name);

    const Vector3f reference_corner = extract_vector3f_from_vector4f(indicator->vertices.positions + reference_corner_idx);

    glUseProgram(get_active_indicator_shader_program_id());
    abort_on_GL_error("Could not use shader program for drawing indicator");

    assert(indicator->vertex_array_object_id > 0);
    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing indicator");

    unsigned int face_is_visible[6] = {0};

    for (unsigned int dim = 0; dim < 3; dim++)
    {
        const unsigned int adjacent_face_idx = adjacent_cube_faces[reference_corner_idx][dim];
        face_is_visible[adjacent_face_idx] = (float)cube_face_normal_signs[adjacent_face_idx]
                                             *get_component_of_vector_from_model_point_to_camera(&reference_corner, dim) >= 0;
    }

    for (unsigned int face_idx = 0; face_idx < 6; face_idx++)
    {
        if ((pass == INDICATOR_FRONT_PASS && face_is_visible[face_idx]) || (pass == INDICATOR_BACK_PASS && !face_is_visible[face_idx]))
        {
            glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, (GLvoid*)(4*face_idx*sizeof(unsigned int)));
            abort_on_GL_error("Could not draw indicator");
        }
    }

    glBindVertexArray(0);

    glUseProgram(0);
}

void draw_brick_boundary_indicator(const BrickedField* bricked_field)
{
    assert(bricked_field);

    if (!bricked_field->brick_boundary_indicator_name)
        return;

    Indicator* const indicator = get_indicator(bricked_field->brick_boundary_indicator_name);

    glUseProgram(get_active_indicator_shader_program_id());
    abort_on_GL_error("Could not use shader program for drawing indicator");

    assert(indicator->vertex_array_object_id > 0);
    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing indicator");

    glDrawElements(GL_LINES, (GLsizei)indicator->n_indices, GL_UNSIGNED_INT, (GLvoid*)0);
    abort_on_GL_error("Could not draw indicator");

    glBindVertexArray(0);

    glUseProgram(0);
}

void draw_sub_brick_boundary_indicator(const BrickedField* bricked_field)
{
    assert(bricked_field);

    if (!bricked_field->sub_brick_boundary_indicator_name)
        return;

    Indicator* const indicator = get_indicator(bricked_field->sub_brick_boundary_indicator_name);

    glUseProgram(get_active_indicator_shader_program_id());
    abort_on_GL_error("Could not use shader program");

    assert(indicator->vertex_array_object_id > 0);
    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing indicator");

    draw_brick_boundaries(bricked_field->tree);

    glBindVertexArray(0);

    glUseProgram(0);
}

void destroy_bricked_field(BrickedField* bricked_field)
{
    check(bricked_field);

    if (bricked_field->field_boundary_indicator_name)
        destroy_indicator(bricked_field->field_boundary_indicator_name);

    if (bricked_field->brick_boundary_indicator_name)
        destroy_indicator(bricked_field->brick_boundary_indicator_name);

    if (bricked_field->sub_brick_boundary_indicator_name)
        destroy_indicator(bricked_field->sub_brick_boundary_indicator_name);

    destroy_brick_tree(bricked_field);

    if (bricked_field->bricks)
    {
        if (bricked_field->bricks[0].data)
            free(bricked_field->bricks[0].data);

        free(bricked_field->bricks);
    }

    reset_bricked_field(bricked_field);
}

static void copy_subarray_with_cycled_layout(const float* full_input_array,
                                             size_t full_input_size_x, size_t full_input_size_y,
                                             size_t input_offset_x, size_t input_offset_y, size_t input_offset_z,
                                             float* output_array,
                                             size_t output_size_x, size_t output_size_y, size_t output_size_z,
                                             unsigned int cycle)
{
    /*
    This function copies data from an input array to an output array.
    Data is copied from a sub-region of the input array specified by
    an offset and a size into the output array (which has the same size
    as the input subregion).

    The cycle argument specifies how the copied data should be laid out
    in memory. A cycle of 0 keeps the original layout with the x-dimension
    varying fastest (zyx). Cycle 1 cycles this order once so y varies fastest
    (xzy) and cycle 2 cycles it twice so z varies fastest (yxz).
    */

    check(full_input_array);
    check(output_array);
    check(output_array != full_input_array);
    check(cycle < 3);

    const size_t input_offset = (input_offset_z*full_input_size_y + input_offset_y)*full_input_size_x + input_offset_x;

    size_t i, j, k;
    size_t input_idx, output_idx;

    if (cycle == 0)
    {
        for (k = 0; k < output_size_z; k++)
            for (j = 0; j < output_size_y; j++)
                for (i = 0; i < output_size_x; i++)
                {
                    input_idx = input_offset + (k*full_input_size_y + j)*full_input_size_x + i;
                    output_idx = (k*output_size_y + j)*output_size_x + i;

                    output_array[output_idx] = full_input_array[input_idx];
                }
    }
    else if (cycle == 1)
    {
        for (i = 0; i < output_size_x; i++)
            for (k = 0; k < output_size_z; k++)
                for (j = 0; j < output_size_y; j++)
                {
                    input_idx = input_offset + (k*full_input_size_y + j)*full_input_size_x + i;
                    output_idx = (i*output_size_z + k)*output_size_y + j;

                    output_array[output_idx] = full_input_array[input_idx];
                }
    }
    else
    {
        for (j = 0; j < output_size_y; j++)
            for (i = 0; i < output_size_x; i++)
                for (k = 0; k < output_size_z; k++)
                {
                    input_idx = input_offset + (k*full_input_size_y + j)*full_input_size_x + i;
                    output_idx = (j*output_size_x + i)*output_size_z + k;

                    output_array[output_idx] = full_input_array[input_idx];
                }
    }
}

static void create_brick_tree(BrickedField* bricked_field)
{
    assert(bricked_field);

    const NodeIndices start_indices = {{0, 0, 0}};
    const NodeIndices end_indices = {{bricked_field->n_bricks_x, bricked_field->n_bricks_y, bricked_field->n_bricks_z}};

    bricked_field->tree = create_brick_tree_nodes(bricked_field, 0, start_indices, end_indices);
    check(bricked_field->tree);
}

static BrickTreeNode* create_brick_tree_nodes(BrickedField* bricked_field, unsigned int level,
                                              NodeIndices start_indices, NodeIndices end_indices)
{
    assert(bricked_field);

    unsigned int axis = level % 3;

    // Advance the level until a divisible axis is found or return a leaf node if none is found
    if (end_indices.idx[axis] - start_indices.idx[axis] == 1)
    {
        level++;
        axis = level % 3;

        if (end_indices.idx[axis] - start_indices.idx[axis] == 1)
        {
            level++;
            axis = level % 3;

            if (end_indices.idx[axis] - start_indices.idx[axis] == 1)
            {
                return create_brick_tree_leaf_node(bricked_field, start_indices);
            }
        }
    }

    BrickTreeNode* node = (BrickTreeNode*)malloc(sizeof(BrickTreeNode));
    node->brick = NULL;
    node->visibility_ratio = 1.0f;
    node->visibility = UNDETERMINED_REGION_VISIBILITY;

    node->split_axis = axis;

    // Subdivide along the current axis as close to the middle as possible
    const size_t middle_idx = (size_t)(0.5f*(float)(start_indices.idx[axis] + end_indices.idx[axis] + 1));
    assert(middle_idx > start_indices.idx[axis] && end_indices.idx[axis] > middle_idx);

    // Create child node for the lower interval
    NodeIndices new_end_indices = end_indices;
    new_end_indices.idx[axis] = middle_idx;
    node->lower_child = create_brick_tree_nodes(bricked_field, level + 1, start_indices, new_end_indices);

    // Create child node for the upper interval
    NodeIndices new_start_indices = start_indices;
    new_start_indices.idx[axis] = middle_idx;
    node->upper_child = create_brick_tree_nodes(bricked_field, level + 1, new_start_indices, end_indices);

    // The spatial offset of the node along the split axis is the minimum of the children's offset along that axis.
    // The other components of the offset are equal for both children and also the same for this node.
    node->spatial_offset = node->lower_child->spatial_offset;
    node->spatial_offset.a[axis] = fminf(node->lower_child->spatial_offset.a[axis], node->upper_child->spatial_offset.a[axis]);

    // The spatial extent of the node along the split axis is the sum of the children's extent along that axis.
    // The other components of the extent are equal for both children and also the same for this node.
    node->spatial_extent = node->lower_child->spatial_extent;
    node->spatial_extent.a[axis] += node->upper_child->spatial_extent.a[axis];

    node->n_children = 2 + node->lower_child->n_children + node->upper_child->n_children;

    return node;
}

static BrickTreeNode* create_brick_tree_leaf_node(BrickedField* bricked_field, NodeIndices indices)
{
    assert(bricked_field);

    BrickTreeNode* node = (BrickTreeNode*)malloc(sizeof(BrickTreeNode));
    node->lower_child = NULL;
    node->upper_child = NULL;
    node->n_children = 0;
    node->visibility_ratio = 1.0f;
    node->visibility = UNDETERMINED_REGION_VISIBILITY;

    node->brick = bricked_field->bricks + (indices.idx[2]*bricked_field->n_bricks_y + indices.idx[1])*bricked_field->n_bricks_x + indices.idx[0];

    node->spatial_offset = node->brick->spatial_offset;
    node->spatial_extent = node->brick->spatial_extent;

    create_sub_brick_tree(node->brick, bricked_field->field);

    return node;
}

static void create_sub_brick_tree(Brick* brick, const Field* field)
{
    assert(brick);
    assert(field);

    const NodeIndices start_indices = {{0, 0, 0}};
    const NodeIndices end_indices = {{brick->size_x, brick->size_y, brick->size_z}};

    brick->tree = create_sub_brick_tree_nodes(brick, field, 0, start_indices, end_indices);
    check(brick->tree);
}

static SubBrickTreeNode* create_sub_brick_tree_nodes(const Brick* brick, const Field* field, unsigned int level,
                                                     NodeIndices start_indices, NodeIndices end_indices)
{
    assert(brick);
    assert(field);

    SubBrickTreeNode* node = create_sub_brick_tree_node(brick, field, start_indices, end_indices);

    unsigned int axis = level % 3;

    // Advance the level until a divisible axis is found or return a leaf node if none is found
    if (end_indices.idx[axis] - start_indices.idx[axis] < configuration.sub_brick_size_limit)
    {
        level++;
        axis = level % 3;

        if (end_indices.idx[axis] - start_indices.idx[axis] < configuration.sub_brick_size_limit)
        {
            level++;
            axis = level % 3;

            if (end_indices.idx[axis] - start_indices.idx[axis] < configuration.sub_brick_size_limit)
            {
                return node;
            }
        }
    }

    node->split_axis = axis;

    // Subdivide along the current axis as close to the middle as possible (rounding down)
    const size_t middle_idx = (size_t)(0.5f*(float)(start_indices.idx[axis] + end_indices.idx[axis]));
    assert(middle_idx > start_indices.idx[axis] && end_indices.idx[axis] > middle_idx);

    // Create child node for the lower interval
    NodeIndices new_end_indices = end_indices;
    new_end_indices.idx[axis] = middle_idx;
    node->lower_child = create_sub_brick_tree_nodes(brick, field, level + 1, start_indices, new_end_indices);

    // Create child node for the upper interval
    NodeIndices new_start_indices = start_indices;
    new_start_indices.idx[axis] = middle_idx;
    node->upper_child = create_sub_brick_tree_nodes(brick, field, level + 1, new_start_indices, end_indices);

    node->n_children = 2 + node->lower_child->n_children + node->upper_child->n_children;

    return node;
}

static SubBrickTreeNode* create_sub_brick_tree_node(const Brick* brick, const Field* field,
                                                    NodeIndices start_indices, NodeIndices end_indices)
{
    SubBrickTreeNode* node = (SubBrickTreeNode*)malloc(sizeof(SubBrickTreeNode));

    node->lower_child = NULL;
    node->upper_child = NULL;
    node->split_axis = 0;
    node->n_children = 0;

    node->offset_x = brick->offset_x + start_indices.idx[0];
    node->offset_y = brick->offset_y + start_indices.idx[1];
    node->offset_z = brick->offset_z + start_indices.idx[2];

    node->size_x = end_indices.idx[0] - start_indices.idx[0];
    node->size_y = end_indices.idx[1] - start_indices.idx[1];
    node->size_z = end_indices.idx[2] - start_indices.idx[2];

    set_vector3f_elements(&node->spatial_offset,
                          brick->spatial_offset.a[0] + (float)start_indices.idx[0]*field->voxel_width,
                          brick->spatial_offset.a[1] + (float)start_indices.idx[1]*field->voxel_height,
                          brick->spatial_offset.a[2] + (float)start_indices.idx[2]*field->voxel_depth);

    set_vector3f_elements(&node->spatial_extent,
                          (float)node->size_x*field->voxel_width,
                          (float)node->size_y*field->voxel_height,
                          (float)node->size_z*field->voxel_depth);

    node->visibility_ratio = 1.0f;
    node->visibility = UNDETERMINED_REGION_VISIBILITY;

    return node;
}

static void destroy_brick_tree(BrickedField* bricked_field)
{
    assert(bricked_field);

    if (bricked_field->tree)
        destroy_brick_tree_node(bricked_field->tree);

    bricked_field->tree = NULL;
}

static void destroy_brick_tree_node(BrickTreeNode* node)
{
    assert(node);

    if (node->brick)
        destroy_sub_brick_tree(node->brick);

    if (node->lower_child)
        destroy_brick_tree_node(node->lower_child);

    if (node->upper_child)
        destroy_brick_tree_node(node->upper_child);

    free(node);
}

static void destroy_sub_brick_tree(Brick* brick)
{
    assert(brick);

    if (brick->tree)
        destroy_sub_brick_tree_node(brick->tree);

    brick->tree = NULL;
}

static void destroy_sub_brick_tree_node(SubBrickTreeNode* node)
{
    assert(node);

    if (node->lower_child)
        destroy_sub_brick_tree_node(node->lower_child);

    if (node->upper_child)
        destroy_sub_brick_tree_node(node->upper_child);

    free(node);
}

static void create_boundary_indicator_for_field(BrickedField* bricked_field)
{
    check(bricked_field);

    Field* const field = bricked_field->field;

    const DynamicString name = create_string("field_boundaries_%d", field_boundary_indicator_count++);
    Indicator* const indicator = create_indicator(&name, 8, 24);

    Vector3f lower_corner = {{-field->halfwidth, -field->halfheight, -field->halfdepth}};
    Vector3f extent = {{2*field->halfwidth, 2*field->halfheight, 2*field->halfdepth}};

    size_t vertex_idx = 0;
    set_cube_vertex_positions_for_indicator(indicator, &vertex_idx, &lower_corner, &extent);

    size_t index_idx = 0;
    set_cube_edges_for_indicator(indicator, 0, &index_idx);

    set_vertex_colors_for_indicator(indicator, 0, indicator->n_vertices, &field_boundary_color);

    load_buffer_data_for_indicator(indicator);

    bricked_field->field_boundary_indicator_name = indicator->name.chars;
}

static void create_boundary_indicator_for_bricks(BrickedField* bricked_field)
{
    check(bricked_field);

    const DynamicString name = create_string("brick_boundaries_%d", brick_boundary_indicator_count++);
    Indicator* const indicator = create_indicator(&name, 8*bricked_field->n_bricks, 24*bricked_field->n_bricks);

    size_t vertex_idx = 0;
    size_t index_idx = 0;

    for (size_t brick_idx = 0; brick_idx < bricked_field->n_bricks; brick_idx++)
    {
        const Brick* const brick = bricked_field->bricks + brick_idx;
        set_cube_edges_for_indicator(indicator, vertex_idx, &index_idx);
        set_cube_vertex_positions_for_indicator(indicator, &vertex_idx, &brick->spatial_offset, &brick->spatial_extent);
    }

    set_vertex_colors_for_indicator(indicator, 0, indicator->n_vertices, &brick_boundary_color);

    load_buffer_data_for_indicator(indicator);

    bricked_field->brick_boundary_indicator_name = indicator->name.chars;
}

static void create_boundary_indicator_for_sub_bricks(BrickedField* bricked_field)
{
    check(bricked_field);

    size_t brick_idx;
    size_t n_sub_bricks = 0;

    for (brick_idx = 0; brick_idx < bricked_field->n_bricks; brick_idx++)
    {
        const Brick* const brick = bricked_field->bricks + brick_idx;
        n_sub_bricks += 1 + brick->tree->n_children;
    }

    const DynamicString name = create_string("sub_brick_boundaries_%d", sub_brick_boundary_indicator_count++);
    Indicator* const indicator = create_indicator(&name, 8*n_sub_bricks, 24*n_sub_bricks);

    size_t vertex_idx = 0;
    size_t index_idx = 0;

    for (brick_idx = 0; brick_idx < bricked_field->n_bricks; brick_idx++)
    {
        Brick* const brick = bricked_field->bricks + brick_idx;
        set_sub_brick_boundary_indicator_data(indicator, brick->tree, &vertex_idx, &index_idx);
    }

    set_vertex_colors_for_indicator(indicator, 0, indicator->n_vertices, &sub_brick_boundary_color);

    load_buffer_data_for_indicator(indicator);

    bricked_field->sub_brick_boundary_indicator_name = indicator->name.chars;
}

static void set_sub_brick_boundary_indicator_data(Indicator* indicator, SubBrickTreeNode* node, size_t* running_vertex_idx, size_t* running_index_idx)
{
    assert(indicator);
    assert(node);
    assert(running_vertex_idx);
    assert(running_index_idx);

    if (node->lower_child)
        set_sub_brick_boundary_indicator_data(indicator, node->lower_child, running_vertex_idx, running_index_idx);

    if (node->upper_child)
        set_sub_brick_boundary_indicator_data(indicator, node->upper_child, running_vertex_idx, running_index_idx);

    node->indicator_idx = *running_index_idx;
    set_cube_edges_for_indicator(indicator, *running_vertex_idx, running_index_idx);
    set_cube_vertex_positions_for_indicator(indicator, running_vertex_idx, &node->spatial_offset, &node->spatial_extent);
}

static void draw_brick_boundaries(const BrickTreeNode* node)
{
    assert(node);

    if (node->visibility == REGION_INVISIBLE || node->visibility == REGION_CLIPPED)
        return;

    if (node->brick)
    {
        draw_sub_brick_boundaries(node->brick->tree);
    }
    else
    {
        if (node->lower_child)
            draw_brick_boundaries(node->lower_child);

        if (node->upper_child)
            draw_brick_boundaries(node->upper_child);
    }
}

static void draw_sub_brick_boundaries(const SubBrickTreeNode* node)
{
    assert(node);

    if (node->visibility == REGION_INVISIBLE || node->visibility == REGION_CLIPPED)
        return;

    if (node->visibility == REGION_VISIBLE)
    {
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, (GLvoid*)(node->indicator_idx*sizeof(unsigned int)));
        abort_on_GL_error("Could not draw indicator");
    }
    else
    {
        if (node->lower_child)
            draw_sub_brick_boundaries(node->lower_child);

        if (node->upper_child)
            draw_sub_brick_boundaries(node->upper_child);
    }
}
