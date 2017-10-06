#version 450 core

uniform mat4 mvp_matrix;

layout (location = 0) in vec3 position;

layout (location = 1) in vec3 color;


out VertexData{
    vec4 color;
} vs_out;


void main()
{
    vec4 pos = vec4(position, 1.0);
    gl_Position = mvp_matrix * pos;
    vec4 colorRGBA = vec4(color, 1.0);
    vs_out.color = colorRGBA;
}
