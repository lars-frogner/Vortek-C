#include "transfer_functions.h"

#include "gl_includes.h"
#include "error.h"
#include "extra_math.h"
#include "hash_map.h"
#include "texture.h"

#include <stdio.h>
#include <math.h>
#include <string.h>


#define TRANSFER_FUNCTION_COMPONENTS 4
#define TRANSFER_FUNCTION_SIZE 256


enum transfer_function_type {PIECEWISE_LINEAR, LOGARITHMIC};

typedef struct TransferFunction
{
    float output[TRANSFER_FUNCTION_SIZE][TRANSFER_FUNCTION_COMPONENTS];
    int node_states[TRANSFER_FUNCTION_SIZE][TRANSFER_FUNCTION_COMPONENTS];
    enum transfer_function_type types[TRANSFER_FUNCTION_COMPONENTS];
} TransferFunction;

typedef struct TransferFunctionTexture
{
    TransferFunction transfer_function;
    Texture* texture;
    int needs_sync;
} TransferFunctionTexture;


static TransferFunctionTexture* get_transfer_function_texture(const char* name);

static void transfer_transfer_function_texture(TransferFunctionTexture* texture);

static void sync_transfer_function(TransferFunctionTexture* texture);

static void clear_transfer_function_texture(TransferFunctionTexture* texture);

static void reset_transfer_function_texture_data(TransferFunctionTexture* texture, unsigned int component);

static unsigned int texture_coordinate_to_node(float texture_coordinate);

static unsigned int find_closest_node_above(const TransferFunction* transfer_function, unsigned int component, unsigned int node);

static unsigned int find_closest_node_below(const TransferFunction* transfer_function, unsigned int component, unsigned int node);

static void set_piecewise_linear_transfer_function_data(TransferFunction* transfer_function, unsigned int component,
                                                        unsigned int start_node, unsigned int end_node,
                                                        float start_value, float end_value);

static void set_logarithmic_transfer_function_data(TransferFunction* transfer_function, unsigned int component,
                                                   unsigned int start_node, unsigned int end_node,
                                                   float start_value, float end_value);

static void compute_linear_array_segment(float* segment, size_t segment_length, unsigned int stride,
                                         float start_value, float end_value);

static void compute_logarithmic_array_segment(float* segment, size_t segment_length, unsigned int stride,
                                              float start_value, float end_value);


static HashMap transfer_function_textures;


void initialize_transfer_functions(void)
{
    transfer_function_textures = create_map();
}

void print_transfer_function(const char* name, enum transfer_function_component component)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);

    const float norm = 1.0f/(TRANSFER_FUNCTION_SIZE - 1);

    unsigned int i;
    for (i = 0; i < TRANSFER_FUNCTION_SIZE; i++)
        printf("%5.3f: %.3f\n", i*norm, transfer_function_texture->transfer_function.output[i][component]);
}

const char* create_transfer_function(ShaderProgram* shader_program)
{
    check(shader_program);

    Texture* const texture = create_texture();

    MapItem item = insert_new_map_item(&transfer_function_textures, texture->name.chars, sizeof(TransferFunctionTexture));
    TransferFunctionTexture* const transfer_function_texture = (TransferFunctionTexture*)item.data;

    unsigned int component;
    for (component = 0; component < TRANSFER_FUNCTION_COMPONENTS; component++)
        reset_transfer_function_texture_data(transfer_function_texture, component);

    transfer_function_texture->texture = texture;

    transfer_transfer_function_texture(transfer_function_texture);

    add_transfer_function_in_shader(&shader_program->fragment_shader_source, texture->name.chars);

    return texture->name.chars;
}

void sync_transfer_functions(void)
{
    for (reset_map_iterator(&transfer_function_textures); valid_map_iterator(&transfer_function_textures); advance_map_iterator(&transfer_function_textures))
    {
        TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(get_current_map_key(&transfer_function_textures));
        sync_transfer_function(transfer_function_texture);
    }
}

void remove_transfer_function(const char* name)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);

    check(transfer_function_texture->texture);
    Texture* const texture = transfer_function_texture->texture;

    remove_map_item(&transfer_function_textures, name);

    destroy_texture(texture);
}

void cleanup_transfer_functions(void)
{
    for (reset_map_iterator(&transfer_function_textures); valid_map_iterator(&transfer_function_textures); advance_map_iterator(&transfer_function_textures))
    {
        TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(get_current_map_key(&transfer_function_textures));
        clear_transfer_function_texture(transfer_function_texture);
    }

    destroy_map(&transfer_function_textures);
}

