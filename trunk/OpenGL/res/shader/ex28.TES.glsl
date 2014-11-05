#version 400 core
layout(quads, equal_spacing, ccw/*, point_mode*/ ) in;
uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;
out vec4 iColor;
uniform sampler1D uTexture;

vec4 sampleCurve(float u, in vec4 p0, in vec4 p1, in vec4 p2, in vec4 p3)
{
	return 
		(1-u)*(1-u)*(1-u) * p0 + 
		3*(1-u)*(1-u)*u * p1 + 
		3*(1-u)*u*u * p2 + 
		u*u*u * p3;
}

vec4 interpolateQuad(in vec4 p0, in vec4 p1, in vec4 p2, in vec4 p3)
{
	vec4 x0 = mix(p0, p1, gl_TessCoord.x);
	vec4 x1 = mix(p2, p3, gl_TessCoord.x);
	return mix(x0, x1, gl_TessCoord.y);
}

void main()
{
	vec4 samplesx[4];
	for(int i=0; i<4; ++i)
	{
		samplesx[i] = sampleCurve(
			gl_TessCoord.x, 
			gl_in[4*i+0].gl_Position, 
			gl_in[4*i+1].gl_Position, 
			gl_in[4*i+2].gl_Position, 
			gl_in[4*i+3].gl_Position);
	}
	vec4 position = sampleCurve(
		gl_TessCoord.y,
		samplesx[0],
		samplesx[1],
		samplesx[2],
		samplesx[3]);

	/*
    vec4 position = interpolateQuad(
		gl_in[0].gl_Position, 
		gl_in[3].gl_Position, 
		gl_in[12].gl_Position, 
		gl_in[15].gl_Position);
	*/
	//vec4 position = sampleCurve(gl_TessCoord.x);
	//gl_Position = uProjectionMatrix * uModelViewMatrix * position;
	gl_Position = position;
	//gl_Position = vec4(0,0,-2,1);
	gl_PointSize = 5;
	//iColor = vec4(,1,1,1);
	iColor = vec4(texelFetch(uTexture, gl_PrimitiveID, 0).xyz,1);
}
