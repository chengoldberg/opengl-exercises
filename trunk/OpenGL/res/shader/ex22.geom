#version 330
layout(triangles) in;
layout(points, max_vertices=1) out;
in vec3 position[3];
out vec3 normal;

void main()
{
	vec3 a = position[1]-position[0];
	vec3 b = position[2]-position[0];
	//normal = normalize(cross(a,b));
	
	vec3 N = cross(a,b);
	float norm = length(N);
	if(norm==0)
	{
		normal = vec3(0);		
	}
	else
	{
		normal = N/norm;
	}
	
	EmitVertex();
	EndPrimitive();
}