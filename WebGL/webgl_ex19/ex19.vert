precision mediump float;

attribute vec3 aPosition;
attribute vec2 aTexCoord;

uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;
uniform int uFrameNum;

varying vec2 vTexCoord;

void main(void)
{	
	vTexCoord = aTexCoord;
	vec3 pos = aPosition;
	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(pos,1);	
}
