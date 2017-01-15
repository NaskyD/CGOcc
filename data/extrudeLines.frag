#version 330 core

uniform vec4 specifiedColor;
uniform bool renderDepthValueForTextureUsage;

out vec4 FragColor;

void main()
{
	//Linearization of the depth value
	float f = 200.0;
	float n = 0.1f;
	float depthValue = (2 * n) / (f + n - gl_FragCoord.z * (f - n));
	
	vec4 color = specifiedColor;
	
	if(renderDepthValueForTextureUsage)
	{
		FragColor = vec4(color.rgb, depthValue);
	}
	else
	{
		FragColor = color;
	}  
}  