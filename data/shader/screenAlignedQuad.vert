#version 430

layout (location = 0) in vec3 screenAlignedQuad_UV;

out vec2 v_screenAlignedQuad_UV;

void main()
{
	v_screenAlignedQuad_UV = screenAlignedQuad_UV.xy * 0.5 + vec2(0.5);
	gl_Position = vec4(screenAlignedQuad_UV, 1.0);
}