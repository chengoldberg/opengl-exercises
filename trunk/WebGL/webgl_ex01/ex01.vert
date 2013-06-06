		attribute vec3 aVertex;
		uniform mat4 uMVMatrix;
		uniform mat4 uPMatrix; 

		//varying mediump vec3 vColor;

		void main(void)
		{
			//vColor = uColor;
			gl_Position = uPMatrix * uMVMatrix * vec4(aVertex,1.0);
		}
