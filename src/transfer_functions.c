#include "transfer_functions.h"

#include "gl_includes.h"
#include "error.h"
#include "extra_math.h"
#include "hash_map.h"
#include "texture.h"
#include "shader_generator.h"

#include <stdio.h>
#include <math.h>
#include <string.h>


#define TRANSFER_FUNCTION_COMPONENTS 4
#define TRANSFER_FUNCTION_SIZE 256
#define TF_LOWER_NODE 0
#define TF_UPPER_NODE 255

#define INVISIBLE_ALPHA 1e-6f


enum transfer_function_type {PIECEWISE_LINEAR, LOGARITHMIC, CUSTOM};

typedef struct ValueLimits
{
    float lower_limit;
    float upper_limit;
    float range_norm;
    float scale;
    float offset;
    size_t lower_visibility;
    size_t upper_visibility;
    Uniform scale_uniform;
    Uniform offset_uniform;
} ValueLimits;

typedef struct TransferFunction
{
    ValueLimits limits;
    float output[TRANSFER_FUNCTION_SIZE][TRANSFER_FUNCTION_COMPONENTS];
    int node_states[TRANSFER_FUNCTION_SIZE][TRANSFER_FUNCTION_COMPONENTS];
    enum transfer_function_type types[TRANSFER_FUNCTION_COMPONENTS];
} TransferFunction;

typedef struct TransferFunctionTexture
{
    TransferFunction transfer_function;
    Texture* texture;
} TransferFunctionTexture;


static TransferFunctionTexture* get_transfer_function_texture(const char* name);

static unsigned int interior_texture_coordinate_to_lower_transfer_function_node(float texture_coordinate);
static float interior_transfer_function_node_to_texture_coordinate(unsigned int node);

static void update_transfer_function_limit_quantities(TransferFunction* transfer_function);

static void update_brick_tree_node_visibility_ratios(const TransferFunction* transfer_function, const Field* field, BrickTreeNode* node);
static void update_sub_brick_tree_node_visibility_ratios(const TransferFunction* transfer_function, const Field* field, SubBrickTreeNode* node);
static float compute_sub_brick_visibility_ratio(const TransferFunction* transfer_function, const Field* field, SubBrickTreeNode* node);

static void transfer_transfer_function_texture(TransferFunctionTexture* transfer_function_texture);

static void load_transfer_function(TransferFunctionTexture* transfer_function_texture);
static void sync_transfer_function(TransferFunctionTexture* transfer_function_texture, unsigned int offset, unsigned int size);
static void sync_transfer_function_limits(TransferFunctionTexture* transfer_function_texture);

static void clear_transfer_function_texture(TransferFunctionTexture* transfer_function_texture);
static void reset_transfer_function_texture_data(TransferFunctionTexture* transfer_function_texture, unsigned int component);

static unsigned int find_closest_node_above(const TransferFunction* transfer_function, unsigned int component, unsigned int node);
static unsigned int find_closest_node_below(const TransferFunction* transfer_function, unsigned int component, unsigned int node);

static void set_piecewise_linear_transfer_function_data(TransferFunction* transfer_function, unsigned int component,
                                                        unsigned int start_node, unsigned int end_node,
                                                        float start_value, float end_value);
static void set_logarithmic_transfer_function_data(TransferFunction* transfer_function, unsigned int component,
                                                   unsigned int start_node, unsigned int end_node,
                                                   float start_value, float end_value);
static void set_custom_transfer_function_data(TransferFunction* transfer_function, unsigned int component,
                                              unsigned int start_node, unsigned int end_node,
                                              const float* values);

static void compute_linear_array_segment(float* segment, size_t segment_length, unsigned int stride,
                                         float start_value, float end_value);
static void compute_logarithmic_array_segment(float* segment, size_t segment_length, unsigned int stride,
                                              float start_value, float end_value);
static void compute_custom_array_segment(float* segment, size_t segment_length, unsigned int stride,
                                         const float* values);

