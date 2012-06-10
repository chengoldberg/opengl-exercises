//
// Fragment shader for producing animated clouds (mostly cloudy)
//
// Author: Randi Rost
//
// Copyright (c) 2002-2005 3Dlabs Inc. Ltd. 
//
// See 3Dlabs-License.txt for license information
//

varying vec3  MCposition;

uniform sampler3D Noise;
uniform vec4 SkyColor;     // (0.0, 0.0, 0.8)
uniform vec4 CloudColor;   // (0.8, 0.8, 0.8)
uniform vec3 Offset;       // updated each frame by the application

void main()
{
    vec4  noisevec  = texture3D(Noise, MCposition + Offset);

    float intensity = (noisevec[0] + noisevec[1] + 
                       noisevec[2] + noisevec[3]) * 1.5;

    vec4 color = mix(SkyColor, CloudColor, intensity);

    gl_FragColor = color;
}