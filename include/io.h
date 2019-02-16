#ifndef IO_H
#define IO_H

typedef struct FloatField
{
    float* data;
    unsigned int size_x;
    unsigned int size_y;
    unsigned int size_z;

} FloatField;


void print_info(const char* message, ...);
void print_error(const char* message, ...);

char* read_text_file(const char* filename);

FloatField read_bifrost_field(const char* data_filename, const char* header_filename);

#endif
