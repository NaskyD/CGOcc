#version 430

uniform vec3 clearColor;
uniform vec3 lightVector;
uniform vec3 lightVector2;
uniform bool useNormals;
uniform bool renderDepthValueForTextureUsage;

uniform sampler2D texture0;			//top fence components
coherent uniform layout(size4x32) image2DArray aBufferImg;
coherent uniform layout(size1x32) image2DArray aBufferAlphaImg;
coherent uniform layout(size1x32) uimage2D aBufferIndexImg;
coherent uniform layout(size1x32) uimage2D typeIdImg;

in vec2 v_screenAlignedQuad_UV;
in vec3 v_normals;

layout (location = 0) out vec4 FragColor;

//has to be const and can not be a uniform input here
const int maxLayer = 16;
 
void main()
{
	ivec2 fragCoords = ivec2(gl_FragCoord.xy);
	int maxIndex = int(imageLoad(aBufferIndexImg, fragCoords).x);
	
	uint typeNumber = uint(imageLoad(typeIdImg, fragCoords));
	
	//Load fragments into a local memory array for sorting
	vec4 fragmentList[maxLayer];
	vec4 alphaList[maxLayer];
	for(int i = 0; i < maxLayer; i++)
	{
		fragmentList[i] = vec4(0.0);
		alphaList[i] = vec4(0.0);
		
		if(i < maxIndex)
		{
			fragmentList[i] =  imageLoad(aBufferImg, ivec3(fragCoords, i));
			alphaList[i] =  imageLoad(aBufferAlphaImg, ivec3(fragCoords, i));
		}
	}

	//sort fragments and corresponding type (bubble sort)
	for (int i = (maxIndex - 2); i >= 0; --i)
	{
		for (int j = 0; j <= i; ++j)
		{
		  if (fragmentList[j].a > fragmentList[j+1].a)
		  {
			//switch fragment color
			vec4 tempColor = fragmentList[j+1];
			fragmentList[j+1] = fragmentList[j];
			fragmentList[j] = tempColor;
			
			//switch fragment alpha values
			tempColor = alphaList[j+1];
			alphaList[j+1] = alphaList[j];
			alphaList[j] = tempColor;
			
			//switch fragment type
			uint tempTypeJ1 = (typeNumber >> (2 * (j + 1))) & 0x3;
			uint tempTypeJ2 = (typeNumber >> (2 * j)) & 0x3;
			typeNumber = typeNumber & (~(0x3 << (2 * j))) | ((tempTypeJ1) << (2 * j));
			typeNumber = typeNumber & (~(0x3 << (2 * (j + 1)))) | ((tempTypeJ1) << (2 * (j + 1)));
		  }
		}
	}
	
	float transparancy = 1.0;
	vec3 aBufferTexelSum = clearColor;
	for(int i = maxIndex-1; i >= 0; i--)
	{
		transparancy = alphaList[i].r;
		aBufferTexelSum = (1 - transparancy) * aBufferTexelSum + transparancy * fragmentList[i].rgb;
	}
	
	vec4 fence = texture(texture0, v_screenAlignedQuad_UV);
	
	float validFence = 1-step(0.5, fence.a);
	
	vec3 color = mix(aBufferTexelSum, fence.rgb, validFence);
	
	if (bool(validFence) && fragmentList[0].a < fence.a)
	{
		color = mix(aBufferTexelSum, fence.rgb, 0.4);
	}
	
	FragColor = vec4(color, 1.0);
	//FragColor = vec4(0.0, step(0.5, fence.a), 0.0, 1.0);
	//FragColor = vec4(vec3(fence.a), 1.0);
	
	//FragColor = vec4(vec3(fragmentList[0].a), 1.0);
}