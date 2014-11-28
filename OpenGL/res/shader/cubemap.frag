#version 120
uniform samplerCube CubeMap;

varying vec3 reflection;

void main(void)
{
	vec3 temp = reflection;
	temp.y *= -1;
	vec4 env = textureCube(CubeMap, temp);
	gl_FragColor = env;
}