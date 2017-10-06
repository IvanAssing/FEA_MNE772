#version 450 core



uniform mat4 mvp_matrix;

in vec3 position;
in vec3 color;


out VertexData{
    vec3 color;
    vec3 normal;
    vec3 fragPos;
}vsout;


void main()
{
    gl_Position = mvp_matrix * vec4(position, 1.0);

    vsout.color = color;

}
