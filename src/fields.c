#include "fields.h"

#include "error.h"
#include "io.h"
#include "hash_map.h"

#include <stdlib.h>
#include <float.h>


static Field* create_field(const char* name,
                           enum field_type type, float* data,
                           size_t size_x, size_t size_y, size_t size_z,
                           float extent_x, float extent_y, float extent_z);
static size_t get_field_array_length(const Field* field);
static void find_float_array_limits(const float* array, size_t length, float* min_value, float* max_value);
static void scale_float_array(float* array, size_t length, float zero_value, float unity_value);
static void clear_field(Field* field);


static HashMap fields;


void initialize_fields(void)
{
    fields = create_map();
}

Field* create_field_from_bifrost_file(const char* name, const char* data_filename, const char* header_filename)
{
    check(name);
    check(data_filename);
    check(header_filename);

    char* header = read_text_file(header_filename);

    if (!header)
        print_severe_message("Could not read header file.");

    const char element_kind = find_char_entry_in_header(header, "element_kind", ":");
    const int element_size = find_int_entry_in_header(header, "element_size", ":");
    const char endianness = find_char_entry_in_header(header, "endianness", ":");
    const int dimensions = find_int_entry_in_header(header, "dimensions", ":");
    const char order = find_char_entry_in_header(header, "order", ":");
    const int signed_size_x = find_int_entry_in_header(header, "x_size", ":");
    const int signed_size_y = find_int_entry_in_header(header, "y_size", ":");
    const int signed_size_z = find_int_entry_in_header(header, "z_size", ":");
    const float dx = find_float_entry_in_header(header, "dx", ":");
    const float dy = find_float_entry_in_header(header, "dy", ":");
    const float dz = find_float_entry_in_header(header, "dz", ":");

    free(header);

    if (element_kind == 0 || element_size == 0 || dimensions == 0 ||
        signed_size_x == 0 || signed_size_y == 0 || signed_size_z == 0 ||
        dx == 0 || dy == 0 || dz == 0)
        print_severe_message("Could not determine all required header entries.");

    if (element_kind != 'f')
        print_severe_message("Field data must be floating-point.");

    if (element_size != 4)
        print_severe_message("Field data must have 4-byte precision.");

    if (is_little_endian())
    {
        if (endianness != 'l')
            print_severe_message("Field data must be little-endian.");
    }
    else
    {
        if (endianness != 'b')
            print_severe_message("Field data must be big-endian.");
    }

    if (dimensions != 3)
        print_severe_message("Field data must be 3D.");

    if (order != 'C')
        print_severe_message("Field data must laid out row-major order.");

    if (signed_size_x < 2 || signed_size_y < 2 || signed_size_z < 2)
        print_severe_message("Field dimensions cannot smaller than 2 along any axis.");

    const size_t size_x = (size_t)signed_size_x;
    const size_t size_y = (size_t)signed_size_y;
    const size_t size_z = (size_t)signed_size_z;

    print_info_message("%d, %d, %d", size_x, size_y, size_z);

    const size_t length = size_x*size_y*size_z;

    float* data = (float*)read_binary_file(data_filename, length, sizeof(float));

    const float extent_x = (size_x - 1)*dx;
    const float extent_y = (size_y - 1)*dy;
    const float extent_z = (size_z - 1)*dz;

    return create_field(name, SCALAR_FIELD, data, size_x, size_y, size_z, extent_x, extent_y, extent_z);
}

Field* get_field(const char* name)
{
    check(name);

    MapItem item = get_map_item(&fields, name);
    assert(item.size == sizeof(Field));
    Field* const field = (Field*)item.data;
    check(field);

    return field;
}

void destroy_field(const char* name)
{
    Field* const field = get_field(name);
    clear_field(field);
    remove_map_item(&fields, name);
}

void cleanup_fields(void)
{
    for (reset_map_iterator(&fields); valid_map_iterator(&fields); advance_map_iterator(&fields))
    {
        Field* const field = get_field(get_current_map_key(&fields));
        clear_field(field);
    }

    destroy_map(&fields);
}

void clip_field_values(const char* name, float lower_clip_value, float upper_clip_value)
{
    check(upper_clip_value > lower_clip_value);

    Field* const field = get_field(name);
    check(field->data);

    const float scale = 1.0f/(field->upper_clip_value - field->lower_clip_value);
    const float new_zero_value = (lower_clip_value - field->lower_clip_value)*scale;
    const float new_unity_value = (upper_clip_value - field->lower_clip_value)*scale;

    scale_float_array(field->data, get_field_array_length(field), new_zero_value, new_unity_value);

    field->lower_clip_value = lower_clip_value;
    field->upper_clip_value = upper_clip_value;
}

float field_value_to_normalized_value(const char* name, float field_value)
{
    Field* const field = get_field(name);
    check(field->upper_clip_value > field->lower_clip_value);
    return (field_value - field->lower_clip_value)/(field->upper_clip_value - field->lower_clip_value);
}

float normalized_value_to_field_value(const char* name, float normalized_value)
{
    Field* const field = get_field(name);
    check(field->upper_clip_value > field->lower_clip_value);
    return field->lower_clip_value + normalized_value*(field->upper_clip_value - field->lower_clip_value);
}

static Field* create_field(const char* name,
                           enum field_type type, float* data,
                           size_t size_x, size_t size_y, size_t size_z,
                           float extent_x, float extent_y, float extent_z)
{
    check(name);
    check(data);

    MapItem item = insert_new_map_item(&fields, name, sizeof(Field));
    Field* const field = (Field*)item.data;

    field->data = data;
    field->type = type;
    field->size_x = size_x;
    field->size_y = size_y;
    field->size_z = size_z;
    field->extent_x = extent_x;
    field->extent_y = extent_y;
    field->extent_z = extent_z;

    const size_t length = get_field_array_length(field);

    find_float_array_limits(data, length, &field->min_value, &field->max_value);

    scale_float_array(data, length, field->min_value, field->max_value);

    field->lower_clip_value = field->min_value;
    field->upper_clip_value = field->max_value;

    return field;
}

static size_t get_field_array_length(const Field* field)
{
    return field->size_x*field->size_y*field->size_z*((field->type == VECTOR_FIELD) ? 3 : 1);
}

static void find_float_array_limits(const float* array, size_t length, float* min_value, float* max_value)
{
    assert(array);
    assert(length > 0);
    assert(min_value);
    assert(max_value);

    size_t i;
    float value;
    float min = FLT_MAX;
    float max = -FLT_MAX;

    for (i = 0; i < length; i++)
    {
        value = array[i];
        if (value < min)
            min = value;
        if (value > max)
            max = value;
    }

    *min_value = min;
    *max_value = max;
}

static void scale_float_array(float* array, size_t length, float zero_value, float unity_value)
{
    assert(array);
    assert(length > 0);

    if (zero_value >= unity_value)
    {
        print_warning_message("Can only scale array with unity value larger than zero value.");
        return;
    }

    size_t i;
    const float scale = 1.0f/(unity_value - zero_value);

    for (i = 0; i < length; i++)
        array[i] = (array[i] - zero_value)*scale;
}

static void clear_field(Field* field)
{
    check(field);

    if (field->data)
        free(field->data);

    field->data = NULL;
    field->type = NULL_FIELD;
    field->size_x = 0;
    field->size_y = 0;
    field->size_z = 0;
    field->extent_x = 0;
    field->extent_y = 0;
    field->extent_z = 0;
    field->min_value = 0;
    field->max_value = 0;
    field->lower_clip_value = 0;
    field->upper_clip_value = 0;
}
