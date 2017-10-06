#version 450 core


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec3 position;
in vec3 normal;
in vec3 color;


out VertexData{
    vec3 color;
    vec3 normal;
    vec3 fragPos;
}vsout;


void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);

    vsout.color = color;
    vsout.normal = mat3(transpose(inverse(model))) *normal;
    vsout.fragPos = vec3(model * vec4(position, 1.0));;

}
