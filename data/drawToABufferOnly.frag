#version 430

in vec3 v_normals;

uniform vec3 lightVector;
uniform vec3 lightVector2;
uniform vec4 specifiedColor;
uniform bool useNormals;
uniform bool renderDepthValueForTextureUsage;
coherent uniform layout(size4x32) image2DArray aBufferImg;
coherent uniform layout(size1x32) uimage2D aBufferIndexImg;

layout (location = 0) out vec4 FragColor;
 
void main()
{
	//Linearization of the depth value
	float f = 200.0;
	float n = 0.1f;
	float depthValue = (2 * n) / (f + n - gl_FragCoord.z * (f - n));
	
	vec3 color = specifiedColor.rgb;
	
	if(useNormals)
	{
		float NdotL  = max(dot(normalize(v_normals), normalize(lightVector)), 0.0);
		float NdotL2 = max(dot(normalize(v_normals), normalize(lightVector2)), 0.0);
		vec3 diffuseColor = vec3(0.8, 0.8, 0.8);
		vec3 diffuseColor2 = vec3(0.3, 0.3, 0.3);
		vec3 ambient = vec3(0.2, 0.2, 0.2);
		color = color * vec3(NdotL2 * diffuseColor + NdotL * diffuseColor2 + ambient);
	}
	
	//Use ABuffer as additional render target
	ivec2 fragCoords = ivec2(gl_FragCoord.xy);
	uint index = imageAtomicAdd(aBufferIndexImg, ivec2(fragCoords), 1);		//the returned index is the old value before addition
	if(renderDepthValueForTextureUsage)
	{
		depthValue = depthValue;
		imageStore(aBufferImg, ivec3(fragCoords, index), vec4(color, depthValue));
	}
	else
	{
		depthValue = gl_FragCoord.z;
		imageStore(aBufferImg, ivec3(fragCoords, index), vec4(color, depthValue));
	}
	
	discard;	//only rendering to A-Buffer
}