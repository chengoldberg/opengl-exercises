varying vec2 vTexCoords;

void main(void)
{  
     gl_FragColor = vec4(vTexCoords, 0, 1);
}
