//
// Vertex shader for producing animated clouds (mostly cloudy)
//
// Authors: John Kessenich, Randi Rost
//
// Copyright (c) 2002-2005 3Dlabs Inc. Ltd. 
//
// See 3Dlabs-License.txt for license information
//

varying vec3  MCposition;
uniform float Scale;

void main()
{
    MCposition      = vec3 (gl_Vertex) * Scale;    
    gl_Position     = ftransform();
}