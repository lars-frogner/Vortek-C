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

#include <stdlib.h>
#include <math.h>


#define MIN_PADDED_BRICK_SIZE 8


typedef struct NodeIndices
{
    size_t idx[3];
} NodeIndices;


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


static unsigned int sub_brick_size_limit = 16;


void set_min_sub_brick_size(unsigned int size)
{
    check(size > 0);
    sub_brick_size_limit = 2*size;
}

void create_bricked_field(BrickedField* bricked_field, const Field* field, unsigned int brick_size_exponent, unsigned int kernel_size)
{
    check(bricked_field);
    check(field);
    check(field->data);
    check(kernel_size > 0);

    if (field->type != SCALAR_FIELD)
        print_severe_message("Bricking is only supported for scalar fields.");

    float* const field_data = field->data;
    const size_t field_size_x = field->size_x;
    const size_t field_size_y = field->size_y;
    const size_t field_size_z = field->size_z;

    size_t pad_size = kernel_size - 1; // The number of voxels to pad on each side is one less than the size of the interpolation kernel

    const size_t requested_brick_size = pow2_size_t(brick_size_exponent);

    // In the special case of using only one brick that exactly fits the field, no padding is needed
    if (requested_brick_size == field_size_x &&
        requested_brick_size == field_size_y &&
        requested_brick_size == field_size_z)
        pad_size = 0;

    size_t padded_brick_size = max_size_t(requested_brick_size, MIN_PADDED_BRICK_SIZE);

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
                                      brick->offset_x*field->voxel_width  - field->halfwidth,
                                      brick->offset_y*field->voxel_height - field->halfheight,
                                      brick->offset_z*field->voxel_depth  - field->halfdepth);

                set_vector3f_elements(&brick->spatial_extent,
                                      brick->size_x*field->voxel_width,
                                      brick->size_y*field->voxel_height,
                                      brick->size_z*field->voxel_depth);

                set_vector3f_elements(&brick->pad_fractions,
                                      (float)pad_size/padded_brick_size_x,
                                      (float)pad_size/padded_brick_size_y,
                                      (float)pad_size/padded_brick_size_z);

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

    bricked_field->texture_unit = 0;

    create_brick_tree(bricked_field);
}

void destroy_bricked_field(BrickedField* bricked_field)
{
    check(bricked_field);

    destroy_brick_tree(bricked_field);

    bricked_field->field = NULL;
    bricked_field->n_bricks = 0;

    if (!bricked_field->bricks)
        return;

    if (bricked_field->bricks[0].data)
        free(bricked_field->bricks[0].data);

    free(bricked_field->bricks);
    bricked_field->bricks = NULL;
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

    node->split_axis = axis;

    // Subdivide along the current axis as close to the middle as possible
    const size_t middle_idx = (size_t)(0.5f*(start_indices.idx[axis] + end_indices.idx[axis] + 1));
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
    copy_vector3f(&node->lower_child->spatial_offset, &node->spatial_offset);
    node->spatial_offset.a[axis] = fminf(node->lower_child->spatial_offset.a[axis], node->upper_child->spatial_offset.a[axis]);

    // The spatial extent of the node along the split axis is the sum of the children's extent along that axis.
    // The other components of the extent are equal for both children and also the same for this node.
    copy_vector3f(&node->lower_child->spatial_extent, &node->spatial_extent);
    node->spatial_extent.a[axis] += node->upper_child->spatial_extent.a[axis];

    return node;
}

static BrickTreeNode* create_brick_tree_leaf_node(BrickedField* bricked_field, NodeIndices indices)
{
    assert(bricked_field);

    BrickTreeNode* node = (BrickTreeNode*)malloc(sizeof(BrickTreeNode));
    node->lower_child = NULL;
    node->upper_child = NULL;
    node->visibility_ratio = 1.0f;

    node->brick = bricked_field->bricks + (indices.idx[2]*bricked_field->n_bricks_y + indices.idx[1])*bricked_field->n_bricks_x + indices.idx[0];

    copy_vector3f(&node->brick->spatial_offset, &node->spatial_offset);
    copy_vector3f(&node->brick->spatial_extent, &node->spatial_extent);

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
    if (end_indices.idx[axis] - start_indices.idx[axis] < sub_brick_size_limit)
    {
        level++;
        axis = level % 3;

        if (end_indices.idx[axis] - start_indices.idx[axis] < sub_brick_size_limit)
        {
            level++;
            axis = level % 3;

            if (end_indices.idx[axis] - start_indices.idx[axis] < sub_brick_size_limit)
            {
                return node;
            }
        }
    }

    node->split_axis = axis;

    // Subdivide along the current axis as close to the middle as possible (rounding down)
    const size_t middle_idx = (size_t)(0.5f*(start_indices.idx[axis] + end_indices.idx[axis]));
    assert(middle_idx > start_indices.idx[axis] && end_indices.idx[axis] > middle_idx);

    // Create child node for the lower interval
    NodeIndices new_end_indices = end_indices;
    new_end_indices.idx[axis] = middle_idx;
    node->lower_child = create_sub_brick_tree_nodes(brick, field, level + 1, start_indices, new_end_indices);

    // Create child node for the upper interval
    NodeIndices new_start_indices = start_indices;
    new_start_indices.idx[axis] = middle_idx;
    node->upper_child = create_sub_brick_tree_nodes(brick, field, level + 1, new_start_indices, end_indices);

    return node;
}

static SubBrickTreeNode* create_sub_brick_tree_node(const Brick* brick, const Field* field,
                                                    NodeIndices start_indices, NodeIndices end_indices)
{
    SubBrickTreeNode* node = (SubBrickTreeNode*)malloc(sizeof(SubBrickTreeNode));

    node->lower_child = NULL;
    node->upper_child = NULL;
    node->split_axis = 0;

    node->offset_x = brick->offset_x + start_indices.idx[0];
    node->offset_y = brick->offset_y + start_indices.idx[1];
    node->offset_z = brick->offset_z + start_indices.idx[2];

    node->size_x = end_indices.idx[0] - start_indices.idx[0];
    node->size_y = end_indices.idx[1] - start_indices.idx[1];
    node->size_z = end_indices.idx[2] - start_indices.idx[2];

    set_vector3f_elements(&node->spatial_offset,
                          brick->spatial_offset.a[0] + start_indices.idx[0]*field->voxel_width,
                          brick->spatial_offset.a[1] + start_indices.idx[1]*field->voxel_height,
                          brick->spatial_offset.a[2] + start_indices.idx[2]*field->voxel_depth);

    set_vector3f_elements(&node->spatial_extent,
                          node->size_x*field->voxel_width,
                          node->size_y*field->voxel_height,
                          node->size_z*field->voxel_depth);

    node->visibility_ratio = 1.0f;

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
