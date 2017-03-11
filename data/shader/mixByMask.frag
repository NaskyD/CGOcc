#version 430

in vec2 v_screenAlignedQuad_UV;


uniform sampler2D texture0;		//first visualization
uniform sampler2D texture1;		//second visualization
uniform sampler2D texture2;		//mask

layout (location = 0) out vec4 FragColor;
 
void main()
{
	vec4 first_vis = texture(texture0, v_screenAlignedQuad_UV);
	vec4 second_vis = texture(texture1, v_screenAlignedQuad_UV);
	vec4 mask = texture(texture2, v_screenAlignedQuad_UV);

	FragColor = mix(second_vis, first_vis, smoothstep(0.0, 1.0, mask));
}