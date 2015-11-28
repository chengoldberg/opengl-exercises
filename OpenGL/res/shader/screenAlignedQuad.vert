attribute vec2 aPosition;
varying vec2 vTexCoords;

void main(void)
{
      gl_Position = vec4(aPosition,0,1);   
	  vTexCoords = aPosition*2.0+1.0;
}
