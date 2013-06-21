#version 330 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec4  vColor[3];
out vec4 gColor;

void main()
{
	int n;
	for(n=0; n<gl_in.length(); ++n) 
	{
		gl_Position = gl_in[n].gl_Position;
		gColor = vColor[n];
		EmitVertex();
	}	
	EndPrimitive();
};
