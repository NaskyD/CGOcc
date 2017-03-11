#version 430

in vec2 v_screenAlignedQuad_UV;

layout (location = 11) uniform sampler2D cityTexture;
uniform sampler2D texture0;			//city without houses
uniform sampler2D texture2;			//mask

out vec4 FragColor;

void main()
{
	vec4 cityColor = texture(cityTexture, v_screenAlignedQuad_UV);
	vec4 cityNoHousesColor = texture(texture0, v_screenAlignedQuad_UV);
	vec4 maskColor = texture(texture2, v_screenAlignedQuad_UV);
	
	FragColor = mix(cityColor, cityNoHousesColor, smoothstep(0.0, 0.4, maskColor.a));
}