#version 430

in vec2 v_screenAlignedQuad_UV;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;

layout (location = 0) out vec4 FragColor;
 
void main()
{
	vec4 mask = texture(texture0, v_screenAlignedQuad_UV);
	vec4 outlineHints = texture(texture1, v_screenAlignedQuad_UV);
	vec4 adaptiveTransparancy = texture(texture2, v_screenAlignedQuad_UV);

	FragColor = mix(adaptiveTransparancy, outlineHints, smoothstep(0.0, 1.0, mask));
}