#define IMG_WIDTH 620
#define IMG_HEIGHT 349

attribute vec2 aPosition;
varying vec2 texCoord;

void main(void)
{
      gl_Position = vec4(aPosition,0,1);   
	  texCoord = aPosition*0.5+0.5;
	  texCoord.y = 1.0-texCoord.y;
	  texCoord *= vec2(IMG_WIDTH, IMG_HEIGHT);
	  
}
