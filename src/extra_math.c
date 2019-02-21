#include "extra_math.h"

#include "error.h"

#include <math.h>


const double PI = 3.14159265358979323846;


int imax(int a, int b)
{
    return (a >= b) ? a : b;
}

int imin(int a, int b)
{
    return (a <= b) ? a : b;
}

size_t max_size(size_t a, size_t b)
{
    return (a >= b) ? a : b;
}

size_t min_size(size_t a, size_t b)
{
    return (a >= b) ? a : b;
}

unsigned int argfmax3(float x, float y, float z)
{
    // Returns the index of the largest number. When multiple numbers are equal
    // and largest the first index of these numbers is returned.
    return (z > y) ? ((z > x) ? 2 : 0) : ((y > x) ? 1 : 0);
}

int sign(float x)
{
    return (x >= 0) ? 1 : -1;
}

int signum(float x)
{
    return (x > 0) ? 1 : ((x < 0) ? -1 : 0);
}

float clamp(float x, float lower, float upper)
{
    return (x > lower) ? ((x < upper) ? x : upper) : lower;
}

float cotangent(float angle)
{
    const float tan_angle = tanf(angle);
    assert(tan_angle != 0.0f);
    return 1.0f/tan_angle;
}

float degrees_to_radians(float degrees)
{
    return degrees*(float)(PI/180);
}

float radians_to_regrees(float radians)
{
    return radians*(float)(180/PI);
}
