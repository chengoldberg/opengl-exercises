#version 330

#define JOINTS_MAX 64

attribute vec3 aPosition;
attribute uvec4 aInfluenceJoints;
attribute vec4 aInfluenceWeights;

varying vec4  vColor;

//in vec3 aColor;
uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;
uniform vec3 uJointColors[JOINTS_MAX];

void main()
{
	//vColor = vec4(aColor,1);
	vColor = vec4(uJointColors[aInfluenceJoints[0]],1);	
	//vColor = vec4(uJointColors[aInfluenceJoints[0]],1)*0.00001 + vec4(aInfluenceJoints)/255.0;	
	//vColor = vec4(uJointColors[20],1);	
	
	//vColor = vec4(1,0,0,1);	
	//unsigned int bla = 43;
	//if(aInfluenceJoints[0]>bla)
//		vColor = vec4(0,1,0,1);	
	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition,1);
};
