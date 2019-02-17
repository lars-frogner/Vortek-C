#version 400

layout(location=0) in vec4 in_position;
layout(location=1) in vec3 in_tex_coord;
out vec3 ex_tex_coord;

uniform mat4 model_view_projection_matrix;

void main(void)
{
    gl_Position = model_view_projection_matrix*in_position;
    ex_tex_coord = in_tex_coord;
}
