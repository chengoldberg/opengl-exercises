precision mediump float;

attribute vec3 aPosition;
attribute vec3 aNormal;

uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mediump bool uLightingEnabled;

varying vec3 vFrontColor;
varying vec3 vBackColor;

void main(void)
{
	vBackColor = vec3(1,0,0);
	vFrontColor = vec3(0,1,0);
	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition,1);
}
