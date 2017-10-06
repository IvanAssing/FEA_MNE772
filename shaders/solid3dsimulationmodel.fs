#version 450 core

out vec4 FragColor;
uniform bool isSolidColor;

in VertexData{
    vec3 color;
    vec3 normal;
    vec3 fragPos;
}vsout;


void main()
{
    if(isSolidColor)
        FragColor  = vec4(1.0, 1.0, 1.0, 1.0); // white
    else
    FragColor = vec4(vsout.color, 1.0);
}