static const float NODE_RANGE_OFFSET = (float)TF_START_NODE;
static const float NODE_RANGE_SIZE = (float)(TF_END_NODE - TF_START_NODE);
static const float NODE_RANGE_NORM = 1.0f/(float)(TF_END_NODE - TF_START_NODE);

static const float TEXTURE_COORDINATE_PAD = 1.0f/TRANSFER_FUNCTION_SIZE;

static HashMap transfer_function_textures;

static ShaderProgram* active_shader_program = NULL;


void set_active_shader_program_for_transfer_functions(ShaderProgram* shader_program)
{
    active_shader_program = shader_program;
}

void initialize_transfer_functions(void)
{
    transfer_function_textures = create_map();
}

const char* create_transfer_function(void)
{
    check(active_shader_program);

    Texture* const texture = create_texture();

    MapItem item = insert_new_map_item(&transfer_function_textures, texture->name.chars, sizeof(TransferFunctionTexture));
    TransferFunctionTexture* const transfer_function_texture = (TransferFunctionTexture*)item.data;
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    unsigned int component;
    for (component = 0; component < TRANSFER_FUNCTION_COMPONENTS; component++)
        reset_transfer_function_texture_data(transfer_function_texture, component);

    transfer_function_texture->texture = texture;

    transfer_function->limits.lower_limit = 0.0f;
    transfer_function->limits.upper_limit = 1.0f;
    update_transfer_function_limit_quantities(transfer_function);

    transfer_transfer_function_texture(transfer_function_texture);

    initialize_uniform(&transfer_function->limits.scale_uniform, "%s_value_scale", texture->name.chars);
    initialize_uniform(&transfer_function->limits.offset_uniform, "%s_value_offset", texture->name.chars);

    add_transfer_function_in_shader(&active_shader_program->fragment_shader_source, texture->name.chars);

    return texture->name.chars;
}

void load_transfer_functions(void)
{
    check(active_shader_program);

    for (reset_map_iterator(&transfer_function_textures); valid_map_iterator(&transfer_function_textures); advance_map_iterator(&transfer_function_textures))
    {
        TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(get_current_map_key(&transfer_function_textures));
        load_transfer_function(transfer_function_texture);
    }
}

void print_transfer_function(const char* name, enum transfer_function_component component)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);

    unsigned int node;
    for (node = 0; node < TRANSFER_FUNCTION_SIZE; node++)
        printf("%d (%5.3f): %.3f\n", node, clamp(((float)node - NODE_RANGE_OFFSET)*NODE_RANGE_NORM, 0, 1), transfer_function_texture->transfer_function.output[node][component]);
}

void reset_transfer_function(const char* name, enum transfer_function_component component)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    reset_transfer_function_texture_data(transfer_function_texture, (unsigned int)component);
    sync_transfer_function(transfer_function_texture, TF_LOWER_NODE, TRANSFER_FUNCTION_SIZE);
}

void set_piecewise_linear_transfer_function_node(const char* name, enum transfer_function_component component,
                                                 unsigned int node, float value)
{
    if (node < TF_START_NODE || node > TF_END_NODE)
    {
        print_warning_message("Cannot set non-interior node in transfer function.");
        return;
    }

    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

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

    sync_transfer_function(transfer_function_texture, closest_node_below + 1, closest_node_above - closest_node_below - 1);
}

void remove_piecewise_linear_transfer_function_node(const char* name, enum transfer_function_component component,
                                                    unsigned int node)
{
    if (node < TF_START_NODE || node > TF_END_NODE)
    {
        print_warning_message("Cannot remove non-interior node from transfer function.");
        return;
    }

    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    if (transfer_function->types[component] != PIECEWISE_LINEAR)
    {
        print_warning_message("Cannot remove node from transfer function that is not piecewise linear.");
        return;
    }

    if (!transfer_function->node_states[node][component])
        return;

    transfer_function->node_states[node][component] = 0;

    const unsigned int closest_node_above = find_closest_node_above(transfer_function, component, node);
    const unsigned int closest_node_below = find_closest_node_below(transfer_function, component, node);

    set_piecewise_linear_transfer_function_data(transfer_function, component, closest_node_below, closest_node_above,
                                                transfer_function->output[closest_node_below][component],
                                                transfer_function->output[closest_node_above][component]);

    sync_transfer_function(transfer_function_texture, closest_node_below + 1, closest_node_above - closest_node_below - 1);
}

