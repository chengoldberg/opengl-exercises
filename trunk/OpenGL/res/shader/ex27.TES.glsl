#version 400 core
layout(isolines, equal_spacing, ccw/*, point_mode*/ ) in;
uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;

vec4 sampleCurve(float u)
{
	return 
		(1-u)*(1-u)*(1-u) * gl_in[0].gl_Position + 
		3*(1-u)*(1-u)*u * gl_in[1].gl_Position + 
		3*(1-u)*u*u * gl_in[2].gl_Position + 
		u*u*u * gl_in[3].gl_Position;
}

void main()
{
    //vec4 position = mix(gl_in[0].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x);
	vec4 position = sampleCurve(gl_TessCoord.x);
	gl_Position = uProjectionMatrix * uModelViewMatrix * position;
	gl_PointSize = 5;
}
