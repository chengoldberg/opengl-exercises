#version 330 core

struct LightSourceParameters 
{   
   vec4 ambient;              // Aclarri   
   vec4 diffuse;              // Dcli   
   vec4 specular;             // Scli   
   vec4 position;             // Ppli   
   vec4 halfVector;           // Derived: Hi   
   vec3 spotDirection;        // Sdli   
   float spotExponent;        // Srli   
   float spotCutoff;          // Crli                              
                              // (range: [0.0,90.0], 180.0)   
   float spotCosCutoff;       // Derived: cos(Crli)                 
                              // (range: [1.0,0.0],-1.0)   
   float constantAttenuation; // K0   
   float linearAttenuation;   // K1   
   float quadraticAttenuation;// K2  
   bool isEnabled;   
};    

struct MaterialParameters  
{   
   vec4 emission;    // Ecm   
   vec4 ambient;     // Acm   
   vec4 diffuse;     // Dcm   
   vec4 specular;    // Scm   
   float shininess;  // Srm  
};  

// Inputs
in vec3 aPosition;
in vec3 aColor;
in vec3 aNormal;
in vec2 aTexCoord;
uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat3 uNormalMatrix;
uniform MaterialParameters uMaterial;  
uniform LightSourceParameters uLightSource[8];
uniform bool uLightingEnabled;

// Outputs
out vec4 vFrontColor;
out vec2 vTexCoord;
out gl_PerVertex
{
	vec4 gl_Position;
};

vec3 calcLighting(vec3 normal, vec4 viewPosition) {

	vec3 uEyePosition = vec3(0,0,0);

	vec3 ViewDirection  = uEyePosition - viewPosition.xyz;	
	vec3 Normal         = uNormalMatrix * normal;   

	vec4 result = vec4(0) + uMaterial.emission;	
	for(int i=0;i<4;++i) {
		if(!uLightSource[i].isEnabled)
			continue;

		vec3 lightPosition = (uLightSource[i].position.xyz)/(uLightSource[i].position.w + 0.00001);
		vec3 LightDirection = lightPosition - viewPosition.xyz;
		float distance = length(LightDirection);
		vec3  fvLightDirection = normalize( LightDirection );
		vec3  fvNormal         = normalize( Normal );
		float fNDotL           = max( 0.0, dot( fvNormal, fvLightDirection )); 

		float spotAngle = dot(-fvLightDirection, uLightSource[i].spotDirection);
		if(acos(abs(spotAngle)) > radians(uLightSource[i].spotCutoff)) 
			continue;

		vec3  fvReflection     = normalize( ( ( 2.0 * fvNormal ) * fNDotL ) - fvLightDirection ); 
		vec3  fvViewDirection  = normalize( ViewDirection );
		float fRDotV           = max( 0.0, dot( fvReflection, fvViewDirection ) );

		vec4  fvTotalAmbient   = uMaterial.ambient * uLightSource[i].ambient; 
		vec4  fvTotalDiffuse   = uMaterial.diffuse * fNDotL * uLightSource[i].diffuse; 
		vec4  fvTotalSpecular  = uMaterial.specular * ( pow( fRDotV, uMaterial.shininess ) ) * uLightSource[i].specular;

		float attenuation = 1.0/(
			uLightSource[i].constantAttenuation + 
			uLightSource[i].linearAttenuation*distance + 
			uLightSource[i].quadraticAttenuation*distance*distance);

		result += fvTotalAmbient + (fvTotalDiffuse + fvTotalSpecular)*attenuation*pow(spotAngle, uLightSource[i].spotExponent);		
	}	
	return result.xyz;       
}

void main()
{
	vTexCoord = aTexCoord;
	vec4 viewPosition = uModelViewMatrix * vec4(aPosition,1);
	
	if(uLightingEnabled) 
	{
		vFrontColor = vec4(calcLighting(aNormal, viewPosition),1);
		//vBackColor = calcLighting(-aNormal, viewPosition);
	}
	else
	{
		vFrontColor = vec4(aColor,1);
	}
	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition,1);
};
