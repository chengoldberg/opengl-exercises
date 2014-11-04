#version 400 core
layout(vertices = 4) out;
uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;

void main()
{
	vec4 vec = uProjectionMatrix*uModelViewMatrix*(gl_in[0].gl_Position - gl_in[3].gl_Position);
	float N = length(vec.xyz);

	gl_TessLevelOuter[0] = 1;
	gl_TessLevelOuter[1] = 40*N;
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
