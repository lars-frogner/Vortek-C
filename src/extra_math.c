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

unsigned int uimax(unsigned int a, unsigned int b)
{
    return (a >= b) ? a : b;
}

unsigned int uimin(unsigned int a, unsigned int b)
{
    return (a <= b) ? a : b;
}

size_t max_size_t(size_t a, size_t b)
{
    return (a >= b) ? a : b;
}

size_t min_size_t(size_t a, size_t b)
{
    return (a <= b) ? a : b;
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

size_t pow2_size_t(unsigned int exponent)
{
    return 1 << (size_t)exponent;
}

unsigned int floored_log2_size_t(size_t number)
{
    assert(number > 0);
    unsigned int result = 0;
    while (number >>= 1)
        result++;
    return result;
}

size_t closest_ge_pow2_size_t(size_t number)
{
    // Returns the next power of 2 number that is either equal to or greater than the given number
    assert(number > 0);
    const size_t result = pow2_size_t(floored_log2_size_t(number));
    return (result < number) ? 2*result : result;
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

int is_prime(size_t n)
{
    if (n < 2)
        return -1; // Return negative number for invalid input

    if (n < 4)
        return 1;

    if ((n % 2) == 0)
        return 0;

    const float limit_float = sqrtf((float)n);
    const size_t limit = (size_t)limit_float;
    size_t i;

    for (i = 3; i <= limit; i += 2)
    {
        if ((n % i) == 0)
            return 0;
    }

    return 1;
}

size_t next_prime(size_t n)
{
    // Returns the next prime that is greater than or equal to the given number
    while (is_prime(n) != 1)
        n++;
    return n;
}
