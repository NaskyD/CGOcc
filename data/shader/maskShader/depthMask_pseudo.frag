#version 430

uniform int windowWidth;
uniform int windowHeight;

in vec2 v_screenAlignedQuad_UV;

layout (location = 0) out vec4 FragColor;

 
void main()
{	
	float x_value = gl_FragCoord.x / float(windowWidth);
	
	//center x_value to bottom middle
	x_value = x_value - 0.5;
	float y_value = gl_FragCoord.y / float(windowHeight);
	float distance = sqrt(x_value * x_value + y_value * y_value);
	vec4 color = mix(vec4(vec3(0.0), 1.0), vec4(vec3(1.0), 1.0), step(0.6, distance));
	vec4 gradientColor = mix(vec4(vec3(0.0), 1.0), vec4(vec3(0.5), 1.0), step(0.55, distance));
	FragColor = mix(gradientColor, color, step(0.6, distance));
}