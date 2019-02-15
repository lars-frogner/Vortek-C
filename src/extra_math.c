#include "extra_math.h"

#include <math.h>


const double PI = 3.14159265358979323846;


int max(int a, int b)
{
    return (a >= b) ? a : b;
}

int min(int a, int b)
{
    return (a <= b) ? a : b;
}

unsigned int umax(unsigned int a, unsigned int b)
{
    return (a >= b) ? a : b;
}

unsigned int umin(unsigned int a, unsigned int b)
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

float cotangent(float angle)
{
  return (float)(1.0/tanf(angle));
}

float degrees_to_radians(float degrees)
{
  return degrees*(float)(PI/180);
}

float radians_to_regrees(float radians)
{
  return radians*(float)(180/PI);
}
