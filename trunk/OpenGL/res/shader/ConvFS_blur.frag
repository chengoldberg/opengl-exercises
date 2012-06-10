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
	vec4 totalWeight = vec4(0);
    for (i = 0; i < KernelSize; i++)
    {
        vec4 tmp = texture2D(BaseImage, gl_TexCoord[0].st + Offset[i]);
		if(tmp.a == 0.0) {
			continue;
		} else {
			sum += (tmp * KernelValue[i]);
			totalWeight += KernelValue[i];
		}
         
		//averageDepth += texture2D(DepthImage, gl_TexCoord[0]+Offset[i]).z*KernelValue[i].z;
		averageDepth = min(averageDepth, texture2D(DepthImage, gl_TexCoord[0].st+Offset[i]).r);
    }
	sum.xyz = sum.xyz/totalWeight.xyz;
	
	//gl_FragDepth = texture2D(DepthImage, gl_TexCoord[0]).z;
	gl_FragDepth = averageDepth;
	gl_FragColor = sum;
    //gl_FragColor = vec4(texture2D(DepthImage, gl_TexCoord[0]).xyz,1);
}