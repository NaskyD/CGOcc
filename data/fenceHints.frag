#version 430

in vec2 v_screenAlignedQuad_UV;
in vec3 v_normals;

uniform vec3 lightVector;
uniform vec3 lightVector2;
uniform bool useNormals;
uniform bool renderDepthValueForTextureUsage;

uniform sampler2D texture0;			//top fence components
uniform sampler2D texture1;			//city

layout (location = 0) out vec4 FragColor;
 
void main()
{
	//Linearization of the depth value
	float f = 200.0;
	float n = 0.1f;
	float depthValue = (2 * n) / (f + n - gl_FragCoord.z * (f - n));
	
	vec4 color;
	vec4 fence = texture(texture0, v_screenAlignedQuad_UV);
	vec4 city = texture(texture1, v_screenAlignedQuad_UV);
	
	//TODO: geht das nicht schöner?
	
	color = mix(city, fence, step(fence.a, 1.0 - fence.a));//1.0 - fence.a);
	
	if (bool(step(fence.a, 1.0 - fence.a)) && city.a <= fence.a)
	{
		color = mix(city, fence, 0.4);
	}

	FragColor = color;
}