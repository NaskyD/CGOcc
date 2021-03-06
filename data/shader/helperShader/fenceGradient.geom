#version 330 core

uniform mat4 transform;
uniform float geometryHeight;

layout(lines) in;

layout(triangle_strip, max_vertices = 4) out;

out float g_currentHeight;

vec4 toWorldPosition(vec4 vertex)
{
	return transform * vertex;
} 

void main() {
//transformation Coord
	vec4 first = gl_in[0].gl_Position;
	vec4 second = gl_in[1].gl_Position;
	
    gl_Position = toWorldPosition(first + vec4(0.0, 0.0, 0.0, 0.0));
	g_currentHeight = first.y;
    EmitVertex();
	
	gl_Position = toWorldPosition(first + vec4(0.0, geometryHeight, 0.0, 0.0));
	g_currentHeight = second.y + geometryHeight;
    EmitVertex();
	
	gl_Position = toWorldPosition(second + vec4(0.0, 0.0, 0.0, 0.0));
	g_currentHeight = first.y;
    EmitVertex();
	
	gl_Position = toWorldPosition(second + vec4(0.0, geometryHeight, 0.0, 0.0));
	g_currentHeight = second.y + geometryHeight;
    EmitVertex();
	EndPrimitive();
}  