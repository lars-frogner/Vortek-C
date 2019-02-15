#include "io.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>


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

char* read_file(const char* filename, enum file_type type)
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

    content = (char*)malloc(file_size + ((type == ASCII) ? 1 : 0));

    if (!content)
    {
        print_error("Could not allocate %i bytes", file_size);
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

    if (type == ASCII)
        content[file_size] = '\0';

    return content;
}
