#ifndef EXTRA_MATH_H
#define EXTRA_MATH_H

#include <stddef.h>

extern const double PI;

int max(int a, int b);
int min(int a, int b);

size_t max_size(size_t a, size_t b);
size_t min_size(size_t a, size_t b);

unsigned int argfmax3(float x, float y, float z);

int sign(float x);
int signum(float x);

float cotangent(float angle);
float degrees_to_radians(float degrees);
float radians_to_regrees(float radians);

#endif