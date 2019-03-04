#ifndef DYNAMIC_STRING_H
#define DYNAMIC_STRING_H

#include <stddef.h>

typedef struct DynamicString
{
    char* chars;
    size_t size;
    size_t length;
} DynamicString;

DynamicString create_string(void);
void set_string(DynamicString* string, const char* chars, ...);
void extend_string(DynamicString* string, const char* extension_chars, ...);
void clear_string(DynamicString* string);

#endif
