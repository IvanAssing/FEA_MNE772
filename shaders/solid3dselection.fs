#version 450 core


in VertexData{
    vec4 color;
}vsout;

out vec4 FragColor;

void main()
{
        FragColor = vsout.color;
}
