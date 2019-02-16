#include "io.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <errno.h>


static int is_little_endian();

static unsigned char* read_binary_file(const char* filename, size_t n_bytes);

static char* create_string_copy(const char* string);
static char* strip_string_in_place(char* string);
static char** extract_lines_from_string(char* string, size_t* n_lines);
static char* find_entry_in_header(char* header, const char* entry_name, const char* separator);
static int find_int_entry_in_header(const char* header, const char* entry_name, const char* separator);
static float find_float_entry_in_header(const char* header, const char* entry_name, const char* separator);
static char find_char_entry_in_header(const char* header, const char* entry_name, const char* separator);


void print_info(const char* message, ...)
{
    va_list args;
    printf("Info: ");
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\n");
}

void print_error(const char* message, ...)
{
    va_list args;
    fprintf(stderr, "Error: ");
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");
}

char* read_text_file(const char* filename)
{
    FILE* file;
    long file_size = -1;
    char* content = NULL;

    file = fopen(filename, "rb");

    if (!file)
    {
        print_error("Could not open file %s", filename);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END))
    {
        print_error("Could not seek in file %s", filename);
        fclose(file);
        return NULL;
    }

    file_size = ftell(file);

    if (file_size == -1)
    {
        print_error("Could not determine size of file %s", filename);
        fclose(file);
        return NULL;
    }

    rewind(file);

    content = (char*)malloc(file_size + 1);

    if (!content)
    {
        print_error("Could not allocate %d bytes", file_size);
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(content, sizeof(char), file_size, file);

    fclose(file);

    if ((long)read_size != file_size)
    {
        print_error("Could not read file %s", filename);
        free(content);
        return NULL;
    }

    content[file_size] = '\0';

    return content;
}

