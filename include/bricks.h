#ifndef BRICKS_H
#define BRICKS_H

#include "gl_includes.h"
#include "geometry.h"
#include "fields.h"
#include "indicators.h"

#include <stddef.h>

enum brick_orientation {ORIENTED_ZYX = 0, ORIENTED_XZY = 1, ORIENTED_YXZ = 2};
enum region_visibility {REGION_VISIBLE, REGION_INVISIBLE, REGION_CLIPPED, UNDETERMINED_REGION_VISIBILITY};

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
    size_t n_children;
    Vector3f spatial_offset;
    Vector3f spatial_extent;
    float visibility_ratio;
    enum region_visibility visibility;
} BrickTreeNode;

typedef struct SubBrickTreeNode
{
    SubBrickTreeNode* lower_child;
    SubBrickTreeNode* upper_child;
    unsigned int split_axis;
    size_t n_children;
    size_t offset_x;
    size_t offset_y;
    size_t offset_z;
    size_t size_x;
    size_t size_y;
    size_t size_z;
    Vector3f spatial_offset;
    Vector3f spatial_extent;
    float visibility_ratio;
    enum region_visibility visibility;
    size_t indicator_idx;
} SubBrickTreeNode;

typedef struct BrickedField
{
    Field* field;
    Brick* bricks;
    BrickTreeNode* tree;
    size_t n_bricks;
    size_t n_bricks_x;
    size_t n_bricks_y;
    size_t n_bricks_z;
    size_t brick_size;
    GLuint texture_unit;
    const char* field_boundary_indicator_name;
    const char* brick_boundary_indicator_name;
    const char* sub_brick_boundary_indicator_name;
} BrickedField;

void initialize_bricks(void);

void reset_bricked_field(BrickedField* bricked_field);

void set_brick_size_exponent(unsigned int brick_size_exponent);
void set_bricked_field_kernel_size(unsigned int kernel_size);
void set_min_sub_brick_size(unsigned int min_sub_brick_size);

void set_field_boundary_indicator_creation(int state);
void set_brick_boundary_indicator_creation(int state);
void set_sub_brick_boundary_indicator_creation(int state);

void create_bricked_field(BrickedField* bricked_field, Field* field);

void draw_field_boundary_indicator(const BrickedField* bricked_field, unsigned int reference_corner_idx, enum indicator_drawing_pass pass);
void draw_brick_boundary_indicator(const BrickedField* bricked_field);
void draw_sub_brick_boundary_indicator(const BrickedField* bricked_field);

void destroy_bricked_field(BrickedField* bricked_field);

#endif
