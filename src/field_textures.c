#include "field_textures.h"

#include "gl_includes.h"
#include "error.h"
#include "hash_map.h"
#include "texture.h"
#include "bricks.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef struct FieldTexture
{
    BrickedField bricked_field;
    Texture* texture;
} FieldTexture;


static FieldTexture* get_field_texture(const char* name);
static void transfer_scalar_field_texture(FieldTexture* field_texture);
static void clear_field_texture(FieldTexture* field_texture);


static HashMap field_textures;


void initialize_field_textures(void)
{
    field_textures = create_map();
}

const char* create_scalar_field_texture(const Field* field, ShaderProgram* shader_program)
{
    check(field);
    check(shader_program);

    Texture* const texture = create_texture();

    MapItem item = insert_new_map_item(&field_textures, texture->name.chars, sizeof(FieldTexture));
    FieldTexture* const field_texture = (FieldTexture*)item.data;

    create_bricked_field(&field_texture->bricked_field, field, 8, 1);
    field_texture->texture = texture;

    transfer_scalar_field_texture(field_texture);

    add_field_texture_in_shader(&shader_program->fragment_shader_source, texture->name.chars);

    return texture->name.chars;
}

void destroy_field_texture(const char* name)
{
    FieldTexture* const field_texture = get_field_texture(name);

    check(field_texture->texture);
    Texture* const texture = field_texture->texture;

    destroy_bricked_field(&field_texture->bricked_field);
    remove_map_item(&field_textures, name);

    destroy_texture(texture);
}

void cleanup_field_textures(void)
{
    for (reset_map_iterator(&field_textures); valid_map_iterator(&field_textures); advance_map_iterator(&field_textures))
    {
        FieldTexture* const field_texture = get_field_texture(get_current_map_key(&field_textures));
        clear_field_texture(field_texture);
    }

    destroy_map(&field_textures);
}

static FieldTexture* get_field_texture(const char* name)
{
    check(name);

    MapItem item = get_map_item(&field_textures, name);
    assert(item.size == sizeof(FieldTexture));
    FieldTexture* const field_texture = (FieldTexture*)item.data;
    check(field_texture);

    return field_texture;
}

static void transfer_scalar_field_texture(FieldTexture* field_texture)
{
    check(field_texture);
    check(field_texture->texture);

    BrickedField* const bricked_field = &field_texture->bricked_field;
    const Field* const field = bricked_field->field;
    check(field);

    if (field->type != SCALAR_FIELD)
        print_severe_message("Cannot create scalar texture from non-scalar field type.");

    if (!field->data)
        print_severe_message("Cannot create texture with NULL data pointer.");

    if (field->size_x == 0 || field->size_y == 0 || field->size_z == 0)
        print_severe_message("Cannot create texture with size 0 along any dimension.");

    if (field->size_x > GL_MAX_3D_TEXTURE_SIZE ||
        field->size_y > GL_MAX_3D_TEXTURE_SIZE ||
        field->size_z > GL_MAX_3D_TEXTURE_SIZE)
        print_severe_message("Cannot create texture with size exceeding %d along any dimension.", GL_MAX_3D_TEXTURE_SIZE);

    glActiveTexture(GL_TEXTURE0 + field_texture->texture->unit);
    abort_on_GL_error("Could not set active texture unit");

    ListItem item = append_new_list_item(&field_texture->texture->ids, sizeof(GLuint));
    GLuint* const id = (GLuint*)item.data;

    glGenTextures(1, id);
    abort_on_GL_error("Could not generate texture object");

    glBindTexture(GL_TEXTURE_3D, *id);
    abort_on_GL_error("Could not bind 3D texture");

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    /*glTexImage3D(GL_TEXTURE_3D,
                 0,
                 GL_COMPRESSED_RED,
                 (GLsizei)field->size_x,
                 (GLsizei)field->size_y,
                 (GLsizei)field->size_z,
                 0,
                 GL_RED,
                 GL_FLOAT,
                 (GLvoid*)field->data);*/
    glTexImage3D(GL_TEXTURE_3D,
                 0,
                 GL_COMPRESSED_RED,
                 (GLsizei)(bricked_field->n_bricks_x*bricked_field->padded_brick_size),
                 (GLsizei)(bricked_field->n_bricks_y*bricked_field->padded_brick_size),
                 (GLsizei)(bricked_field->n_bricks_z*bricked_field->padded_brick_size),
                 0,
                 GL_RED,
                 GL_FLOAT,
                 (GLvoid*)bricked_field->bricks->data);
    abort_on_GL_error("Could not define 3D texture image");

    GLint is_compressed;
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_COMPRESSED, &is_compressed);
    abort_on_GL_error("Could not determine compression state of 3D texture");
    if (is_compressed == GL_FALSE)
        print_warning_message("Could not compress 3D texture.");

    glGenerateMipmap(GL_TEXTURE_3D);
    abort_on_GL_error("Could not generate mipmap for 3D texture");
}

static void clear_field_texture(FieldTexture* field_texture)
{
    assert(field_texture);
    assert(field_texture->texture);

    destroy_bricked_field(&field_texture->bricked_field);
    destroy_texture(field_texture->texture);
    field_texture->texture = NULL;
}
