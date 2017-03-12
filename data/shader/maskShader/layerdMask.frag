#version 430

coherent uniform layout(size1x32) uimage2D aBufferIndexImg;

in vec2 v_screenAlignedQuad_UV;

layout (location = 0) out vec4 FragColor;
 
void main()
{
	ivec2 fragCoords = ivec2(gl_FragCoord.xy);
	uint maxIndex = imageLoad(aBufferIndexImg, fragCoords).x;
	
	vec4 color = vec4(vec3(1.0), 1.0);
	
	if (maxIndex > 5)
	{
		color = vec4(0.0);
	}
	
	FragColor = color;
}