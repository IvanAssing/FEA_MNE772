#version 450 core

uniform mat4 mvp_matrix;

in vec3 position;
in vec3 color;

out VertexData{
    vec4 color;
}vsout;

void main()
{
    vec4 pos = vec4(position, 1.0);
    gl_Position = mvp_matrix * pos;

    vsout.color = vec4(color, 1.0);
}
