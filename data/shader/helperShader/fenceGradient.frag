#version 430

uniform vec4 specifiedColor;
uniform bool renderDepthValueForTextureUsage;
uniform float geometryHeight;

uniform uint typeId;												//has to be smaller than 3
coherent uniform layout(size4x32) image2DArray aBufferImg;
coherent uniform layout(size1x32) image2DArray aBufferAlphaImg;
coherent uniform layout(size1x32) uimage2D aBufferIndexImg;
coherent uniform layout(size1x32) uimage2D typeIdImg;

in float g_currentHeight;

layout (location = 0) out vec4 FragColor;

const float minHeight = -0.7704;
const float maxHeight = geometryHeight;
 
void main()
{
	vec4 color = specifiedColor;
	
	float fragmentRelativeHeight = (g_currentHeight - minHeight) / (maxHeight - minHeight);
	float factor = 1 - fragmentRelativeHeight;
	color.a = pow(smoothstep(0.0, 1.0, factor), 2);
	
	//Use ABuffer as additional render target
	ivec2 fragCoords = ivec2(gl_FragCoord.xy);
	uint index = imageAtomicAdd(aBufferIndexImg, ivec2(fragCoords), 1);		//the returned index is the old value before addition

	imageStore(aBufferImg, ivec3(fragCoords, index), vec4(color.rgb, gl_FragCoord.z));
	imageStore(aBufferAlphaImg, ivec3(fragCoords, index), vec4(color.a));

	//store type information (4 different types storable for 16 layers)
	uint currentTypeNumber = uint(imageLoad(typeIdImg, fragCoords));
	uint newTypeNumber = currentTypeNumber & (~(0x3 << (2 * index))) | ((typeId & 0x3) << (2 * index));
	imageStore(typeIdImg, fragCoords, uvec4(newTypeNumber));
	
	discard;
}