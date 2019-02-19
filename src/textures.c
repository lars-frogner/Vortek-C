#include "textures.h"

#include "gl_includes.h"
#include "error.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define MAX_TEXTURES 2
#define MAX_TEXTURE_NAME_LENGTH 12


typedef struct FieldTexture
{
    Field field;
    GLuint id;
    GLuint unit;
    GLint uniform_location;
    char name[MAX_TEXTURE_NAME_LENGTH];
} FieldTexture;

typedef struct FieldTextureSet
{
    FieldTexture textures[MAX_TEXTURES];
    unsigned int n_textures;
} FieldTextureSet;


static void set_field_texture_uniforms(const ShaderProgram* shader_program);
static void destroy_field_texture_set(void);
static void create_scalar_field_texture(FieldTexture* texture);
static void set_field_texture_uniform(FieldTexture* texture, const ShaderProgram* shader_program);
static void destroy_field_texture(FieldTexture* texture);


static FieldTextureSet texture_set;


void add_scalar_field_texture(const Field* field)
{
    check(field);

    const unsigned int texture_idx = texture_set.n_textures;

    if (texture_idx >= MAX_TEXTURES)
        print_severe_message("Cannot exceed max limit of %d textures.", MAX_TEXTURES);

    FieldTexture* const texture = texture_set.textures + texture_idx;
    texture->field = *field;
    texture->unit = texture_idx;
    texture->uniform_location = 0;
    sprintf(texture->name, "texture%d", texture_idx);

    create_scalar_field_texture(texture);
    texture_set.n_textures++;
}

void load_textures(const ShaderProgram* shader_program)
{
    check(shader_program);
    set_field_texture_uniforms(shader_program);
}

void bind_textures(void)
{
    unsigned int i;
    for (i = 0; i < texture_set.n_textures; i++)
    {
        glActiveTexture(GL_TEXTURE0 + texture_set.textures[i].unit);
        abort_on_GL_error("Could not activate 3D texture for drawing");

        glBindTexture(GL_TEXTURE_3D, texture_set.textures[i].id);
        abort_on_GL_error("Could not bind 3D texture for drawing");
    }
}

void cleanup_textures(void)
{
    destroy_field_texture_set();
}

static void set_field_texture_uniforms(const ShaderProgram* shader_program)
{
    unsigned int i;
    for (i = 0; i < texture_set.n_textures; i++)
        set_field_texture_uniform(texture_set.textures + i, shader_program);
}

static void destroy_field_texture_set(void)
{
    unsigned int i;
    for (i = 0; i < texture_set.n_textures; i++)
        destroy_field_texture(texture_set.textures + i);
}

static void create_scalar_field_texture(FieldTexture* texture)
{
    check(texture);

    if (strlen(texture->name) == 0)
        print_severe_message("Cannot create texture with no name.");

    if (texture->field.type != SCALAR_FIELD)
        print_severe_message("Cannot create scalar texture from non-scalar field type.");

    if (!texture->field.data)
        print_severe_message("Cannot create texture with NULL data pointer.");

    if (texture->field.size_x == 0 || texture->field.size_y == 0 || texture->field.size_z == 0)
        print_severe_message("Cannot create texture with size 0 along any dimension.");

    if (texture->field.size_x > GL_MAX_3D_TEXTURE_SIZE ||
        texture->field.size_y > GL_MAX_3D_TEXTURE_SIZE ||
        texture->field.size_z > GL_MAX_3D_TEXTURE_SIZE)
        print_severe_message("Cannot create texture with size exceeding %d along any dimension.", GL_MAX_3D_TEXTURE_SIZE);

    glGenTextures(1, &texture->id);
    abort_on_GL_error("Could not generate texture object");

    glActiveTexture(GL_TEXTURE0 + texture->unit);
    abort_on_GL_error("Could not set active texture unit");

    glBindTexture(GL_TEXTURE_3D, texture->id);
    abort_on_GL_error("Could not bind 3D texture");

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage3D(GL_TEXTURE_3D,
                 0,
                 GL_COMPRESSED_RED,
                 (GLsizei)texture->field.size_x,
                 (GLsizei)texture->field.size_y,
                 (GLsizei)texture->field.size_z,
                 0,
                 GL_RED,
                 GL_FLOAT,
                 (GLvoid*)texture->field.data);
    abort_on_GL_error("Could not define 3D texture image");

    GLint is_compressed;
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_COMPRESSED, &is_compressed);
    abort_on_GL_error("Could not determine compression state of 3D texture");
    if (is_compressed == GL_FALSE)
        print_warning_message("Could not compress 3D texture.");

    glGenerateMipmap(GL_TEXTURE_3D);
    abort_on_GL_error("Could not generate mipmap for 3D texture");

    free(texture->field.data);
    texture->field.data = NULL;
}

static void set_field_texture_uniform(FieldTexture* texture, const ShaderProgram* shader_program)
{
    texture->uniform_location = glGetUniformLocation(shader_program->id, texture->name);
    abort_on_GL_error("Could not get texture uniform location");

    glUseProgram(shader_program->id);
    abort_on_GL_error("Could not use shader program when updating texture uniform");
    glUniform1i(texture->uniform_location, (GLint)texture->unit);
    abort_on_GL_error("Could not get texture uniform location");
    glUseProgram(0);
}

static void destroy_field_texture(FieldTexture* texture)
{
    check(texture);

    if (texture->id == 0)
    {
        print_warning_message("Cannot destroy texture before it is created.");
        return;
    }

    glDeleteTextures(1, &texture->id);
    abort_on_GL_error("Could not destroy texture object");

    reset_field(&texture->field);
    texture->unit = 0;
    texture->uniform_location = 0;
    texture->name[0] = '\0';
}
