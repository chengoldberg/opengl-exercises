#version 330

#define M_PI 3.1415926535897932384626433832795

layout(lines) in;
layout(line_strip, max_vertices=32) out;

uniform mat4 uProjectionMatrix;

// No need to declare these
in gl_PerVertex
{
  vec4 gl_Position;
} gl_in[];

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{	
	vec4 forward = normalize(gl_in[1].gl_Position - gl_in[0].gl_Position);
	vec4 right = normalize(vec4(-forward.y, forward.x, 0, 0));
	float r = 0.2;
	for(int i=0; i<2; ++i)
	{
		gl_Position = uProjectionMatrix*(gl_in[i].gl_Position+r*right);	
		EmitVertex();
	}
	EndPrimitive();

	for(int i=0; i<2; ++i)
	{
		gl_Position = uProjectionMatrix*(gl_in[i].gl_Position-r*right);	
		EmitVertex();
	}
	EndPrimitive();
}