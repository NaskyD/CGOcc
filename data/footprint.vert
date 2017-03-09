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
	
	float height = step(0.1, vertexPos.y) * 0.5;
    gl_Position = vec4(transform * vec4(vertexPos.x, height, vertexPos.z, 1.0));
}