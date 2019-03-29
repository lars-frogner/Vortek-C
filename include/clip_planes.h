#ifndef CLIP_PLANES_H
#define CLIP_PLANES_H

#include "geometry.h"
#include "shaders.h"

enum clip_plane_state {CLIP_PLANE_DISABLED = 0, CLIP_PLANE_ENABLED = 1};

void set_active_shader_program_for_clip_planes(ShaderProgram* shader_program);

void initialize_clip_planes(void);

void load_clip_planes(void);

void set_max_clip_plane_origin_shifts(float max_x, float max_y, float max_z);

void set_clip_plane_state(unsigned int plane_idx, enum clip_plane_state state);
void toggle_clip_plane_enabled_state(unsigned int plane_idx);

void set_controllable_clip_plane(unsigned int idx);
void enable_clip_plane_control(void);
void disable_clip_plane_control(void);

void clip_plane_control_drag_start_callback(double screen_coord_x, double screen_coord_y);
void clip_plane_control_drag_callback(double screen_coord_x, double screen_coord_y);
void clip_plane_control_drag_end_callback(void);
void clip_plane_control_scroll_callback(double scroll_rate);
void clip_plane_control_flip_callback(void);

void draw_clip_planes(void);

void reset_clip_plane(unsigned int plane_idx);

int axis_aligned_box_in_clipped_region(const Vector3f* offset, const Vector3f* extent);

void cleanup_clip_planes(void);

#endif