void set_logarithmic_transfer_function(const char* name, enum transfer_function_component component,
                                       float start_value, float end_value)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    if (end_value <= start_value)
    {
        print_warning_message("Cannot create logarithmic transfer function between non-ascending values.");
        return;
    }

    transfer_function->types[component] = LOGARITHMIC;

    unsigned int node;
    for (node = TF_START_NODE; node <= TF_END_NODE; node++)
        transfer_function->node_states[node][component] = 0;

    set_logarithmic_transfer_function_data(transfer_function, component, TF_START_NODE, TF_END_NODE, start_value, end_value);

    sync_transfer_function(transfer_function_texture, TF_LOWER_NODE, TRANSFER_FUNCTION_SIZE);
}

void set_custom_transfer_function(const char* name, enum transfer_function_component component,
                                  const float* values)
{
    check(values);

    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    transfer_function->types[component] = CUSTOM;

    unsigned int node;
    for (node = TF_START_NODE; node <= TF_END_NODE; node++)
        transfer_function->node_states[node][component] = 0;

    set_custom_transfer_function_data(transfer_function, component, TF_START_NODE, TF_END_NODE, values);

    sync_transfer_function(transfer_function_texture, TF_LOWER_NODE, TRANSFER_FUNCTION_SIZE);
}

void set_transfer_function_lower_limit(const char* name, float lower_limit)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    transfer_function->limits.lower_limit = fminf(transfer_function->limits.upper_limit, fmaxf(0.0f, lower_limit));
    update_transfer_function_limit_quantities(transfer_function);

    sync_transfer_function_limits(transfer_function_texture);
}

void set_transfer_function_upper_limit(const char* name, float upper_limit)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    transfer_function->limits.upper_limit = fmaxf(transfer_function->limits.lower_limit, fminf(1.0f, upper_limit));
    update_transfer_function_limit_quantities(transfer_function);

    sync_transfer_function_limits(transfer_function_texture);
}

void set_transfer_function_lower_node_value(const char* name, enum transfer_function_component component, float value)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    transfer_function->output[TF_LOWER_NODE][component] = value;

    if (component == TF_ALPHA)
        transfer_function->limits.lower_visibility = value > INVISIBLE_ALPHA;

    sync_transfer_function(transfer_function_texture, TF_LOWER_NODE, 1);
}

void set_transfer_function_upper_node_value(const char* name, enum transfer_function_component component, float value)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    transfer_function->output[TF_UPPER_NODE][component] = value;

    if (component == TF_ALPHA)
        transfer_function->limits.upper_visibility = value > INVISIBLE_ALPHA;

    sync_transfer_function(transfer_function_texture, TF_UPPER_NODE-100, 101);
}

