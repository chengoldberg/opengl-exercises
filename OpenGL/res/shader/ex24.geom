#version 330

#define M_PI 3.1415926535897932384626433832795

layout(lines_adjacency) in;
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

out vec4 gColor;

vec4 line_point_distance_x(vec4 p0, vec4 D, vec4 p1, float r)
{
	vec4 closestPoint = p0 + D*dot(p1-p0, D);
	float t = distance(p1, closestPoint);
	float x = sqrt(r*r-t*t+0.001);
	//return closestPoint -D*x;
	return closestPoint -D*x;
	//return closestPoint;
}

vec4 lines_intersection_2d(vec4 p0, vec4 D0, vec4 p1, vec4 D1)
{
	vec4 R1 = vec4(D1[1],-D1[0],0,0);
	return p0 - (dot(p0-p1, R1)/dot(D0, R1))*D0;
}

void main()
{	
	gColor = vec4(1,1,0,1);
	vec4 forwardPrev = normalize(gl_in[1].gl_Position - gl_in[0].gl_Position);
	vec4 rightPrev = normalize(vec4(forwardPrev.y, -forwardPrev.x, 0, 0));

	vec4 forward = normalize(gl_in[2].gl_Position - gl_in[1].gl_Position);
	vec4 right = normalize(vec4(forward.y, -forward.x, 0, 0));

	vec4 forwardNext = normalize(gl_in[3].gl_Position - gl_in[2].gl_Position);
	vec4 rightNext = normalize(vec4(forwardNext.y, -forwardNext.x, 0, 0));

	float turn0 = sign(dot(forward, rightPrev));
	float turn1 = sign(dot(forwardNext, right));

	/*
	if(turn==1)	
		gColor = vec4(1,1,0,1);
	if(turn==-1)
		gColor = vec4(0,1,1,1);
	if(turn==0)
		gColor = vec4(1,0,0,1);
	*/
	float r = 0.2;

	vec4 p0 = gl_in[1].gl_Position;
	vec4 p1 = gl_in[2].gl_Position;

	// Calculate the seperating line between the two adjacent lies
	// Note the direction is towards the narrow side
	vec4 sepD0 = normalize(forward-forwardPrev);
	vec4 sepD1 = normalize(forwardNext-forward);

	// right side
	vec4 narrowSideP0 = p0+r*right;	
	vec4 narrowSideP1 = p1+r*right;	

	gColor = vec4(0,1,0,1)*float(turn0>0) + vec4(1,0,0,1)*float(turn0<=0);
	if(turn0<=0)
	{
		gl_Position = uProjectionMatrix*(p0-sepD0*r);
		EmitVertex();
		gl_Position = uProjectionMatrix*narrowSideP0;
	}
	else
	{
		gl_Position = uProjectionMatrix*lines_intersection_2d(narrowSideP0, forward, p0, sepD0);
	}
	EmitVertex();

	gColor = vec4(0,1,0,1)*float(turn1>0) + vec4(1,0,0,1)*float(turn1<=0);
	if(turn1<=0)
	{
		//gl_Position = uProjectionMatrix*line_point_distance_x(narrowSideP0, forward, p1, r);
		gl_Position = uProjectionMatrix*narrowSideP1;
		EmitVertex();
		gl_Position = uProjectionMatrix*(p1-sepD1*r);
	}
	else
	{
		gl_Position = uProjectionMatrix*lines_intersection_2d(narrowSideP0, forward, p1, sepD1);
	}
	EmitVertex();

	EndPrimitive();

	// left side
	gColor = vec4(0,1,1,1);

	vec4 wideSideP0 = p0-r*right;	
	vec4 wideSideP1 = p1-r*right;	
	
	gColor = vec4(0,1,0,1)*float(turn0<=0) + vec4(1,0,0,1)*float(turn0>0);
	gl_Position = uProjectionMatrix*lines_intersection_2d(wideSideP0, forward, p0, sepD0);
	EmitVertex();

	gColor = vec4(0,1,0,1)*float(turn1<=0) + vec4(1,0,0,1)*float(turn1>0);
	gl_Position = uProjectionMatrix*lines_intersection_2d(wideSideP0, forward, p1, sepD1);
	EmitVertex();

	EndPrimitive();
}