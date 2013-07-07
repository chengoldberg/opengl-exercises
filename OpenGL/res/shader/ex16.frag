
uniform sampler2D uTex;

void main()
{
	vec4 sample = texture2D(uTex, gl_TexCoord[0].xy);
	if(sample.w<0)
		discard;
	gl_FragColor = vec4(sample.xyz,1);

	float ndcDepth = sample.w;	
	// Transform from NDC to viewport using depthrange
	gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
};
