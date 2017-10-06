#version 450 core

uniform mat4 mvp_matrix;

layout (location = 0) in vec2 position;
uniform float posz;

void main()
{
    vec4 pos = vec4(position, posz, 1.0);
    gl_Position = mvp_matrix * pos;
}
