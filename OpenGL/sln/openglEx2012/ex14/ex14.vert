#version 330 core

out vec4  vColor;
in vec3 aPosition;
in vec3 aColor;
uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;

void main()
{
	vColor = vec4(aColor,1);
	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition,1);
};