#ifndef COLORS_H
#define COLORS_H

enum standard_color {COLOR_BLACK   = 0,
                     COLOR_WHITE   = 1,
                     COLOR_RED     = 2,
                     COLOR_GREEN   = 3,
                     COLOR_BLUE    = 4,
                     COLOR_CYAN    = 5,
                     COLOR_MAGENTA = 6,
                     COLOR_YELLOW  = 7};

typedef struct Color
{
    float r;
    float g;
    float b;
    float a;
} Color;

Color create_color(float red, float green, float blue, float alpha);
Color create_standard_color(enum standard_color color, float alpha);
Color create_hex_color(int hex_color, float alpha);
const Color* get_full_standard_color(enum standard_color color);

#endif
