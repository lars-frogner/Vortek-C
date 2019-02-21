#include "field_textures.h"

#include "gl_includes.h"
#include "error.h"
#include "texture.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef struct FieldTexture
{
    Field field;
    Texture* texture;
} FieldTexture;

typedef struct FieldTextureSet
{
    FieldTexture textures[MAX_TEXTURES];
    unsigned int n_textures;
} FieldTextureSet;


static void destroy_field_texture_set(void);
static void transfer_scalar_field_texture(FieldTexture* field_texture);
static void destroy_field_texture(FieldTexture* field_texture);


static FieldTextureSet texture_set = {0};


unsigned int add_scalar_field_texture(const Field* field)
{
    check(field);

    const unsigned int handle = texture_set.n_textures;

    FieldTexture* const field_texture = texture_set.textures + handle;

    field_texture->field = *field;
    field_texture->texture = create_texture("texture_%d", handle);

    transfer_scalar_field_texture(field_texture);
    texture_set.n_textures++;

    return handle;
}

void cleanup_field_textures(void)
{
    destroy_field_texture_set();
}

static void destroy_field_texture_set(void)
{
    unsigned int i;
    for (i = 0; i < texture_set.n_textures; i++)
        destroy_field_texture(texture_set.textures + i);
}

static void transfer_scalar_field_texture(FieldTexture* field_texture)
{
    check(field_texture);
    check(field_texture->texture);

    if (field_texture->field.type != SCALAR_FIELD)
        print_severe_message("Cannot create scalar texture from non-scalar field type.");

    if (!field_texture->field.data)
        print_severe_message("Cannot create texture with NULL data pointer.");

    if (field_texture->field.size_x == 0 || field_texture->field.size_y == 0 || field_texture->field.size_z == 0)
        print_severe_message("Cannot create texture with size 0 along any dimension.");

    if (field_texture->field.size_x > GL_MAX_3D_TEXTURE_SIZE ||
        field_texture->field.size_y > GL_MAX_3D_TEXTURE_SIZE ||
        field_texture->field.size_z > GL_MAX_3D_TEXTURE_SIZE)
        print_severe_message("Cannot create texture with size exceeding %d along any dimension.", GL_MAX_3D_TEXTURE_SIZE);

    glGenTextures(1, &field_texture->texture->id);
    abort_on_GL_error("Could not generate texture object");

    glActiveTexture(GL_TEXTURE0 + field_texture->texture->unit);
    abort_on_GL_error("Could not set active texture unit");

    glBindTexture(GL_TEXTURE_3D, field_texture->texture->id);
    abort_on_GL_error("Could not bind 3D texture");

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glTexImage3D(GL_TEXTURE_3D,
                 0,
                 GL_COMPRESSED_RED,
                 (GLsizei)field_texture->field.size_x,
                 (GLsizei)field_texture->field.size_y,
                 (GLsizei)field_texture->field.size_z,
                 0,
                 GL_RED,
                 GL_FLOAT,
                 (GLvoid*)field_texture->field.data);
    abort_on_GL_error("Could not define 3D texture image");

    free(field_texture->field.data);
    field_texture->field.data = NULL;

    GLint is_compressed;
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_COMPRESSED, &is_compressed);
    abort_on_GL_error("Could not determine compression state of 3D texture");
    if (is_compressed == GL_FALSE)
        print_warning_message("Could not compress 3D texture.");

    glGenerateMipmap(GL_TEXTURE_3D);
    abort_on_GL_error("Could not generate mipmap for 3D texture");
}

static void destroy_field_texture(FieldTexture* field_texture)
{
    check(field_texture);
    check(field_texture->texture);

    destroy_texture(field_texture->texture->handle);
    reset_field(&field_texture->field);
}
