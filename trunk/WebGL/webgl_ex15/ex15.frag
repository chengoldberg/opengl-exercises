precision mediump float;

uniform mediump vec3 uColor;
uniform bool uTextureEnabled;

varying vec3 vColor;

void main(void)
{
	gl_FragColor = vec4(vColor, 1.0);		
}
