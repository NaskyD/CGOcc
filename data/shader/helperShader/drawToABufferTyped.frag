#version 430

in vec3 v_normals;

uniform vec4 specifiedColor;
layout (location = 10) uniform samplerCube cubeMap;
uniform bool useNormals;
uniform bool renderDepthValueForTextureUsage;
uniform uint typeId;												//has to be smaller than 3
coherent uniform layout(size4x32) image2DArray aBufferImg;
coherent uniform layout(size1x32) image2DArray aBufferAlphaImg;
coherent uniform layout(size1x32) uimage2D aBufferIndexImg;
coherent uniform layout(size1X32) uimage2D typeIdImg;

layout (location = 0) out uint Type;
 
void main()
{
	//Linearization of the depth value
	float f = 200.0;
	float n = 0.1f;
	float depthValue = (2 * n) / (f + n - gl_FragCoord.z * (f - n));
	
	vec3 color = specifiedColor.rgb;
	
	if(useNormals)
	{
		color = texture(cubeMap, v_normals).xyz * 0.8;
	}
	
	//Use ABuffer as additional render target
	ivec2 fragCoords = ivec2(gl_FragCoord.xy);
	uint index = imageAtomicAdd(aBufferIndexImg, ivec2(fragCoords), 1);		//the returned index is the old value before addition
	if(renderDepthValueForTextureUsage)
	{
		imageStore(aBufferImg, ivec3(fragCoords, index), vec4(color, depthValue));
		imageStore(aBufferAlphaImg, ivec3(fragCoords, index), vec4(1.0));
	}
	else
	{
		depthValue = gl_FragCoord.z;
		imageStore(aBufferImg, ivec3(fragCoords, index), vec4(color, depthValue));
		imageStore(aBufferAlphaImg, ivec3(fragCoords, index), vec4(1.0));
	}
	
	//store type information (4 different types storable for 16 layers)
	uint currentTypeNumber = uint(imageLoad(typeIdImg, fragCoords));
	uint newTypeNumber = currentTypeNumber & (~(0x3 << (2 * index))) | ((typeId & 0x3) << (2 * index));
	imageStore(typeIdImg, fragCoords, uvec4(newTypeNumber));
	
	discard;
}