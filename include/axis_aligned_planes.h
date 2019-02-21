#ifndef AXIS_ALIGNED_PLANES_H
#define AXIS_ALIGNED_PLANES_H

#include <stddef.h>

void create_planes(size_t size_x, size_t size_y, size_t size_z,
                   float extent_x, float extent_y, float extent_z,
                   float spacing_multiplier);
void load_planes(void);
void sync_planes(void);
void draw_planes(void);
void cleanup_planes(void);

#endif
