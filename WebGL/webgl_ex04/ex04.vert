precision mediump float;

attribute vec3 aPosition;
attribute vec3 aNormal;

uniform mat3 uNormalMatrix;
uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mediump bool uLightingEnabled;

uniform vec4 uLightPosition;
uniform vec4 uAmbient;  
uniform vec4 uDiffuse;
uniform vec4 uSpecular;
uniform float uSpecularPower;

varying vec3 vFrontColor;
varying vec3 vBackColor;

vec3 calc(vec3 normal) {
	//TODO:
	vec3 uEyePosition = vec3(0,0,0);

	vec4 fvObjectPosition = uModelViewMatrix * vec4(aPosition,1);

	vec3 ViewDirection  = uEyePosition - fvObjectPosition.xyz;
	vec3 LightDirection = uLightPosition.xyz - fvObjectPosition.xyz;
	vec3 Normal         = uNormalMatrix * normal;   

	vec3  fvLightDirection = normalize( LightDirection );
	vec3  fvNormal         = normalize( Normal );
	float fNDotL           = dot( fvNormal, fvLightDirection ); 

	vec3  fvReflection     = normalize( ( ( 2.0 * fvNormal ) * fNDotL ) - fvLightDirection ); 
	vec3  fvViewDirection  = normalize( ViewDirection );
	float fRDotV           = max( 0.0, dot( fvReflection, fvViewDirection ) );

	vec4  fvTotalAmbient   = uAmbient; 
	vec4  fvTotalDiffuse   = uDiffuse * fNDotL; 
	vec4  fvTotalSpecular  = uSpecular * ( pow( fRDotV, uSpecularPower ) );

	return ( fvTotalAmbient + fvTotalDiffuse + fvTotalSpecular ).xyz;       
}

void main(void)
{
	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition,1);

	vFrontColor = calc(aNormal);
	vBackColor = calc(-aNormal);
}
