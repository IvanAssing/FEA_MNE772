#version 450 core

uniform mat4 mvp_matrix;

in vec2 position;
in vec3 color;


out VertexData{
    vec4 color;
} vs_out;


void main()
{
    vec4 pos = vec4(position, 0.0, 1.0);
    gl_Position = mvp_matrix * pos;

    vs_out.color = vec4(color,0.8);
}
