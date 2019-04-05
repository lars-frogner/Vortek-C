#include "colors.h"

#include "error.h"


#define NUMBER_OF_STANDARD_COLORS 8


static const Color full_standard_colors[NUMBER_OF_STANDARD_COLORS] =
    {{0.0f, 0.0f, 0.0f, 1.0f},  // Black
     {1.0f, 1.0f, 1.0f, 1.0f},  // White
     {1.0f, 0.0f, 0.0f, 1.0f},  // Red
     {0.0f, 1.0f, 0.0f, 1.0f},  // Green
     {0.0f, 0.0f, 1.0f, 1.0f},  // Blue
     {0.0f, 1.0f, 1.0f, 1.0f},  // Cyan
     {1.0f, 0.0f, 1.0f, 1.0f},  // Magenta
     {1.0f, 1.0f, 0.0f, 1.0f}}; // Yellow


Color create_color(float red, float green, float blue, float alpha)
{
    check(red   >= 0 && red   <= 1);
    check(green >= 0 && green <= 1);
    check(blue  >= 0 && blue  <= 1);
    check(alpha >= 0 && alpha <= 1);

    Color result = {red, green, blue, alpha};
    return result;
}

Color create_standard_color(enum standard_color color, float alpha)
{
    check(alpha >= 0 && alpha <= 1);
    const Color* const full_standard_color = get_full_standard_color(color);
    Color result = {full_standard_color->r, full_standard_color->g, full_standard_color->b, alpha};
    return result;
}

Color create_hex_color(int hex_color, float alpha)
{
    check(alpha >= 0 && alpha <= 1);
    Color result = {(float)((hex_color >> 16) & 0xFF)/255.0f,
                    (float)((hex_color >>  8) & 0xFF)/255.0f,
                    (float)((hex_color >>  0) & 0xFF)/255.0f,
                    alpha};
    return result;
}

const Color* get_full_standard_color(enum standard_color color)
{
    return full_standard_colors + color;
}