void add_piecewise_linear_transfer_function_node(const char* name, enum transfer_function_component component,
                                                 float texture_coordinate, float value)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    const unsigned int node = texture_coordinate_to_node(texture_coordinate);

    transfer_function->types[component] = PIECEWISE_LINEAR;
    transfer_function->output[node][component] = value;
    transfer_function->node_states[node][component] = 1;

    const unsigned int closest_node_below = find_closest_node_below(transfer_function, component, node);
    const unsigned int closest_node_above = find_closest_node_above(transfer_function, component, node);

    if (node - closest_node_below > 1)
        set_piecewise_linear_transfer_function_data(transfer_function, component, closest_node_below, node,
                                                    transfer_function->output[closest_node_below][component], value);

    if (closest_node_above - node > 1)
        set_piecewise_linear_transfer_function_data(transfer_function, component, node, closest_node_above,
                                                    value, transfer_function->output[closest_node_above][component]);

    transfer_function_texture->needs_sync = 1;
}

void remove_piecewise_linear_transfer_function_node(const char* name, enum transfer_function_component component,
                                                    float texture_coordinate)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    if (transfer_function->types[component] != PIECEWISE_LINEAR)
    {
        print_warning_message("Cannot remove node from transfer function that is not piecewise linear.");
        return;
    }

    const unsigned int node = texture_coordinate_to_node(texture_coordinate);

    if (!transfer_function->node_states[node][component] || node == 0 || node == TRANSFER_FUNCTION_SIZE - 1)
        return;

    transfer_function->node_states[node][component] = 0;

    const unsigned int closest_node_above = find_closest_node_above(transfer_function, component, node);
    const unsigned int closest_node_below = find_closest_node_below(transfer_function, component, node);

    set_piecewise_linear_transfer_function_data(transfer_function, component, closest_node_below, closest_node_above,
                                                transfer_function->output[closest_node_below][component],
                                                transfer_function->output[closest_node_above][component]);

    transfer_function_texture->needs_sync = 1;
}

void set_logarithmic_transfer_function(const char* name, enum transfer_function_component component,
                                       float start_texture_coordinate, float end_texture_coordinate,
                                       float start_value, float end_value)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    if (end_value <= start_value)
    {
        print_warning_message("Cannot create logarithmic transfer function between non-ascending values.");
        return;
    }

    const unsigned int start_node = texture_coordinate_to_node(start_texture_coordinate);
    const unsigned int end_node = texture_coordinate_to_node(end_texture_coordinate);

    if (end_node - start_node < 1)
        return;

    transfer_function->types[component] = LOGARITHMIC;

    transfer_function->node_states[start_node][component] = 1;
    transfer_function->node_states[end_node][component] = 1;

    transfer_function->output[0][component] = 0;
    transfer_function->output[TRANSFER_FUNCTION_SIZE - 1][component] = 0;

    transfer_function->output[start_node][component] = start_value;
    transfer_function->output[end_node][component] = end_value;

    unsigned int i;

    for (i = 1; i < start_node; i++)
    {
        transfer_function->node_states[i][component] = 0;
        transfer_function->output[i][component] = 0;
    }

    for (i = start_node + 1; i < end_node; i++)
        transfer_function->node_states[i][component] = 0;

    for (i = end_node + 1; i < TRANSFER_FUNCTION_SIZE - 1; i++)
    {
        transfer_function->node_states[i][component] = 0;
        transfer_function->output[i][component] = 0;
    }

    transfer_function_texture->needs_sync = 1;

    if (end_node - start_node < 2)
        return;

    set_logarithmic_transfer_function_data(transfer_function, component, start_node, end_node, start_value, end_value);
}

static TransferFunctionTexture* get_transfer_function_texture(const char* name)
{
    check(name);

    MapItem item = get_map_item(&transfer_function_textures, name);
    assert(item.size == sizeof(TransferFunctionTexture));
    TransferFunctionTexture* const transfer_function_texture = (TransferFunctionTexture*)item.data;
    check(transfer_function_texture);

    return transfer_function_texture;
}

static void transfer_transfer_function_texture(TransferFunctionTexture* transfer_function_texture)
{
    check(transfer_function_texture);
    check(transfer_function_texture->texture);

    ListItem item = append_new_list_item(&transfer_function_texture->texture->ids, sizeof(GLuint));
    GLuint* const id = (GLuint*)item.data;

    glGenTextures(1, id);
    abort_on_GL_error("Could not generate texture object for transfer function");

    glActiveTexture(GL_TEXTURE0 + transfer_function_texture->texture->unit);
    abort_on_GL_error("Could not set active texture unit for transfer function");

    glBindTexture(GL_TEXTURE_1D, *id);
    abort_on_GL_error("Could not bind 1D texture for transfer function");

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glTexImage1D(GL_TEXTURE_1D,
                 0,
                 GL_RGBA4,
                 (GLsizei)TRANSFER_FUNCTION_SIZE,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 (GLvoid*)transfer_function_texture->transfer_function.output);
    abort_on_GL_error("Could not define 1D texture image for transfer function");

    transfer_function_texture->needs_sync = 0;
}

