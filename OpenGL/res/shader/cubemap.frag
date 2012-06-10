#version 130
uniform samplerCube CubeMap;
//uniform float trans;

varying vec3 reflection;

void main(void)
{
	//float trans = 0.5;
   vec4 env = texture(CubeMap, reflection);
   gl_FragColor = env;
   //gl_FragColor = mix(gl_Color,env,trans);
   //gl_FragColor = mix(gl_Color,vec4(1,1,1,1),trans)*env;
}