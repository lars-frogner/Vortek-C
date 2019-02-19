#ifndef AXIS_ALIGNED_PLANES_H
#define AXIS_ALIGNED_PLANES_H

#include "fields.h"

void create_planes(const Field* reference_field, float spacing_multiplier);
void load_planes(void);
void sync_planes(void);
void draw_planes(void);
void cleanup_planes(void);

#endif
