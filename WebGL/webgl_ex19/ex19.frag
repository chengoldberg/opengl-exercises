precision mediump float;

uniform mediump vec3 uColor;
uniform mediump bool uTextureEnabled;

uniform sampler2D uTexImg1;
uniform sampler2D uTexImg1to2;

varying vec2 vTexCoord;

void main(void)
{
	vec4 color = texture2D(uTexImg1, vTexCoord);		
	//gl_FragColor = vec4(color, 1.0);		
	gl_FragColor = color;
}
