#version 430

in float g_currentHeight;
in vec3 pointColor;

uniform vec4 specifiedColor;
uniform bool renderDepthValueForTextureUsage;

layout (location = 0) out vec4 FragColor;

//TODO: set from outside
const float minHeight = -0.7704;
const float maxHeight = 14.0;
 
void main()
{
	//Linearization of the depth value
	float f = 200.0;
	float n = 0.1f;
	float depthValue = (2 * n) / (f + n - gl_FragCoord.z * (f - n));
	
	vec4 color = specifiedColor;
	
	float fragmentRelativeHeight = (g_currentHeight - minHeight) / (maxHeight - minHeight);
	float factor = 1 - fragmentRelativeHeight;
	color.a = pow(smoothstep(0.0, 1.0, factor), 2);
	
	//TODO: remove, if it really is not important
	//if(renderDepthValueForTextureUsage)
	//{
		//FragColor = vec4(color.rgb, depthValue);
		//FragColor = vec4(vec3(color.a), color.a);
		//FragColor = color;
	//}
	//else
	//{
		FragColor = color;
	//}
}