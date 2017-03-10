#version 430

uniform int windowWidth;
uniform int windowHeight;

uniform sampler2D texture0;

in vec2 v_screenAlignedQuad_UV;

layout (location = 0) out vec4 FragColor;

const float pixelSizeX = 1.0/float(windowWidth);
const float pixelSizeY = 1.0/float(windowHeight);

const mat3 horizontalEdge = mat3(vec3(-1, -2, -1),
								 vec3( 0,  0,  0),
								 vec3( 1,  2,  1));

const mat3 verticalEdge = mat3(vec3(-1, 0, 1),
							   vec3(-2, 0, 2),
							   vec3(-1, 0, 1));

const mat3 firstDiagonal = mat3(vec3( 0,  1,  2),
								vec3(-1,  0,  1),
								vec3(-2, -1,  0));
						  
const mat3 secondDiagonal = mat3(vec3(-2, -1, 0), 
								 vec3(-1,  0, 1), 
								 vec3( 0,  1, 2));
						   
const int rKernel = 3;
 
void main()
{	
	vec3 sumHorizontal = vec3(0.0);
	vec3 sumVertical = vec3(0.0);
	vec3 sumFirstD = vec3(0.0);
	vec3 sumSecondD = vec3(0.0);
	vec3 sum = vec3(0.0);
	
	for (int dx = 0; dx < rKernel; ++dx)
	{
		for (int dy = 0; dy < rKernel; ++dy)
		{
			vec2 textureKernel = vec2(float(dx) * pixelSizeX, float(dy) * pixelSizeY);
			vec3 boxTexel = texture(texture0, v_screenAlignedQuad_UV + textureKernel).xyz;
			
			sumHorizontal = sumHorizontal + horizontalEdge[dx][dy] * boxTexel;
			sumVertical = sumVertical + verticalEdge[dx][dy] * boxTexel;
			sumFirstD = sumFirstD + firstDiagonal[dx][dy] * boxTexel;
			sumSecondD = sumSecondD + secondDiagonal[dx][dy] * boxTexel;
		}
	}
	
	sum = abs(sumHorizontal) + abs(sumVertical) + abs(sumFirstD) + abs(sumSecondD);
	//sum = step(1.3, sum);
	float grey_sum = (sum.x + sum.y + sum.z) / 3.0;
	//sum = step(0.1, sum);
	grey_sum = step(0.05, grey_sum);
	FragColor = vec4(vec3(grey_sum), 1.0);
}