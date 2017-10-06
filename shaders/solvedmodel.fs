#version 450 core

out vec4 FragColor;
in vec4 fscolor;

void main()
{
        //FragColor = vec4(0.0, 1.0, 0.0, 1.0);
        FragColor = fscolor;
}