void update_visibility_ratios(const char* transfer_function_name, BrickedField* bricked_field)
{
    check(bricked_field->field);

    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(transfer_function_name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    update_brick_tree_node_visibility_ratios(transfer_function, bricked_field->field, bricked_field->tree);
}

unsigned int texture_coordinate_to_nearest_transfer_function_node(float texture_coordinate)
{
    return (unsigned int)(NODE_RANGE_OFFSET + clamp(texture_coordinate, 0, 1)*NODE_RANGE_SIZE + 0.5f);
}

float transfer_function_node_to_texture_coordinate(unsigned int node)
{
    return clamp(((float)node - NODE_RANGE_OFFSET)*NODE_RANGE_NORM, 0, 1);
}

void remove_transfer_function(const char* name)
{
    TransferFunctionTexture* const transfer_function_texture = get_transfer_function_texture(name);
    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    check(transfer_function_texture->texture);
    Texture* const texture = transfer_function_texture->texture;

    destroy_uniform(&transfer_function->limits.scale_uniform);
    destroy_uniform(&transfer_function->limits.offset_uniform);

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

    active_shader_program = NULL;
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

static unsigned int interior_texture_coordinate_to_lower_transfer_function_node(float texture_coordinate)
{
    return (unsigned int)(NODE_RANGE_OFFSET + texture_coordinate*NODE_RANGE_SIZE);
}

static float interior_transfer_function_node_to_texture_coordinate(unsigned int node)
{
    return ((float)node - NODE_RANGE_OFFSET)*NODE_RANGE_NORM;
}

static void update_transfer_function_limit_quantities(TransferFunction* transfer_function)
{
    assert(transfer_function);

    transfer_function->limits.range_norm = 1.0f/(transfer_function->limits.upper_limit - transfer_function->limits.lower_limit);

    transfer_function->limits.scale  = (1.0f - 2.0f*TEXTURE_COORDINATE_PAD)*transfer_function->limits.range_norm;

    transfer_function->limits.offset = (        TEXTURE_COORDINATE_PAD*transfer_function->limits.upper_limit -
                                        (1.0f - TEXTURE_COORDINATE_PAD)*transfer_function->limits.lower_limit)*transfer_function->limits.range_norm;
}

static void update_brick_tree_node_visibility_ratios(const TransferFunction* transfer_function, const Field* field, BrickTreeNode* node)
{
    assert(transfer_function);
    assert(field);
    assert(node);

    if (node->brick)
    {
        update_sub_brick_tree_node_visibility_ratios(transfer_function, field, node->brick->tree);
        node->visibility_ratio = node->brick->tree->visibility_ratio;
    }
    else
    {
        assert(node->lower_child);
        assert(node->upper_child);

        update_brick_tree_node_visibility_ratios(transfer_function, field, node->lower_child);
        update_brick_tree_node_visibility_ratios(transfer_function, field, node->upper_child);

        node->visibility_ratio = 0.5f*(node->lower_child->visibility_ratio + node->upper_child->visibility_ratio);
    }

    node->visibility = UNDETERMINED_REGION_VISIBILITY;
}

static void update_sub_brick_tree_node_visibility_ratios(const TransferFunction* transfer_function, const Field* field, SubBrickTreeNode* node)
{
    assert(transfer_function);
    assert(field);
    assert(node);

    if (node->lower_child)
    {
        assert(node->upper_child);

        update_sub_brick_tree_node_visibility_ratios(transfer_function, field, node->lower_child);
        update_sub_brick_tree_node_visibility_ratios(transfer_function, field, node->upper_child);

        node->visibility_ratio = 0.5f*(node->lower_child->visibility_ratio + node->upper_child->visibility_ratio);
    }
    else
    {
        node->visibility_ratio = compute_sub_brick_visibility_ratio(transfer_function, field, node);
    }

    node->visibility = UNDETERMINED_REGION_VISIBILITY;
}

static float compute_sub_brick_visibility_ratio(const TransferFunction* transfer_function, const Field* field, SubBrickTreeNode* node)
{
    assert(transfer_function);
    assert(field);
    assert(node);

    const size_t offset = (node->offset_z*field->size_y + node->offset_y)*field->size_x + node->offset_x;

    float field_value;
    float texture_coordinate;
    unsigned int node_below;
    float above_weight;
    size_t n_visible_voxels = 0;

    size_t i, j, k;
    for (k = 0; k < node->size_z; k++)
        for (j = 0; j < node->size_y; j++)
            for (i = 0; i < node->size_x; i++)
    {
        field_value = field->data[offset + (k*field->size_y + j)*field->size_x + i];

        if (field_value <= transfer_function->limits.lower_limit)
        {
            n_visible_voxels += transfer_function->limits.lower_visibility;
        }
        else if (field_value >= transfer_function->limits.upper_limit)
        {
            n_visible_voxels += transfer_function->limits.upper_visibility;
        }
        else
        {
            texture_coordinate = (field_value - transfer_function->limits.lower_limit)*transfer_function->limits.range_norm;
            node_below = interior_texture_coordinate_to_lower_transfer_function_node(texture_coordinate);
            above_weight = (texture_coordinate - interior_transfer_function_node_to_texture_coordinate(node_below))*NODE_RANGE_SIZE;

            n_visible_voxels += ((1.0f - above_weight)*transfer_function->output[node_below][TF_ALPHA] +
                                         above_weight *transfer_function->output[node_below + 1][TF_ALPHA]) > INVISIBLE_ALPHA;
        }
    }

    return (float)n_visible_voxels/(float)(node->size_x*node->size_y*node->size_z);
}

static void transfer_transfer_function_texture(TransferFunctionTexture* transfer_function_texture)
{
    check(transfer_function_texture);
    check(transfer_function_texture->texture);

    glActiveTexture(GL_TEXTURE0 + transfer_function_texture->texture->unit);
    abort_on_GL_error("Could not set active texture unit for transfer function");

    ListItem item = append_new_list_item(&transfer_function_texture->texture->ids, sizeof(GLuint));
    GLuint* const id = (GLuint*)item.data;

    glGenTextures(1, id);
    abort_on_GL_error("Could not generate texture object for transfer function");

    glBindTexture(GL_TEXTURE_1D, *id);
    abort_on_GL_error("Could not bind 1D texture for transfer function");

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glTexImage1D(GL_TEXTURE_1D,
                 0,
                 GL_RGBA,
                 (GLsizei)TRANSFER_FUNCTION_SIZE,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 (GLvoid*)transfer_function_texture->transfer_function.output);
    abort_on_GL_error("Could not define 1D texture image for transfer function");
}

static void load_transfer_function(TransferFunctionTexture* transfer_function_texture)
{
    assert(active_shader_program);
    assert(transfer_function_texture);

    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    load_uniform(active_shader_program, &transfer_function->limits.scale_uniform);
    load_uniform(active_shader_program, &transfer_function->limits.offset_uniform);

    sync_transfer_function_limits(transfer_function_texture);
}

static void sync_transfer_function(TransferFunctionTexture* transfer_function_texture, unsigned int offset, unsigned int size)
{
    assert(offset + size <= TRANSFER_FUNCTION_SIZE);

    assert(active_shader_program);
    assert(transfer_function_texture);

    glActiveTexture(GL_TEXTURE0 + transfer_function_texture->texture->unit);
    abort_on_GL_error("Could not set active texture unit for transfer function");

    glTexSubImage1D(GL_TEXTURE_1D,
                    0,
                    (GLint)offset,
                    (GLsizei)size,
                    GL_RGBA,
                    GL_FLOAT,
                    (GLvoid*)transfer_function_texture->transfer_function.output);
    abort_on_GL_error("Could not sync transfer function texture data");
}

static void sync_transfer_function_limits(TransferFunctionTexture* transfer_function_texture)
{
    assert(active_shader_program);
    assert(transfer_function_texture);

    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program for updating field texture uniforms");

    glUniform1f(transfer_function->limits.scale_uniform.location, transfer_function->limits.scale);
    abort_on_GL_error("Could not update transfer function limit scale uniform");

    glUniform1f(transfer_function->limits.offset_uniform.location, transfer_function->limits.offset);
    abort_on_GL_error("Could not update transfer function limit offset uniform");

    glUseProgram(0);
}

static void clear_transfer_function_texture(TransferFunctionTexture* transfer_function_texture)
{
    assert(transfer_function_texture);
    assert(transfer_function_texture->texture);

    destroy_texture(transfer_function_texture->texture);

    destroy_uniform(&transfer_function_texture->transfer_function.limits.offset_uniform);
    destroy_uniform(&transfer_function_texture->transfer_function.limits.scale_uniform);

    transfer_function_texture->texture = NULL;
}

static void reset_transfer_function_texture_data(TransferFunctionTexture* transfer_function_texture, unsigned int component)
{
    check(transfer_function_texture);
    check(component < TRANSFER_FUNCTION_COMPONENTS);

    TransferFunction* const transfer_function = &transfer_function_texture->transfer_function;

    transfer_function->types[component] = PIECEWISE_LINEAR;

    transfer_function->node_states[TF_LOWER_NODE][component] = 1;
    transfer_function->node_states[TF_UPPER_NODE][component] = 1;

    unsigned int i;
    for (i = TF_START_NODE; i <= TF_END_NODE; i++)
        transfer_function->node_states[i][component] = 0;

    transfer_function->output[TF_LOWER_NODE][component] = 0.0f;
    transfer_function->output[TF_UPPER_NODE][component] = 1.0f;

    compute_linear_array_segment(transfer_function->output[TF_START_NODE] + component,
                                 TRANSFER_FUNCTION_SIZE - 2,
                                 TRANSFER_FUNCTION_COMPONENTS,
                                 0.0f, 1.0f);

    if (component == TF_ALPHA)
    {
        transfer_function->limits.lower_visibility = 0;
        transfer_function->limits.upper_visibility = 1;
    }
}

static unsigned int find_closest_node_above(const TransferFunction* transfer_function, unsigned int component, unsigned int node)
{
    check(transfer_function);
    unsigned int i;
    unsigned int closest_node_above = TF_UPPER_NODE;

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
    unsigned int closest_node_below = TF_LOWER_NODE;

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
    assert(transfer_function);
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
    assert(transfer_function);
    assert(component < TRANSFER_FUNCTION_COMPONENTS);
    assert(end_node > start_node);
    assert(end_node < TRANSFER_FUNCTION_SIZE);

    compute_logarithmic_array_segment(transfer_function->output[start_node] + component,
                                      end_node - start_node + 1,
                                      TRANSFER_FUNCTION_COMPONENTS,
                                      start_value, end_value);
}

static void set_custom_transfer_function_data(TransferFunction* transfer_function, unsigned int component,
                                              unsigned int start_node, unsigned int end_node,
                                              const float* values)
{
    assert(transfer_function);
    assert(component < TRANSFER_FUNCTION_COMPONENTS);
    assert(end_node > start_node);
    assert(end_node < TRANSFER_FUNCTION_SIZE);
    assert(values);

    compute_custom_array_segment(transfer_function->output[start_node] + component,
                                 end_node - start_node + 1,
                                 TRANSFER_FUNCTION_COMPONENTS,
                                 values);
}

static void compute_linear_array_segment(float* segment, size_t segment_length, unsigned int stride,
                                         float start_value, float end_value)
{
    assert(segment);
    assert(segment_length > 1);

    size_t i;
    const float scale = (end_value - start_value)/(float)(segment_length - 1);

    for (i = 0; i < segment_length; i++)
        segment[i*stride] = start_value + (float)i*scale;
}

static void compute_logarithmic_array_segment(float* segment, size_t segment_length, unsigned int stride,
                                              float start_value, float end_value)
{
    assert(segment);
    assert(segment_length > 1);
    assert(end_value > start_value);

    size_t i;
    const float offset = powf(10, start_value);
    const float scale = (powf(10, end_value) - offset)/(float)(segment_length - 1);

    for (i = 0; i < segment_length; i++)
        segment[i*stride] = log10f((float)i*scale + offset);
}

static void compute_custom_array_segment(float* segment, size_t segment_length, unsigned int stride,
                                         const float* values)
{
    assert(segment);
    assert(segment_length > 1);
    assert(values);

    size_t i;

    for (i = 0; i < segment_length; i++)
        segment[i*stride] = values[i];
}
