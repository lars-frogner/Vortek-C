#ifndef IO_H
#define IO_H

#include <stddef.h>

int is_little_endian(void);

char* read_text_file(const char* filename);
void* read_binary_file(const char* filename, size_t length, size_t element_size);

int find_int_entry_in_header(const char* header, const char* entry_name, const char* separator);
float find_float_entry_in_header(const char* header, const char* entry_name, const char* separator);
char find_char_entry_in_header(const char* header, const char* entry_name, const char* separator);

#endif
