#version 400

in vec3 ex_tex_coord;
out vec4 out_color;

uniform sampler3D texture0;

void main(void)
{
    float intensity;
    intensity = texture(texture0, ex_tex_coord).r;
    out_color.x = intensity;
    out_color.y = intensity;
    out_color.z = intensity;
    out_color.w = intensity;
}
