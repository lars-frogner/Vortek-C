#ifndef FIELD_TEXTURES_H
#define FIELD_TEXTURES_H

#include "fields.h"

void initialize_field_textures(void);
const char* add_scalar_field_texture(const Field* field);
void destroy_field_texture(const char* name);
void cleanup_field_textures(void);

#endif
