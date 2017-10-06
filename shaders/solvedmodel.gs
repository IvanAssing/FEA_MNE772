#version 450 core

uniform mat4 mvp_matrix;

layout(lines) in;
layout (line_strip, max_vertices = 2) out;

in VertexData{
vec4 color;
} gs_in[];

out vec4 fscolor;

void main()
{
fscolor = gs_in[1].color;
gl_Position = gl_in[0].gl_Position;
EmitVertex();
fscolor = gs_in[1].color;
gl_Position = gl_in[1].gl_Position;
EmitVertex();
EndPrimitive();
}

