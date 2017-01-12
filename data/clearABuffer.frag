#version 430

in vec2 v_screenAlignedQuad_UV;

coherent uniform layout(size4x32) image2DArray aBufferImg;
coherent uniform layout(size1x32) uimage2D aBufferIndexImg;
coherent uniform layout(size1x32) uimage2D typeIdImg;
uniform int windowWidth;
uniform int windowHeight;
uniform int maxLayer;

void main(void) {
	ivec2 coords = ivec2(gl_FragCoord.xy);
	
	if(coords.x >= 0 && coords.y >= 0 && coords.x < windowWidth && coords.y < windowHeight ){
		imageStore(aBufferIndexImg, coords, ivec4(0));
		for(int i = 0; i < maxLayer; i++)
		{
			imageStore(aBufferImg, ivec3(coords, i), vec4(0.0f));
		}
		imageStore(typeIdImg, coords, uvec4(0));
	}

	//Discard fragment so nothing is writen to the framebuffer
	discard;
}


