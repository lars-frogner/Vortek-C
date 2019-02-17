#include "error.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>


void print_info_message(const char* message, ...)
{
    va_list args;
    printf("Info: ");
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\n");
}

void print_warning_message(const char* message, ...)
{
    va_list args;
    fprintf(stderr, "Warning: ");
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void print_error_message(const char* message, ...)
{
    va_list args;
    fprintf(stderr, "Error: ");
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void print_severe_message(const char* message, ...)
{
    va_list args;
    fprintf(stderr, "Fatal error: ");
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");

    exit(EXIT_FAILURE);
}
