#ifndef BRICKS_H
#define BRICKS_H

#include "fields.h"

#include <stddef.h>

enum brick_orientation {ORIENTED_XYZ = 0, ORIENTED_ZXY = 1, ORIENTED_YZX = 2};

typedef struct Brick
{
    float* data;
    float x_offset;
    float y_offset;
    float z_offset;
    float x_extent;
    float y_extent;
    float z_extent;
    enum brick_orientation orientation;
} Brick;

typedef struct BrickedField
{
    const Field* field;
    Brick* bricks;
    size_t n_bricks_x;
    size_t n_bricks_y;
    size_t n_bricks_z;
    size_t n_bricks;
    size_t brick_size;
    size_t padded_brick_size;
} BrickedField;

void create_bricked_field(BrickedField*, const Field* field, unsigned int brick_size_exponent, unsigned int kernel_size);
void destroy_bricked_field(BrickedField* bricked_field);

#endif
