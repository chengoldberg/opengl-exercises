#version 330 core
in vec4  vFrontColor;
in vec2 vTexCoord;

uniform bool uTextureEnabled;
uniform sampler2D uTexture0;

out vec4 fragColor;

void main()
{
	if(uTextureEnabled) 
	{
		fragColor = texture2D(uTexture0, vTexCoord)*vFrontColor;		
	}
	else
	{
		fragColor = vFrontColor;	
	}
};
