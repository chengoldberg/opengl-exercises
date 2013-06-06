varying vec4 vColor;
attribute mediump vec3 aPosition;
attribute mediump vec3 aColor;
uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;

void main()
{
  vColor = vec4(aColor,1);
  gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition,1);
  gl_PointSize = 3.0;
}