FloatField read_bifrost_field(const char* data_filename, const char* header_filename)
{
    char* header = read_text_file(header_filename);

    if (!header)
    {
        print_error("Could not read header file.");
        exit(EXIT_FAILURE);
    }

    const char data_kind = find_char_entry_in_header(header, "data_kind", ":");
    const int data_size = find_int_entry_in_header(header, "data_size", ":");
    const char endianness = find_char_entry_in_header(header, "endianness", ":");
    const int dimensions = find_int_entry_in_header(header, "dimensions", ":");
    const char order = find_char_entry_in_header(header, "order", ":");
    const int size_x = find_int_entry_in_header(header, "x_size", ":");
    const int size_y = find_int_entry_in_header(header, "y_size", ":");
    const int size_z = find_int_entry_in_header(header, "z_size", ":");

    free(header);

    if (data_kind == 0 || data_size == 0 || dimensions == 0 ||
        size_x == 0 || size_y == 0 || size_z == 0)
    {
        print_error("Could not determine all required header entries.");
        exit(EXIT_FAILURE);
    }

    if (data_kind != 'f')
    {
        print_error("Field data must be floating-point.");
        exit(EXIT_FAILURE);
    }

    if (data_size != 4)
    {
        print_error("Field data must have 4-byte precision.");
        exit(EXIT_FAILURE);
    }

    if (is_little_endian())
    {
        if (endianness != 'l')
        {
            print_error("Field data must be little-endian.");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (endianness != 'b')
        {
            print_error("Field data must be big-endian.");
            exit(EXIT_FAILURE);
        }
    }

    if (dimensions != 3)
    {
        print_error("Field data must be 3D.");
        exit(EXIT_FAILURE);
    }

    if (order != 'C')
    {
        print_error("Field data must laid out row-major order.");
        exit(EXIT_FAILURE);
    }

    if (size_x < 0 || size_y < 0 || size_z < 0)
    {
        print_error("Field dimensions can not be negative.");
        exit(EXIT_FAILURE);
    }

    FloatField field;

    field.size_x = (unsigned int)size_x;
    field.size_y = (unsigned int)size_y;
    field.size_z = (unsigned int)size_z;

    const size_t n_bytes = sizeof(float)*field.size_x*field.size_y*field.size_z;

    field.data = (float*)read_binary_file(data_filename, n_bytes);

    return field;
}

static int is_little_endian()
{
    unsigned int i = 1;
    char* c = (char*)&i;
    return (int)(*c);
}

static unsigned char* read_binary_file(const char* filename, size_t n_bytes)
{
    FILE* file;
    unsigned char* data = NULL;

    file = fopen(filename, "rb");

    if (!file)
    {
        print_error("Could not open file %s", filename);
        return NULL;
    }

    data = (unsigned char*)malloc(n_bytes);

    if (!data)
    {
        print_error("Could not allocate %d bytes", n_bytes);
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(data, sizeof(unsigned char), n_bytes, file);

    fclose(file);

    if (read_size != n_bytes)
    {
        print_error("Could not read file %s", filename);
        free(data);
        return NULL;
    }

    return data;
}

static char* create_string_copy(const char* string)
{
    const size_t length = strlen(string);
    char* string_copy = (char*)malloc((length + 1)*sizeof(char));
    strcpy(string_copy, string);
    string_copy[length] = '\0';
    return string_copy;
}

static char* strip_string_in_place(char* string)
{
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
    int i;
    size_t line_num;

    const size_t length = strlen(string);

    line_num = 1;
    for (i = 0; i < length; i++)
        line_num += (string[i] == '\n');

    *n_lines = line_num-1;

    if (*n_lines == 0)
        return NULL;

    char** lines = (char**)malloc((*n_lines)*sizeof(char*));

    lines[0] = strtok(string, "\n");

    for (i = 1; i < (*n_lines); i++)
        lines[i] = strtok(NULL, "\n");

    return lines;
}

static char* find_entry_in_header(char* header, const char* entry_name, const char* separator)
{
    int i;
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
    char* header_copy = create_string_copy(header);

    const char* entry_string = find_entry_in_header(header_copy, entry_name, separator);

    if (!entry_string)
    {
        print_error("Could not find entry name %s in header. Returning 0.", entry_name);
        free(header_copy);
        return 0;
    }

    char* end_ptr;
    long entry_long = strtol(entry_string, &end_ptr, 10);

    if (end_ptr == entry_string)
    {
        print_error("Could not convert to int value for entry %s %s %s. Returning 0.", entry_name, separator, entry_string);
        free(header_copy);
        return 0;
    }

    if (((entry_long == LONG_MAX || entry_long == LONG_MIN) && errno == ERANGE) ||
        ((entry_long > INT_MAX) || (entry_long < INT_MIN)))
    {
        print_error("Entry value for %s out of range. Returning 0.", entry_name);
        free(header_copy);
        return 0;
    }

    free(header_copy);

    return (int)entry_long;
}

static float find_float_entry_in_header(const char* header, const char* entry_name, const char* separator)
{
    char* header_copy = create_string_copy(header);

    const char* entry_string = find_entry_in_header(header_copy, entry_name, separator);

    if (!entry_string)
    {
        print_error("Could not find entry name %s in header. Returning 0.", entry_name);
        free(header_copy);
        return 0;
    }

    char* end_ptr;
    float entry = strtof(entry_string, &end_ptr);

    if (end_ptr == entry_string)
    {
        print_error("Could not convert to float value for entry %s %s %s. Returning 0.", entry_name, separator, entry_string);
        free(header_copy);
        return 0;
    }

    free(header_copy);

    return entry;
}

static char find_char_entry_in_header(const char* header, const char* entry_name, const char* separator)
{
    char* header_copy = create_string_copy(header);

    const char* entry_string = find_entry_in_header(header_copy, entry_name, separator);

    if (!entry_string)
    {
        print_error("Could not find entry name %s in header. Returning 0.", entry_name);
        free(header_copy);
        return 0;
    }

    free(header_copy);

    return *entry_string;
}
