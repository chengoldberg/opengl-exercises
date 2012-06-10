//
// fragment shader for general convolution
//
// Author: Randi Rost
//
// Copyright (c) 2003-2005: 3Dlabs, Inc.
//
// See 3Dlabs-License.txt for license information
//

// maximum size supported by this shader
const int MaxKernelSize = 25; 

// array of offsets for accessing the base image
uniform vec2 Offset[MaxKernelSize];

// size of kernel (width * height) for this execution
uniform int KernelSize;

// value for each location in the convolution kernel
uniform float KernelValue[MaxKernelSize];

// image to be convolved
uniform sampler2D BaseImage;

// image to be convolved
uniform sampler2D DepthImage;

void main()
{	
    int i;
    vec4 sum = vec4(0.0);
	float averageDepth = 1.0;
	float alphaSum = 0.0;
	
    for (i = 0; i < KernelSize; i++)
    {
        vec4 tmp = texture2D(BaseImage, gl_TexCoord[0].st + Offset[i]);
		sum += (tmp * KernelValue[i]);
		alphaSum += tmp.a;

		averageDepth = min(averageDepth, texture2D(DepthImage, gl_TexCoord[0].st+Offset[i]).r);
    }	
	
	if(alphaSum==0.0)
		discard;
	
	sum = vec4(
		sign(sum.x)*sqrt(abs(sum.x)),
		sign(sum.y)*sqrt(abs(sum.y)),
		sign(sum.z)*sqrt(abs(sum.z)),
		1);
		
	sum = (sum+1.0)/2.0;
		
	gl_FragDepth = averageDepth;
	gl_FragColor = sum;
}