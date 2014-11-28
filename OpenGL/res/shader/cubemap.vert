#version 120
varying vec3 reflection;

void main(void)
{
      gl_Position = ftransform();   
   
      //vec4 esLightpos = gl_ModelViewMatrix*lightpos;
      vec4 esPos = gl_ModelViewMatrix*gl_Vertex;   
      vec3 esNormal = normalize(gl_NormalMatrix*gl_Normal);
      
      vec3 eyeToSurf =  normalize(esPos.xyz);
	  
      //vec3 surfToLight = normalize(esPos.xyz-esLightpos.xyz);
      //float diffuse = clamp(dot(esNormal,surfToLight),0.0,1.0);     
                   
      //gl_FrontColor = vec4(diffuse*lightdiff*matdiff,1);
      
     vec3 esReflection = reflect(eyeToSurf, esNormal);
	 //reflection = esReflection;
     reflection = transpose(gl_NormalMatrix )* esReflection; 
}
