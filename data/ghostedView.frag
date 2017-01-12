#version 430

in vec2 v_screenAlignedQuad_UV;

uniform sampler2D texture0;			//city with houses
uniform sampler2D texture1;			//city without houses
uniform sampler2D texture3;			//mask

out vec4 FragColor;

void main()
{
	vec4 cityColor = texture(texture0, v_screenAlignedQuad_UV);
	vec4 cityNoHousesColor = texture(texture1, v_screenAlignedQuad_UV);
	vec4 maskColor = texture(texture3, v_screenAlignedQuad_UV);
	
	FragColor = mix(cityColor, cityNoHousesColor, smoothstep(0.0, 0.4, maskColor.a));
}