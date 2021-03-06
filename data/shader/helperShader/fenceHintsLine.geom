#version 330 core

uniform mat4 transform;
uniform float geometryHeight;

layout(lines) in;
layout(line_strip, max_vertices = 4) out;

vec4 toWorldPosition(vec4 vertex)
{
	vertex.y = vertex.y + geometryHeight;
	return transform * vertex;
} 

void main() {
//transformation Coord
	vec4 first = gl_in[0].gl_Position;
	vec4 second = gl_in[1].gl_Position;
    gl_Position = toWorldPosition(first);
    EmitVertex();
	
	gl_Position = toWorldPosition(second);
    EmitVertex();
	EndPrimitive();
}  