static void sync_transfer_function(TransferFunctionTexture* transfer_function_texture)
{
    if (transfer_function_texture->needs_sync)
    {
        glActiveTexture(GL_TEXTURE0 + transfer_function_texture->texture->unit);
        abort_on_GL_error("Could not set active texture unit for transfer function");

        glTexSubImage1D(GL_TEXTURE_1D,
                        0,
                        0,
                        (GLsizei)TRANSFER_FUNCTION_SIZE,
                        GL_RGBA,
                        GL_FLOAT,
                        (GLvoid*)transfer_function_texture->transfer_function.output);
        abort_on_GL_error("Could not sync transfer function texture data");

        transfer_function_texture->needs_sync = 0;
    }
}

static void clear_transfer_function_texture(TransferFunctionTexture* transfer_function_texture)
{
    assert(transfer_function_texture);
    assert(transfer_function_texture->texture);

    destroy_texture(transfer_function_texture->texture);

    transfer_function_texture->needs_sync = 0;
    transfer_function_texture->texture = NULL;
}

static void reset_transfer_function_texture_data(TransferFunctionTexture* transfer_function_texture, unsigned int component)
{
    check(transfer_function_texture);
    check(component < TRANSFER_FUNCTION_COMPONENTS);

    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    transfer_function->types[component] = PIECEWISE_LINEAR;

    transfer_function->node_states[0][component] = 1;
    transfer_function->node_states[TRANSFER_FUNCTION_SIZE - 1][component] = 1;

    unsigned int i;
    for (i = 0; i < TRANSFER_FUNCTION_SIZE; i++)
        transfer_function->node_states[i][component] = 0;

    const float start_value = (component == 3) ? 1.0f : 0.0f;
    const float end_value = 1.0f;

    compute_linear_array_segment(transfer_function->output[0] + component,
                                 TRANSFER_FUNCTION_SIZE,
                                 TRANSFER_FUNCTION_COMPONENTS,
                                 start_value, end_value);

    transfer_function_texture->needs_sync = 1;
}

static unsigned int texture_coordinate_to_node(float texture_coordinate)
{
    const float clamped_coordinate = clamp(texture_coordinate, 0, 1);
    return (unsigned int)(clamped_coordinate*(TRANSFER_FUNCTION_SIZE - 1) + 0.5f);
}

static unsigned int find_closest_node_above(const TransferFunction* transfer_function, unsigned int component, unsigned int node)
{
    check(transfer_function);
    unsigned int i;
    unsigned int closest_node_above = TRANSFER_FUNCTION_SIZE - 1;

    for (i = node + 1; i < closest_node_above; i++)
    {
        if (transfer_function->node_states[i][component])
        {
            closest_node_above = i;
            break;
        }
    }

    return closest_node_above;
}

static unsigned int find_closest_node_below(const TransferFunction* transfer_function, unsigned int component, unsigned int node)
{
    check(transfer_function);
    int i;
    unsigned int closest_node_below = 0;

    for (i = (int)node - 1; i > 0; i--)
    {
        if (transfer_function->node_states[i][component])
        {
            closest_node_below = (unsigned int)i;
            break;
        }
    }

    return closest_node_below;
}

static void set_piecewise_linear_transfer_function_data(TransferFunction* transfer_function, unsigned int component,
                                                        unsigned int start_node, unsigned int end_node,
                                                        float start_value, float end_value)
{
    check(transfer_function);
    assert(component < TRANSFER_FUNCTION_COMPONENTS);
    assert(end_node > start_node);
    assert(end_node < TRANSFER_FUNCTION_SIZE);

    compute_linear_array_segment(transfer_function->output[start_node] + component,
                                 end_node - start_node + 1,
                                 TRANSFER_FUNCTION_COMPONENTS,
                                 start_value, end_value);
}

static void set_logarithmic_transfer_function_data(TransferFunction* transfer_function, unsigned int component,
                                                   unsigned int start_node, unsigned int end_node,
                                                   float start_value, float end_value)
{
    check(transfer_function);
    assert(component < TRANSFER_FUNCTION_COMPONENTS);
    assert(end_node > start_node);
    assert(end_node < TRANSFER_FUNCTION_SIZE);

    compute_logarithmic_array_segment(transfer_function->output[start_node] + component,
                                      end_node - start_node + 1,
                                      TRANSFER_FUNCTION_COMPONENTS,
                                      start_value, end_value);
}

static void compute_linear_array_segment(float* segment, size_t segment_length, unsigned int stride,
                                         float start_value, float end_value)
{
    assert(segment);
    assert(segment_length > 1);

    size_t i;
    const float scale = (end_value - start_value)/(segment_length - 1);

    for (i = 0; i < segment_length; i++)
        segment[i*stride] = start_value + i*scale;
}

static void compute_logarithmic_array_segment(float* segment, size_t segment_length, unsigned int stride,
                                              float start_value, float end_value)
{
    assert(segment);
    assert(segment_length > 1);
    assert(end_value > start_value);

    size_t i;
    const float offset = powf(10, start_value);
    const float scale = (powf(10, end_value) - offset)/(segment_length - 1);

    for (i = 0; i < segment_length; i++)
        segment[i*stride] = log10f(i*scale + offset);
}
