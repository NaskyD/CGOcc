#version 430

in vec2 v_screenAlignedQuad_UV;

uniform sampler2D texture0;
uniform sampler2D texture1;

layout (location = 0) out vec4 FragColor;
 
void main()
{
	vec4 enhancedEdges = texture(texture0, v_screenAlignedQuad_UV);
	vec4 scene = texture(texture1, v_screenAlignedQuad_UV);

	FragColor = mix(scene, -enhancedEdges, step(0.5, enhancedEdges.r) * 0.3);
}