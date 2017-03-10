#version 430

uniform vec3 lightVector;
uniform vec3 lightVector2;
uniform vec4 specifiedColor;
uniform bool useNormals;
uniform bool renderDepthValueForTextureUsage;

uniform samplerCube cubeMap;

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
		//float NdotL  = max(dot(normalize(v_normals), normalize(lightVector)), 0.0);
		//float NdotL2 = max(dot(normalize(v_normals), normalize(lightVector2)), 0.0);
		//vec3 diffuseColor = vec3(0.8, 0.8, 0.8);
		//vec3 diffuseColor2 = vec3(0.3, 0.3, 0.3);
		//vec3 ambient = vec3(0.2, 0.2, 0.2);
		//color = color * vec4(NdotL2 * diffuseColor + NdotL * diffuseColor2 + ambient, 1.0);
		
		color = vec4(texture(cubeMap, v_normals).xyz * 0.7, 1.0);
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