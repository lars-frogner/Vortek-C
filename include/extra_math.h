#ifndef EXTRA_MATH_H
#define EXTRA_MATH_H

#include <stddef.h>

extern const double PI;

int imax(int a, int b);
int imin(int a, int b);

size_t max_size(size_t a, size_t b);
size_t min_size(size_t a, size_t b);

unsigned int argfmax3(float x, float y, float z);

int sign(float x);
int signum(float x);

float clamp(float x, float lower, float upper);

float cotangent(float angle);
float degrees_to_radians(float degrees);
float radians_to_regrees(float radians);

int is_prime(size_t n);
size_t next_prime(size_t n);

#endif
