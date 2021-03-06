#version 430

in vec2 v_screenAlignedQuad_UV;

uniform int windowWidth;
uniform int windowHeight;
uniform int kernelSize;
uniform sampler2D texture4;		//simple mask

layout (location = 0) out vec4 FragColor;

const float pixelSizeX = 1.0/float(windowWidth);
const float pixelSizeY = 1.0/float(windowHeight);
 
void main()
{	
	int abs_kernelSize = abs(kernelSize);
	
	float valid = 0.0;
	for (int dx = -abs_kernelSize; dx <= abs_kernelSize; ++dx)
	{
		for (int dy = -abs_kernelSize; dy <= abs_kernelSize; ++dy)
		{
			vec2 textureKernel = vec2(float(dx) * pixelSizeX, float(dy) * pixelSizeY);
			float boxTexel = texture(texture4, v_screenAlignedQuad_UV + textureKernel).x;
			
			valid = valid + 1-boxTexel;
		}
	}
	
	float threshhold = (2*abs_kernelSize+1)*(2*abs_kernelSize+1) / abs_kernelSize;

	FragColor = vec4(vec3(step(threshhold, valid)), 1.0);	
}