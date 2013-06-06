precision mediump float;

uniform mediump vec3 uColor;
uniform mediump bool uTextureEnabled;
uniform sampler2D uTexture0;

varying vec2 vTexCoord;

void main(void)
{
	if(uTextureEnabled) {
   		gl_FragColor = texture2D(uTexture0, vTexCoord);
	} else {
		gl_FragColor = vec4( uColor, 1.0 );		
	}
}
