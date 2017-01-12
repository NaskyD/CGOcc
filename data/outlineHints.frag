#version 430

uniform sampler2D texture0;		//city
uniform sampler2D texture1;		//halo line1
uniform sampler2D texture2;		//halo line2

in vec2 v_screenAlignedQuad_UV;

out vec4 FragColor;

void main()
{
	vec4 cityColor = vec4(texture(texture0, v_screenAlignedQuad_UV));
	vec4 haloLine1 = vec4(texture(texture1, v_screenAlignedQuad_UV));
	vec4 haloLine2 = vec4(texture(texture2, v_screenAlignedQuad_UV));
	vec4 blendColor = mix(cityColor, haloLine1, haloLine1.a);
	blendColor = mix(blendColor, haloLine2, haloLine2.a);
	
	FragColor = blendColor;
}