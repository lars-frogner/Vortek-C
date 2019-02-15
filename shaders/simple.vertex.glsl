#version 400

layout(location=0) in vec4 in_position;
layout(location=1) in vec3 in_tex_coord;
out vec4 ex_color;

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

void main()
{
    gl_Position = (projection_matrix*view_matrix*model_matrix)*in_position;
    ex_color.xyz = in_tex_coord;
}
