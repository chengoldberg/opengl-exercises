precision mediump float;

attribute vec3 aPosition;
attribute vec3 aNormal;

uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;

varying vec3 vFrontColor;
varying vec3 vBackColor;

vec3 calcLighting(vec3 normal, vec4 viewPosition);

void main(void)
{
	vec4 viewPosition = uModelViewMatrix * vec4(aPosition,1);
	gl_Position = uProjectionMatrix * viewPosition;

	vFrontColor = calcLighting(aNormal, viewPosition);
	vBackColor = calcLighting(-aNormal, viewPosition);
}
