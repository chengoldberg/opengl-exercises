#version 150

#define M_PI 3.1415926535897932384626433832795

layout(points) in;
layout(triangle_strip, max_vertices=120) out;

in vec4	vColor[];
out vec4 gColor;

uniform mat4 uProjectionMatrix;

// Valid from version 330
/*
in gl_PerVertex
{
  vec4 gl_Position;
} gl_in[];

out gl_PerVertex
{
	vec4 gl_Position;
};
*/

void main()
{
	gColor = vColor[0];
	vec4 colorOpaque = vColor[0];
	vec4 colorZeroAlpha = vec4(colorOpaque.rgb,0);
	vec4 center = gl_in[0].gl_Position;	

	float r = 0.05;
	float alpha = clamp(1-(-center.z/2.0), 0, 1);
	int d = int(40*alpha+(1-alpha)*4);
	
	float radStep = 2*M_PI/d;

	gl_Position = uProjectionMatrix*(center + r*vec4(cos(0), sin(0),0,0));
	gColor = colorZeroAlpha;
	EmitVertex();

	for(int i=0; i<d; ++i)
	{
		gl_Position = uProjectionMatrix*center;	
		gColor = colorOpaque;
		EmitVertex();
		gl_Position = uProjectionMatrix*(center + r*vec4(cos(radStep*(i+1)), sin(radStep*(i+1)), 0,0));
		gColor = colorZeroAlpha;
		EmitVertex();		
	}
	gl_Position = uProjectionMatrix*center;	
	gColor = colorOpaque;
	EmitVertex();

	EndPrimitive();
}