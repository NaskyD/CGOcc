#version 330 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

void main() {
//transformation Coord
	vec4 first = gl_in[0].gl_Position;
	vec4 second = gl_in[1].gl_Position;
    gl_Position = first + vec4(0.0, -10.0, 0.0, 0.0);
    EmitVertex();
	
	gl_Position = first + vec4(0.0, 15.0, 0.0, 0.0);
    EmitVertex();
	
	gl_Position = second + vec4(0.0, -10.0, 0.0, 0.0);
    EmitVertex();
	
	gl_Position = second + vec4(0.0, 15.0, 0.0, 0.0);
    EmitVertex();
	EndPrimitive();
}  