#version 430

layout (location = 11) uniform sampler2D cityTexture;

in vec2 v_screenAlignedQuad_UV;

layout (location = 0) out vec4 FragColor;
 
void main()
{
	vec4 color = texture(cityTexture, v_screenAlignedQuad_UV);
	
	if (color.a < 0.3)
	{
		discard;
	}
	
	FragColor = vec4(0.0);
}