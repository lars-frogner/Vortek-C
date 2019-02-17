#include "io.h"

#include "error.h"
#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <errno.h>


static int is_little_endian(void);

static FloatField create_float_field(float* data, size_t size_x, size_t size_y, size_t size_z);

static void* read_binary_file(const char* filename, size_t length, size_t element_size);

static char* create_string_copy(const char* string);
static char* strip_string_in_place(char* string);
static char** extract_lines_from_string(char* string, size_t* n_lines);
static char* find_entry_in_header(char* header, const char* entry_name, const char* separator);
static int find_int_entry_in_header(const char* header, const char* entry_name, const char* separator);
static float find_float_entry_in_header(const char* header, const char* entry_name, const char* separator);
static char find_char_entry_in_header(const char* header, const char* entry_name, const char* separator);


char* read_text_file(const char* filename)
{
    check(filename);

    FILE* file;
    long signed_file_size = -1;
    char* content = NULL;

    file = fopen(filename, "rb");

    if (!file)
    {
        print_error_message("Could not open file %s.", filename);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END))
    {
        fclose(file);
        print_error_message("Could not seek in file %s.", filename);
        return NULL;
    }

    signed_file_size = ftell(file);

    if (signed_file_size < 0)
    {
        fclose(file);
        print_error_message("Could not determine size of file %s.", filename);
        return NULL;
    }

    rewind(file);

    const size_t file_size = (size_t)signed_file_size;

    content = (char*)malloc(file_size + 1);

    if (!content)
    {
        fclose(file);
        print_error_message("Could not allocate %d bytes.", file_size);
        return NULL;
    }

    size_t read_size = fread(content, sizeof(char), file_size, file);

    fclose(file);

    if (read_size != file_size)
    {
        free(content);
        print_error_message("Could not read file %s.", filename);
        return NULL;
    }

    content[file_size] = '\0';

    return content;
}

FloatField read_bifrost_field(const char* data_filename, const char* header_filename)
{
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

    free(header);

    if (element_kind == 0 || element_size == 0 || dimensions == 0 ||
        signed_size_x == 0 || signed_size_y == 0 || signed_size_z == 0)
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

    if (signed_size_x < 0 || signed_size_y < 0 || signed_size_z < 0)
        print_severe_message("Field dimensions can not be negative.");

    const size_t size_x = (size_t)signed_size_x;
    const size_t size_y = (size_t)signed_size_y;
    const size_t size_z = (size_t)signed_size_z;

    print_info_message("%d, %d, %d", size_x, size_y, size_z);

    const size_t length = size_x*size_y*size_z;

    float* data = (float*)read_binary_file(data_filename, length, sizeof(float));

    return create_float_field(data, size_x, size_y, size_z);
}

static int is_little_endian()
{
    unsigned int i = 1;
    char* c = (char*)&i;
    return (int)(*c);
}

static FloatField create_float_field(float* data, size_t size_x, size_t size_y, size_t size_z)
{
    check(data);

    FloatField field;

    field.data = data;
    field.size_x = size_x;
    field.size_y = size_y;
    field.size_z = size_z;

    const size_t length = size_x*size_y*size_z;

    find_float_array_limits(data, length, &field.min_value, &field.max_value);

    scale_float_array(data, length, field.min_value, field.max_value);

    return field;
}

static void* read_binary_file(const char* filename, size_t length, size_t element_size)
{
    check(filename);

    FILE* file;
    void* data = NULL;

    file = fopen(filename, "rb");

    if (!file)
    {
        print_error_message("Could not open file %s.", filename);
        return NULL;
    }

    const size_t n_bytes = length*element_size;

    data = malloc(n_bytes);

    if (!data)
    {
        fclose(file);
        print_error_message("Could not allocate %d bytes.", n_bytes);
        return NULL;
    }

    size_t read_length = fread(data, element_size, length, file);

    fclose(file);

    if (read_length != length)
    {
        free(data);
        print_error_message("Could not read file %s.", filename);
        return NULL;
    }

    return data;
}

static char* create_string_copy(const char* string)
{
    check(string);

    const size_t length = strlen(string);

    char* string_copy = (char*)malloc((length + 1)*sizeof(char));
    check(string_copy);

    strcpy(string_copy, string);
    string_copy[length] = '\0';

    return string_copy;
}

