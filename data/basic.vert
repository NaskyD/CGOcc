#version 430

uniform mat4 transform;
uniform bool useNormals;

layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec3 normals;

out vec3 v_normals;

void main()
{
	if(useNormals)
	{
		v_normals = normals;
	}
	else
	{
		v_normals = vec3(1.0);
	}

    gl_Position = vec4(transform * vec4(vertexPos, 1.0));
}