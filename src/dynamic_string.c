#include "dynamic_string.h"

#include "error.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


DynamicString create_empty_string(void)
{
    DynamicString string;
    string.chars = NULL;
    string.size = 0;
    string.length = 0;
    return string;
}

DynamicString create_duplicate_string(const DynamicString* source_string)
{
    check(source_string);

    DynamicString destination_string;

    destination_string.chars = (char*)malloc(source_string->size);
    strcpy(destination_string.chars, source_string->chars);

    destination_string.size = source_string->size;
    destination_string.length = source_string->length;

    return destination_string;
}

void copy_string_attributes(const DynamicString* source, DynamicString* destination)
{
    check(source);
    check(destination);
    destination->chars = source->chars;
    destination->size = source->size;
    destination->length = source->length;
}

DynamicString create_string(const char* chars, ...)
{
    check(chars);

    DynamicString string;

    va_list args;

    // Perform "dry run" of formatted printing to obtain the length of the resulting string
    va_start(args, chars);
    const int length = vsnprintf(NULL, 0, chars, args);
    va_end(args);

    check(length > 0);
    string.length = (size_t)length;
    string.size = sizeof(char)*(string.length + 1);

    // Allocate the required amount of memory to hold the formatted expression
    string.chars = (char*)malloc(string.size);

    // Perform the actual formatted printing
    va_start(args, chars);
    vsprintf(string.chars, chars, args);
    va_end(args);

    return string;
}

DynamicString create_string_from_arg_list(const char* chars, va_list args)
{
    check(chars);

    DynamicString string;

    va_list args_copy;

    va_copy(args_copy, args);
    const int length = vsnprintf(NULL, 0, chars, args_copy);
    va_end(args_copy);

    check(length > 0);
    string.length = (size_t)length;
    string.size = sizeof(char)*(string.length + 1);

    string.chars = (char*)malloc(string.size);

    vsprintf(string.chars, chars, args);

    return string;
}

void copy_string(DynamicString* destination_string, const DynamicString* source_string)
{
    check(destination_string);
    check(source_string);

    if (!source_string->chars)
        return;

    clear_string(destination_string);

    destination_string->chars = (char*)malloc(source_string->size);
    strcpy(destination_string->chars, source_string->chars);

    destination_string->size = source_string->size;
    destination_string->length = source_string->length;
}

void set_string(DynamicString* string, const char* chars, ...)
{
    check(string);
    check(chars);

    if (string->chars)
        free(string->chars);

    va_list args;

    va_start(args, chars);
    const int length = vsnprintf(NULL, 0, chars, args);
    va_end(args);

    check(length > 0);
    string->length = (size_t)length;
    string->size = sizeof(char)*(string->length + 1);

    string->chars = (char*)malloc(string->size);

    va_start(args, chars);
    vsprintf(string->chars, chars, args);
    va_end(args);
}

void extend_string(DynamicString* string, const char* extension_chars, ...)
{
    check(string);
    check(extension_chars);

    char* old_chars = string->chars;
    const size_t old_length = string->length;

    va_list args;

    va_start(args, extension_chars);
    const int extension_length = vsnprintf(NULL, 0, extension_chars, args);
    va_end(args);

    check(extension_length > 0);
    string->length = old_length + (size_t)extension_length;
    string->size = sizeof(char)*(string->length + 1);

    string->chars = (char*)malloc(string->size);

    if (old_chars)
        strcpy(string->chars, old_chars);

    va_start(args, extension_chars);
    vsprintf(string->chars + old_length, extension_chars, args);
    va_end(args);

    if (old_chars)
        free(old_chars);
}

void clear_string(DynamicString* string)
{
    check(string);

    if (string->chars)
        free(string->chars);

    string->chars = NULL;
    string->length = 0;
    string->size = 0;
}
