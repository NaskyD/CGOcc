#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout(points) in;
layout(triangle_strip, max_vertices = 18) out;

float scaleFactor = 0.3;
float height = 10.0;

vec4 scale(vec4 vertex)
{
	return vertex * scaleFactor;
}

vec4 transform(vec4 vertex)
{
	vertex.y = vertex.y + height;
	return projection * view * model * vertex;
}

void main() {
//transformation Coord
	vec4 first = gl_in[0].gl_Position;
	
	//## side_faces ##
	//1
    gl_Position = transform(first + scale(vec4(-1.0, 0.0, -1.0, 0.0)));
    EmitVertex();
	
	//2
	gl_Position = transform(first + scale(vec4(-1.0, 1.0, -1.0, 0.0)));
    EmitVertex();
	
	//3
	gl_Position = transform(first + scale(vec4(1.0, 0.0, -1.0, 0.0)));
    EmitVertex();
	
	//4
	gl_Position = transform(first + scale(vec4(1.0, 1.0, -1.0, 0.0)));
    EmitVertex();
	
	//5
	gl_Position = transform(first + scale(vec4(1.0, 0.0, 1.0, 0.0)));
    EmitVertex();
	
	//6
	gl_Position = transform(first + scale(vec4(1.0, 1.0, 1.0, 0.0)));
    EmitVertex();
	
	//7
	gl_Position = transform(first + scale(vec4(-1.0, 0.0, 1.0, 0.0)));
    EmitVertex();	
	
	//8
	gl_Position = transform(first + scale(vec4(-1.0, 1.0, 1.0, 0.0)));
    EmitVertex();
	
	//1
	gl_Position = transform(first + scale(vec4(-1.0, 0.0, -1.0, 0.0)));
    EmitVertex();
	
	//2
	gl_Position = transform(first + scale(vec4(-1.0, 1.0, -1.0, 0.0)));
    EmitVertex();
	EndPrimitive();
	
	//## bottom_face ##
	//1
	gl_Position = transform(first + scale(vec4(-1.0, 0.0, -1.0, 0.0)));
    EmitVertex();
	
	//3
	gl_Position = transform(first + scale(vec4(1.0, 0.0, -1.0, 0.0)));
    EmitVertex();
	
	//7
	gl_Position = transform(first + scale(vec4(-1.0, 1.0, 1.0, 0.0)));
    EmitVertex();
	
	//5
	gl_Position = transform(first + scale(vec4(1.0, 0.0, 1.0, 0.0)));
    EmitVertex();
	EndPrimitive();
	
	//## top_face ##
	//2
	gl_Position = transform(first + scale(vec4(-1.0, 1.0, -1.0, 0.0)));
    EmitVertex();
	
	//4
	gl_Position = transform(first + scale(vec4(1.0, 1.0, -1.0, 0.0)));
    EmitVertex();
	
	//8
	gl_Position = transform(first + scale(vec4(-1.0, 1.0, 1.0, 0.0)));
    EmitVertex();
	
	//6
	gl_Position = transform(first + scale(vec4(1.0, 1.0, 1.0, 0.0)));
    EmitVertex();
	EndPrimitive();
}  