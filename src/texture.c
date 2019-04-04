#include "texture.h"

#include "error.h"
#include "linked_list.h"
#include "hash_map.h"

#include <stdio.h>
#include <string.h>


typedef struct ExtendedTexture
{
    Texture texture;
    Uniform uniform;
} ExtendedTexture;


static ExtendedTexture* get_texture(const char* name);
static GLuint next_unused_texture_unit(void);
static void load_texture(ExtendedTexture* extended_texture);
static void sync_texture(ExtendedTexture* extended_texture);


static HashMap textures;
static LinkedList deleted_units;
static GLuint next_undeleted_unused_unit;

static ShaderProgram* active_shader_program = NULL;


void set_active_shader_program_for_textures(ShaderProgram* shader_program)
{
    active_shader_program = shader_program;
}

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
    texture.name = create_string("texture_%d", unit);

    MapItem item = insert_new_map_item(&textures, texture.name.chars, sizeof(ExtendedTexture));

    ExtendedTexture* const extended_texture = (ExtendedTexture*)item.data;

    extended_texture->texture = texture;

    initialize_uniform(&extended_texture->uniform, texture.name.chars);

    return &extended_texture->texture;
}

void load_textures(void)
{
    check(active_shader_program);

    for (reset_map_iterator(&textures); valid_map_iterator(&textures); advance_map_iterator(&textures))
    {
        ExtendedTexture* const extended_texture = get_texture(get_current_map_key(&textures));
        load_texture(extended_texture);
    }
}

void delete_texture_data(Texture* texture)
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

void destroy_texture(Texture* texture)
{
    check(texture);

    delete_texture_data(texture);

    ListItem item = append_new_list_item(&deleted_units, sizeof(GLuint));
    GLuint* const unit = (GLuint*)item.data;
    *unit = texture->unit;

    ExtendedTexture* const extended_texture = get_texture(texture->name.chars);

    DynamicString texture_name_copy = create_duplicate_string(&texture->name);
    clear_string(&texture->name);
    destroy_uniform(&extended_texture->uniform);

    remove_map_item(&textures, texture_name_copy.chars);

    clear_string(&texture_name_copy);
}

void cleanup_textures(void)
{
    for (reset_map_iterator(&textures); valid_map_iterator(&textures); advance_map_iterator(&textures))
    {
        ExtendedTexture* const extended_texture = get_texture(get_current_map_key(&textures));
        delete_texture_data(&extended_texture->texture);
        clear_string(&extended_texture->texture.name);
        destroy_uniform(&extended_texture->uniform);
    }

    destroy_map(&textures);
    clear_list(&deleted_units);

    active_shader_program = NULL;
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

static void load_texture(ExtendedTexture* extended_texture)
{
    assert(active_shader_program);
    assert(extended_texture);

    load_uniform(active_shader_program, &extended_texture->uniform);

    sync_texture(extended_texture);
}

static void sync_texture(ExtendedTexture* extended_texture)
{
    assert(active_shader_program);
    assert(extended_texture);

    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program for updating field texture uniforms");

    glUniform1i(extended_texture->uniform.location, (GLint)extended_texture->texture.unit);
    abort_on_GL_error("Could not set texture uniform location");

    glUseProgram(0);
}
