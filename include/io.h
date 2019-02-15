#ifndef IO_H
#define IO_H

enum file_type
{
    BINARY,
    ASCII
};

typedef struct ScalarField3Df
{
    float* data;
    int size_x;
    int size_y;
    int size_z;

} ScalarField3Df;


void print_info(const char* message, ...);
void print_error(const char* message, ...);

char* read_file(const char* filename, enum file_type type);

#endif
