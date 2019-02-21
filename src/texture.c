#include "texture.h"

#include "error.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>


#define MAX_TEXTURE_NAME_LENGTH 25


typedef struct ExtendedTexture
{
    Texture texture;
    char name[MAX_TEXTURE_NAME_LENGTH];
    GLint uniform_location;
    int is_active;
} ExtendedTexture;


static unsigned int activate_next_texture(void);
static void set_texture_uniform(unsigned int handle, const ShaderProgram* shader_program);


static ExtendedTexture textures[MAX_TEXTURES] = {0};


Texture* create_texture(const char* name, ...)
{
    check(name);

    const unsigned int handle = activate_next_texture();

    ExtendedTexture* const extended_texture = textures + handle;
    Texture* const texture = &extended_texture->texture;

    texture->unit = handle;
    texture->id = 0;

    check(strlen(name) + 1 < MAX_TEXTURE_NAME_LENGTH);

    va_list args;
    va_start(args, name);
    vsprintf(extended_texture->name, name, args);
    va_end(args);

    extended_texture->uniform_location = 0;
    texture->handle = handle;

    return texture;
}

void load_textures(const ShaderProgram* shader_program)
{
    check(shader_program);

    unsigned int handle;
    for (handle = 0; handle < MAX_TEXTURES; handle++)
        if (textures[handle].is_active)
            set_texture_uniform(handle, shader_program);
}

void destroy_texture(unsigned int handle)
{
    check(handle < MAX_TEXTURES);

    ExtendedTexture* const extended_texture = textures + handle;
    Texture* const texture = &extended_texture->texture;

    if (!extended_texture->is_active)
    {
        print_warning_message("Cannot destroy texture before it is created.");
        return;
    }

    glDeleteTextures(1, &texture->id);
    abort_on_GL_error("Could not destroy texture object");

    texture->unit = 0;
    extended_texture->name[0] = '\0';
    extended_texture->uniform_location = 0;
    extended_texture->is_active = 0;
}

static unsigned int activate_next_texture(void)
{
    unsigned int first_inactive_handle = MAX_TEXTURES;
    unsigned int handle;
    for (handle = 0; handle < MAX_TEXTURES; handle++)
    {
        if (!textures[handle].is_active)
        {
            first_inactive_handle = handle;
            textures[handle].is_active = 1;
            break;
        }
    }

    if (first_inactive_handle == MAX_TEXTURES)
        print_severe_message("Cannot exceed max limit of %d active textures.", MAX_TEXTURES);

    return first_inactive_handle;
}

static void set_texture_uniform(unsigned int handle, const ShaderProgram* shader_program)
{
    check(handle < MAX_TEXTURES);

    ExtendedTexture* const extended_texture = textures + handle;

    if (strlen(extended_texture->name) == 0)
        print_severe_message("Cannot set uniform for texture with no name.");

    extended_texture->uniform_location = glGetUniformLocation(shader_program->id, extended_texture->name);
    abort_on_GL_error("Could not get texture uniform location");

    if (extended_texture->uniform_location == -1)
    {
        print_warning_message("Texture \"%s\" not used in shader program", extended_texture->name);
        return;
    }

    print_info_message("Name: %s, Uniform location: %d, Unit: %d, ID: %d", extended_texture->name, extended_texture->uniform_location, extended_texture->texture.unit, extended_texture->texture.id);

    glUseProgram(shader_program->id);
    abort_on_GL_error("Could not use shader program when updating texture uniform");
    glUniform1i(extended_texture->uniform_location, (GLint)extended_texture->texture.unit);
    abort_on_GL_error("Could not get texture uniform location");
    glUseProgram(0);
}
