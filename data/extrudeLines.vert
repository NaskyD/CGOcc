#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout (location = 0) in vec3 vertexPos;

void main()
{
    mat4 transform = projection * view * model;
    gl_Position = vec4(transform * vec4(vertexPos, 1.0));
}