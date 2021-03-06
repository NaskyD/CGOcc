#version 430

uniform vec4 specifiedColor;
uniform bool useNormals;
uniform bool renderDepthValueForTextureUsage;

layout (location = 10) uniform samplerCube cubeMap;

in vec3 v_normals;

layout (location = 0) out vec4 FragColor;
 
void main()
{
	//Linearization of the depth value
	float f = 200.0;
	float n = 0.1f;
	float depthValue = (2 * n) / (f + n - gl_FragCoord.z * (f - n));
	
	vec4 color = specifiedColor;
	
	if(useNormals)
	{
		color = vec4(texture(cubeMap, v_normals).xyz * 0.8, 1.0);
	}
	
	if(renderDepthValueForTextureUsage)
	{
		FragColor = vec4(color.rgb, depthValue);
	}
	else
	{
		FragColor = color;
	}
}