#version 430

uniform vec4 specifiedColor;
uniform bool renderDepthValueForTextureUsage;

layout (location = 0) out vec4 FragColor;

//TODO: set from outside
float height = 10.0;
 
void main()
{
	//Linearization of the depth value
	float f = 200.0;
	float n = 0.1f;
	float depthValue = (2 * n) / (f + n - gl_FragCoord.z * (f - n));
	
	vec4 color = specifiedColor;
	
	//TODO!!!!!!!!!!!!!!
	float fragmentHeight = 1.0;
	color.a = smoothstep(0.0, 1.0, fragmentHeight);
	
	if(renderDepthValueForTextureUsage)
	{
		FragColor = vec4(color.rgb, depthValue);
	}
	else
	{
		FragColor = color;
	}
}