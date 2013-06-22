#version 330 core

#define TRIANGLE 2
#define LINE 1
#define POINT 0

//#define PRIMITIVE POINT

layout(lines_adjacency) in;

#if PRIMITIVE == TRIANGLE
layout(triangle_strip, max_vertices = 4) out;
int map[] = int[](0,1,3,2);

#elif PRIMITIVE == LINE
layout(line_strip, max_vertices = 4) out;
int map[] = int[](0,1,2,3);

#elif PRIMITIVE == POINT
layout(points, max_vertices = 4) out;
int map[] = int[](0,1,2,3);

#endif

in vec4  vColor[];
out vec4 gColor;

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
	int n;
	
	for(n=0; n<gl_in.length(); ++n) 
	{
		gl_Position = gl_in[map[n]].gl_Position;
		gColor = vColor[map[n]];
		EmitVertex();
	}	
	EndPrimitive();
};
