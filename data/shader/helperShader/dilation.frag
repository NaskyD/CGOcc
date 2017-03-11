#version 430

in vec2 v_screenAlignedQuad_UV;

uniform int windowWidth;
uniform int windowHeight;
uniform sampler2D texture0;

layout (location = 0) out vec4 FragColor;

const float pixelSizeX = 1.0/float(windowWidth);
const float pixelSizeY = 1.0/float(windowHeight);
const int rKernel = 16;
 
void main()
{	
	float valid = 0.0;
	for (int dx = -rKernel; dx <= rKernel; ++dx)
	{
		for (int dy = -rKernel; dy <= rKernel; ++dy)
		{
			vec2 textureKernel = vec2(float(dx) * pixelSizeX, float(dy) * pixelSizeY);
			float boxTexel = texture(texture0, v_screenAlignedQuad_UV + textureKernel).a;
			valid = valid + ceil(1-boxTexel);
		}
	}
	
	valid = valid/float(rKernel*rKernel/2);
	FragColor = vec4(valid);
}