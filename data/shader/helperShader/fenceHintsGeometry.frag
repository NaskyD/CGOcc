#version 430

uniform vec4 specifiedColor;
uniform bool useNormals;

layout (location = 10) uniform samplerCube cubeMap;

in vec3 v_normals;

layout (location = 0) out vec4 FragColor;
 
void main()
{	
	vec4 color = specifiedColor;
	
	if(useNormals)
	{
		color = vec4(texture(cubeMap, v_normals).xyz * 0.8, 1.0);
	}
	
	FragColor = vec4(color.rgb, 0.0);
}