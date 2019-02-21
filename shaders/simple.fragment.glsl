#version 400

in vec3 ex_tex_coord;
out vec4 out_color;

uniform sampler3D texture_0;
uniform sampler1D transfer_function_0;

void main(void)
{
    float field_value;
    field_value = texture(texture_0, ex_tex_coord).r;
    out_color = texture(transfer_function_0, field_value);
}
