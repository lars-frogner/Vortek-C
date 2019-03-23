#ifndef CLIP_PLANES_H
#define CLIP_PLANES_H

#include "geometry.h"
#include "shaders.h"

void set_active_shader_program_for_clip_planes(ShaderProgram* shader_program);

void initialize_clip_planes(void);

void load_clip_planes(void);

void reset_clip_planes(void);

int axis_aligned_box_in_clipped_region(const Vector3f* offset, const Vector3f* extent);

void cleanup_clip_planes(void);

#endif
