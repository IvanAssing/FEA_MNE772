#version 450 core

in VertexData{
    vec4 color;
} vs_out;

out vec4 FragColor;

void main()
{
        FragColor = vs_out.color;
}
