precision mediump float;

attribute vec3 aPosition;
//attribute vec2 aTexCoord;

uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;
uniform int uFrameNum;

uniform sampler2D uTexImg1;
uniform sampler2D uTexImg1to2;

varying vec3 vColor;

void main(void)
{	
	vColor = texture2D(uTexImg1, aPosition.xy/256.0).xyz;	
	
	vec3 pos1 = aPosition;
	pos1.z -= (1.0-vColor.x)*256.0;

	vec3 pos2 = texture2D(uTexImg1to2, aPosition.xy/256.0).xyz * 256.0;
	pos2.y = 256.0 - pos2.y;
	pos2.z -= (1.0-vColor.x)*256.0;

	float alpha = (cos(float(uFrameNum)*0.02)+1.0) /2.0;
	vec3 pos = mix(pos1, pos2, alpha);

	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(pos,1);
	gl_PointSize = 1.0; // NOTE: Must specify!
}
