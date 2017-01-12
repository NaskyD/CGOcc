#version 430

in vec2 v_screenAlignedQuad_UV;

coherent uniform layout(size4x32) image2DArray aBufferImg;
coherent uniform layout(size1x32) uimage2D aBufferIndexImg;

//has to be const
const int maxLayer = 16;

void sortABufferFragments(ivec2 coords, uint maxIndex){
	//Load fragments into a local memory array for sorting
	vec4 fragmentList[maxLayer];
	for(int i = 0; i < maxIndex; i++)
	{
		fragmentList[i] = imageLoad(aBufferImg, ivec3(coords, i));
	}

	//Bubble sort
	for (int i = (int(maxIndex) - 2); i >= 0; --i) {
		for (int j = 0; j <= i; ++j) {
		  if (fragmentList[j].w > fragmentList[j+1].w) {
			vec4 temp = fragmentList[j+1];
			fragmentList[j+1] = fragmentList[j];
			fragmentList[j] = temp;
		  }
		}
	}
	
	//Store fragments in right order
	for(int i = 0; i < maxIndex; i++)
	{
		imageStore(aBufferImg, ivec3(coords, i), fragmentList[i]);
	}
}


void main()
{
	ivec2 coords = ivec2(gl_FragCoord.xy);
	uint maxIndex = imageLoad(aBufferIndexImg, coords).x;

	sortABufferFragments(coords, maxIndex);

	discard;
}