static char* strip_string_in_place(char* string)
{
    check(string);

    const size_t length = strlen(string);

    char* beginning = string;
    char* end = string + length - 1;

    while (beginning <= end && isspace(*beginning))
    {
        *beginning = '\0';
        beginning++;
    }

    while(end > beginning && isspace(*end))
    {
        *end = '\0';
        end--;
    }

    return beginning;
}

static char** extract_lines_from_string(char* string, size_t* n_lines)
{
    check(string);
    check(n_lines);

    size_t i;
    size_t line_num;

    const size_t length = strlen(string);

    line_num = 1;
    for (i = 0; i < length; i++)
        line_num += (string[i] == '\n');

    const size_t n_lines_local = line_num-1;
    *n_lines = n_lines_local;

    if (n_lines_local == 0)
        return NULL;

    char** lines = (char**)malloc((n_lines_local)*sizeof(char*));
    check(lines);

    lines[0] = strtok(string, "\n");

    for (i = 1; i < n_lines_local; i++)
        lines[i] = strtok(NULL, "\n");

    return lines;
}

static char* find_entry_in_header(char* header, const char* entry_name, const char* separator)
{
    check(header);
    check(entry_name);
    check(separator);

    size_t i;
    char* word;
    size_t word_length;
    char* entry = NULL;

    size_t n_lines;
    char** lines = extract_lines_from_string(header, &n_lines);

    if (lines)
    {
        for (i = 0; i < n_lines; i++)
        {
            if (lines[i] && *lines[i] != '\0')
            {
                word = strtok(lines[i], separator);
                if (word && *word != '\0')
                {
                    word_length = strlen(word);
                    word = strip_string_in_place(word);

                    if (strcmp(word, entry_name) == 0)
                    {
                        entry = lines[i] + word_length + 1;
                        entry = strip_string_in_place(entry);
                        break;
                    }
                }
            }
        }
        free(lines);
    }

    return entry;
}

static int find_int_entry_in_header(const char* header, const char* entry_name, const char* separator)
{
    check(header);
    check(entry_name);
    check(separator);

    char* header_copy = create_string_copy(header);

    const char* entry_string = find_entry_in_header(header_copy, entry_name, separator);

    if (!entry_string)
    {
        free(header_copy);
        print_error_message("Could not find entry name %s in header. Returning 0.", entry_name);
        return 0;
    }

    char* end_ptr;
    long entry_long = strtol(entry_string, &end_ptr, 10);

    if (end_ptr == entry_string)
    {
        print_error_message("Could not convert to int value for entry %s %s %s. Returning 0.", entry_name, separator, entry_string);
        free(header_copy);
        return 0;
    }

    if (((entry_long == LONG_MAX || entry_long == LONG_MIN) && errno == ERANGE) ||
        ((entry_long > INT_MAX) || (entry_long < INT_MIN)))
    {
        free(header_copy);
        print_error_message("Entry value for %s out of range. Returning 0.", entry_name);
        return 0;
    }

    free(header_copy);

    return (int)entry_long;
}

static float find_float_entry_in_header(const char* header, const char* entry_name, const char* separator)
{
    check(header);
    check(entry_name);
    check(separator);

    char* header_copy = create_string_copy(header);

    const char* entry_string = find_entry_in_header(header_copy, entry_name, separator);

    if (!entry_string)
    {
        free(header_copy);
        print_error_message("Could not find entry name %s in header. Returning 0.", entry_name);
        return 0;
    }

    char* end_ptr;
    float entry = strtof(entry_string, &end_ptr);

    if (end_ptr == entry_string)
    {
        print_error_message("Could not convert to float value for entry %s %s %s. Returning 0.", entry_name, separator, entry_string);
        free(header_copy);
        return 0;
    }

    free(header_copy);

    return entry;
}

static char find_char_entry_in_header(const char* header, const char* entry_name, const char* separator)
{
    check(header);
    check(entry_name);
    check(separator);

    char* header_copy = create_string_copy(header);

    const char* entry_string = find_entry_in_header(header_copy, entry_name, separator);

    if (!entry_string)
    {
        free(header_copy);
        print_error_message("Could not find entry name %s in header. Returning 0.", entry_name);
        return 0;
    }

    char entry = *entry_string;

    free(header_copy);

    return entry;
}
