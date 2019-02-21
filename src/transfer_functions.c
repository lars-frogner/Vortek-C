#include "transfer_functions.h"

#include "gl_includes.h"
#include "error.h"
#include "extra_math.h"
#include "texture.h"

#include <stdio.h>
#include <math.h>
#include <string.h>


#define MAX_TRANSFER_FUNCTIONS 2
#define TRANSFER_FUNCTION_COMPONENTS 4
#define TRANSFER_FUNCTION_SIZE 256
#define MAX_TEXTURE_NAME_LENGTH 22


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
    int is_active;
} TransferFunctionTexture;


static unsigned int activate_next_transfer_function_texture(void);

static void transfer_transfer_function_texture(TransferFunctionTexture* texture);

static void sync_transfer_function(TransferFunctionTexture* texture);

static void destroy_transfer_function_texture(TransferFunctionTexture* texture);

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


static TransferFunctionTexture textures[MAX_TRANSFER_FUNCTIONS] = {0};


void print_transfer_function(unsigned int id, enum transfer_function_component component)
{
    check(id < MAX_TRANSFER_FUNCTIONS);

    const float norm = 1.0f/(TRANSFER_FUNCTION_SIZE - 1);

    unsigned int i;
    for (i = 0; i < TRANSFER_FUNCTION_SIZE; i++)
        printf("%5.3f: %.3f\n", i*norm, textures[id].transfer_function.output[i][component]);
}

unsigned int add_transfer_function(void)
{
    const unsigned int id = activate_next_transfer_function_texture();

    TransferFunctionTexture* const transfer_function_texture = textures + id;

    unsigned int component;
    for (component = 0; component < TRANSFER_FUNCTION_COMPONENTS; component++)
        reset_transfer_function_texture_data(transfer_function_texture, component);

    transfer_function_texture->texture = create_texture("transfer_function_%d", id);

    transfer_transfer_function_texture(transfer_function_texture);

    return id;
}

void sync_transfer_functions(void)
{
    unsigned int id;
    for (id = 0; id < MAX_TRANSFER_FUNCTIONS; id++)
        if (textures[id].is_active)
            sync_transfer_function(textures + id);
}

void remove_transfer_function(unsigned int id)
{
    check(id < MAX_TRANSFER_FUNCTIONS);
    destroy_transfer_function_texture(textures + id);
}

void cleanup_transfer_functions(void)
{
    unsigned int id;
    for (id = 0; id < MAX_TRANSFER_FUNCTIONS; id++)
        if (textures[id].is_active)
            remove_transfer_function(id);
}

void add_piecewise_linear_transfer_function_node(unsigned int id, enum transfer_function_component component,
                                                 float texture_coordinate, float value)
{
    check(id < MAX_TRANSFER_FUNCTIONS);

    TransferFunctionTexture* const transfer_function_texture = textures + id;
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    if (!transfer_function_texture->is_active)
    {
        print_warning_message("Cannot modify inactive transfer function.");
        return;
    }

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

void remove_piecewise_linear_transfer_function_node(unsigned int id, enum transfer_function_component component,
                                                    float texture_coordinate)
{
    check(id < MAX_TRANSFER_FUNCTIONS);

    TransferFunctionTexture* const transfer_function_texture = textures + id;
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    if (!transfer_function_texture->is_active)
    {
        print_warning_message("Cannot modify inactive transfer function.");
        return;
    }

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

void set_logarithmic_transfer_function(unsigned int id, enum transfer_function_component component,
                                       float start_texture_coordinate, float end_texture_coordinate,
                                       float start_value, float end_value)
{
    check(id < MAX_TRANSFER_FUNCTIONS);

    TransferFunctionTexture* const transfer_function_texture = textures + id;
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    if (!transfer_function_texture->is_active)
    {
        print_warning_message("Cannot modify inactive transfer function.");
        return;
    }

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

static unsigned int activate_next_transfer_function_texture(void)
{
    unsigned int first_inactive_id = MAX_TRANSFER_FUNCTIONS;
    unsigned int id;
    for (id = 0; id < MAX_TRANSFER_FUNCTIONS; id++)
    {
        if (!textures[id].is_active)
        {
            first_inactive_id = id;
            textures[id].is_active = 1;
            break;
        }
    }

    if (first_inactive_id == MAX_TRANSFER_FUNCTIONS)
        print_severe_message("Cannot exceed max limit of %d active transfer functions.", MAX_TRANSFER_FUNCTIONS);

    return first_inactive_id;
}

static void transfer_transfer_function_texture(TransferFunctionTexture* transfer_function_texture)
{
    check(transfer_function_texture);
    check(transfer_function_texture->texture);

    glGenTextures(1, &transfer_function_texture->texture->id);
    abort_on_GL_error("Could not generate texture object for transfer function");

    glActiveTexture(GL_TEXTURE0 + transfer_function_texture->texture->unit);
    abort_on_GL_error("Could not set active texture unit for transfer function");

    glBindTexture(GL_TEXTURE_1D, transfer_function_texture->texture->id);
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

static void destroy_transfer_function_texture(TransferFunctionTexture* transfer_function_texture)
{
    check(transfer_function_texture);
    check(transfer_function_texture->texture);

    if (!transfer_function_texture->is_active)
    {
        print_warning_message("Cannot destroy transfer function texture before it is created.");
        return;
    }

    destroy_texture(transfer_function_texture->texture->handle);

    transfer_function_texture->needs_sync = 0;
    transfer_function_texture->is_active = 0;
}

static void reset_transfer_function_texture_data(TransferFunctionTexture* transfer_function_texture, unsigned int component)
{
    check(transfer_function_texture);
    check(component < TRANSFER_FUNCTION_COMPONENTS);

    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    transfer_function->types[component] = PIECEWISE_LINEAR;

    transfer_function->node_states[0][component] = 1;
    transfer_function->node_states[MAX_TRANSFER_FUNCTIONS - 1][component] = 1;

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
    const float offset = expf(start_value);
    const float scale = (expf(end_value) - offset)/(segment_length - 1);

    for (i = 0; i < segment_length; i++)
        segment[i*stride] = logf(i*scale + offset);
}
