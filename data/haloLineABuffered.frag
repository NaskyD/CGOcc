#version 430

layout (location = 0) out vec4 FragColor;

uniform int windowWidth;
uniform int windowHeight;
uniform vec4 haloColor;
uniform sampler2D texture0;		//city texture
coherent uniform layout(size4x32) image2DArray aBufferImg;
coherent uniform layout(size1x32) uimage2D aBufferIndexImg;

in vec2 v_screenAlignedQuad_UV;

const float pixelSizeX = 1.0/float(windowWidth);
const float pixelSizeY = 1.0/float(windowHeight);
const int rKernel = 3;
const int maxLayer = 16;

void main()
{
	vec4 actualCityFragment = texture(texture0, v_screenAlignedQuad_UV);
	
	ivec2 fragCoords = ivec2(gl_FragCoord.xy);
	uint maxIndex = imageLoad(aBufferIndexImg, fragCoords).x;
	
	float envCityOccludesLineCount = 0.0;
	float lineAreaCount = 0.0;
	float isHalo = 0.0;

	for(int i = 0; i < maxIndex; i++)
	{
		for (int dx = -rKernel; dx <= rKernel; ++dx)
		{
			for (int dy = -rKernel; dy <= rKernel; ++dy)
			{
				vec2 textureKernel = vec2(float(dx) * pixelSizeX, float(dy) * pixelSizeY);
				vec2 aBufferKernel = vec2(float(dx), float(dy));
				
				vec4 envLineTex = imageLoad(aBufferImg, ivec3((fragCoords + aBufferKernel), i));
				vec4 envCityTex = texture(texture0, v_screenAlignedQuad_UV + textureKernel);
				
				//city geometry occludes line geometry
				if(envLineTex.a >= envCityTex.a && envLineTex.r > 0.5)
				{
					envCityOccludesLineCount = envCityOccludesLineCount + 1.0;
				}
				
				//line geometry is visible
				if(envLineTex.a <= envCityTex.a && envLineTex.r > 0.5)
				{
					//test whether this is a valid line to count against
					float valid = 1;
					if(envLineTex.a < actualCityFragment.a)
					{
						valid = 0;
					}
					lineAreaCount = lineAreaCount + 1.0 * valid;
				}
			}
		}
	}
	isHalo = step(6.0, envCityOccludesLineCount) * step(4.0, lineAreaCount);
	if(isHalo < 0.5)
	{
		discard;
	}
	
	FragColor = haloColor;
}