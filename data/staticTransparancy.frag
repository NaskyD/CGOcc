#version 430

in vec2 v_screenAlignedQuad_UV;

uniform vec3 clearColor;
coherent uniform layout(size4x32) image2DArray aBufferImg;
coherent uniform layout(size1x32) uimage2D aBufferIndexImg;
coherent uniform layout(size1x32) uimage2D typeIdImg;

out vec4 FragColor;

//has to be a const value
const int maxLayer = 16;

void main()
{
	ivec2 fragCoords = ivec2(gl_FragCoord.xy);
	int maxIndex = int(imageLoad(aBufferIndexImg, fragCoords).x);
	
	uint typeNumber = uint(imageLoad(typeIdImg, fragCoords));
	
	//Load fragments into a local memory array for sorting
	vec4 fragmentList[maxLayer];
	for(int i = 0; i < maxLayer; i++)
	{
		fragmentList[i] = vec4(0.0);
		if(i < maxIndex)
		{
			fragmentList[i] =  imageLoad(aBufferImg, ivec3(fragCoords, i));
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
			
			//switch fragment type
			uint tempTypeJ1 = (typeNumber >> (2 * (j + 1))) & 0x3;
			uint tempTypeJ2 = (typeNumber >> (2 * j)) & 0x3;
			typeNumber = typeNumber & (~(0x3 << (2 * j))) | ((tempTypeJ1) << (2 * j));
			typeNumber = typeNumber & (~(0x3 << (2 * (j + 1)))) | ((tempTypeJ1) << (2 * (j + 1)));
		  }
		}
	}
	
	float transparancy = 0.0;
	float transparancyCity = 0.3;
	float transparancyPlane = 1.0;
	float transparancyStreets = 1.0;
	float transparancyLines = 1.0;
	vec3 aBufferTexelSum = clearColor;
	float type = 0.0;
	for(int i = maxIndex-1; i >= 0; i--)
	{
		type = float((typeNumber >> (2 * i)) & 0x3);
		transparancy = mix(mix(mix(transparancyCity, transparancyPlane, step(0.5, type)), transparancyStreets, step(1.5, type)), transparancyLines, step(2.5, type));
		aBufferTexelSum = (1 - transparancy) * aBufferTexelSum + transparancy * fragmentList[i].rgb;
	}
	
	FragColor = vec4(vec3(aBufferTexelSum), 1.0);
}