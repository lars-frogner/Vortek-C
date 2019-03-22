#ifndef BRICKS_H
#define BRICKS_H

#include "gl_includes.h"
#include "geometry.h"
#include "fields.h"

#include <stddef.h>

enum brick_orientation {ORIENTED_ZYX = 0, ORIENTED_XZY = 1, ORIENTED_YXZ = 2};

typedef struct BrickTreeNode BrickTreeNode;
typedef struct SubBrickTreeNode SubBrickTreeNode;

typedef struct Brick
{
    float* data;
    SubBrickTreeNode* tree;
    enum brick_orientation orientation;
    size_t offset_x;
    size_t offset_y;
    size_t offset_z;
    size_t size_x;
    size_t size_y;
    size_t size_z;
    size_t padded_size[3];
    Vector3f spatial_offset;
    Vector3f spatial_extent;
    Vector3f pad_fractions;
    GLuint texture_id;
} Brick;

typedef struct BrickTreeNode
{
    BrickTreeNode* lower_child;
    BrickTreeNode* upper_child;
    Brick* brick;
    unsigned int split_axis;
    Vector3f spatial_offset;
    Vector3f spatial_extent;
    float visibility_ratio;
} BrickTreeNode;

typedef struct SubBrickTreeNode
{
    SubBrickTreeNode* lower_child;
    SubBrickTreeNode* upper_child;
    unsigned int split_axis;
    size_t offset_x;
    size_t offset_y;
    size_t offset_z;
    size_t size_x;
    size_t size_y;
    size_t size_z;
    Vector3f spatial_offset;
    Vector3f spatial_extent;
    float visibility_ratio;
} SubBrickTreeNode;

typedef struct BrickedField
{
    const Field* field;
    Brick* bricks;
    BrickTreeNode* tree;
    size_t n_bricks;
    size_t n_bricks_x;
    size_t n_bricks_y;
    size_t n_bricks_z;
    size_t brick_size;
    GLuint texture_unit;
} BrickedField;

void set_min_sub_brick_size(unsigned int size);

void create_bricked_field(BrickedField* bricked_field, const Field* field, unsigned int brick_size_exponent, unsigned int kernel_size);

void destroy_bricked_field(BrickedField* bricked_field);

#endif
