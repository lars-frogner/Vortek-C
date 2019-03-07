#include "texture.h"

#include "error.h"
#include "linked_list.h"
#include "hash_map.h"

#include <stdio.h>
#include <string.h>


typedef struct ExtendedTexture
{
    Texture texture;
    GLint uniform_location;
} ExtendedTexture;


static ExtendedTexture* get_texture(const char* name);
static GLuint next_unused_texture_unit(void);
static void set_texture_uniform(ExtendedTexture* extended_texture, const ShaderProgram* shader_program);
static void unload_texture(Texture* texture);


static HashMap textures;
static LinkedList deleted_units;
static GLuint next_undeleted_unused_unit;


void initialize_textures(void)
{
    textures = create_map();
    deleted_units = create_list();
    next_undeleted_unused_unit = 0;
}

Texture* create_texture(void)
{
    Texture texture;

    const GLuint unit = next_unused_texture_unit();

    texture.unit = unit;
    texture.ids = create_list();
    texture.name = create_string();
    set_string(&texture.name, "texture_%d", unit);

    MapItem item = insert_new_map_item(&textures, texture.name.chars, sizeof(ExtendedTexture));

    ExtendedTexture* const extended_texture = (ExtendedTexture*)item.data;

    extended_texture->texture = texture;
    extended_texture->uniform_location = 0;

    return &extended_texture->texture;
}

void load_textures(const ShaderProgram* shader_program)
{
    check(shader_program);

    for (reset_map_iterator(&textures); valid_map_iterator(&textures); advance_map_iterator(&textures))
    {
        ExtendedTexture* const extended_texture = get_texture(get_current_map_key(&textures));
        set_texture_uniform(extended_texture, shader_program);
    }
}

void destroy_texture(Texture* texture)
{
    unload_texture(texture);

    ListItem item = append_new_list_item(&deleted_units, sizeof(GLuint));
    GLuint* const unit = (GLuint*)item.data;
    *unit = texture->unit;

    DynamicString texture_name_copy = create_duplicate_string(&texture->name);
    clear_string(&texture->name);
    remove_map_item(&textures, texture_name_copy.chars);
    clear_string(&texture_name_copy);
}

void cleanup_textures(void)
{
    for (reset_map_iterator(&textures); valid_map_iterator(&textures); advance_map_iterator(&textures))
    {
        ExtendedTexture* const extended_texture = get_texture(get_current_map_key(&textures));
        unload_texture(&extended_texture->texture);
        clear_string(&extended_texture->texture.name);
    }

    destroy_map(&textures);
    clear_list(&deleted_units);
}

static ExtendedTexture* get_texture(const char* name)
{
    check(name);

    MapItem item = get_map_item(&textures, name);
    ExtendedTexture* const extended_texture = (ExtendedTexture*)item.data;
    check(extended_texture);

    return extended_texture;
}

static GLuint next_unused_texture_unit(void)
{
    GLuint unit;

    if (deleted_units.length > 0)
    {
        const ListItem item = get_first_list_item(&deleted_units);
        unit = *((GLuint*)item.data);
        remove_first_list_item(&deleted_units);
    }
    else
    {
        unit = next_undeleted_unused_unit++;
    }

    return unit;
}

static void set_texture_uniform(ExtendedTexture* extended_texture, const ShaderProgram* shader_program)
{
    check(extended_texture);

    if (extended_texture->texture.name.length == 0)
        print_severe_message("Cannot set uniform for texture with no name.");

    extended_texture->uniform_location = glGetUniformLocation(shader_program->id, extended_texture->texture.name.chars);
    abort_on_GL_error("Could not get texture uniform location");

    if (extended_texture->uniform_location == -1)
    {
        print_warning_message("Texture \"%s\" not used in shader program", extended_texture->texture.name.chars);
        return;
    }

    glUseProgram(shader_program->id);
    abort_on_GL_error("Could not use shader program when updating texture uniform");
    glUniform1i(extended_texture->uniform_location, (GLint)extended_texture->texture.unit);
    abort_on_GL_error("Could not set texture uniform location");
    glUseProgram(0);
}

static void unload_texture(Texture* texture)
{
    check(texture);

    for (reset_list_iterator(&texture->ids); valid_list_iterator(&texture->ids); advance_list_iterator(&texture->ids))
    {
        ListItem item = get_current_list_item(&texture->ids);
        glDeleteTextures(1, (GLuint*)item.data);
        abort_on_GL_error("Could not destroy texture object");
    }

    clear_list(&texture->ids);
}
