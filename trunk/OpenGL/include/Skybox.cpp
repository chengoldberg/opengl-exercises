/*
 * Copyright (C) 2010  Chen Goldberg
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include "Skybox.h"
#include <GL/glew.h>
#include "CTargaImage.h"
#include <stdio.h>

char filenames[6][7] = {
	"lt.tga",
	"rt.tga",
	
	"dn.tga",
	"up.tga",
		
	"bk.tga",		
	"ft.tga"
};

CSkybox::CSkybox(){
}


CSkybox::~CSkybox(){
	Release();
}


void CSkybox::init(float size, char* dir){
	m_size = size;
	m_directory = dir;	

	glGenTextures(1, &m_tex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_tex);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	char temp[256];
	for(int i=0;i<6;++i) {
		sprintf(temp,"%s%s", dir, filenames[i]);
		loadTexture(temp,i);
	}
}

void CSkybox::Render(float cameraX, float cameraY, float cameraZ)
{
	glPushMatrix();
	
	// 
	// Move the skybox so that it's centered on the camera.
    // Implement a small factor for creating a minor sense of movment.
	//
	//float factor = 95.0f/96.0f;
	float factor = 90.0f/96.0f;
	glTranslatef(cameraX*factor, cameraY*factor, cameraZ*factor);
	glScaled(1,-1,1);
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glDisable(GL_LIGHTING);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);	

	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_tex);
	
	glBegin(GL_QUADS);
	{
		// Top	
		glTexCoord3f(-1,1,-1); glVertex3f(-m_size, m_size, -m_size);
		glTexCoord3f(1,1,-1); glVertex3f(m_size, m_size, -m_size);
		glTexCoord3f(1,1,1); glVertex3f(m_size, m_size, m_size);
		glTexCoord3f(-1,1,1); glVertex3f(-m_size, m_size, m_size);

		// Bottom
		glTexCoord3f(1,-1,-1); glVertex3f(m_size, -m_size, -m_size);
		glTexCoord3f(-1,-1,-1); glVertex3f(-m_size, -m_size, -m_size);
		glTexCoord3f(-1,-1,1); glVertex3f(-m_size, -m_size, m_size);
		glTexCoord3f(1,-1,1); glVertex3f(m_size, -m_size, m_size);

		// Front
		glTexCoord3f(-1,-1,-1); glVertex3f(-m_size, -m_size, -m_size);
		glTexCoord3f(1,-1,-1); glVertex3f(m_size, -m_size, -m_size);
		glTexCoord3f(1,1,-1); glVertex3f(m_size, m_size, -m_size);
		glTexCoord3f(-1,1,-1); glVertex3f(-m_size, m_size, -m_size);
		
		// Back
		glTexCoord3f(1,-1,1); glVertex3f(m_size, -m_size, m_size);
		glTexCoord3f(-1,-1,1); glVertex3f(-m_size, -m_size, m_size);
		glTexCoord3f(-1,1,1); glVertex3f(-m_size, m_size, m_size);
		glTexCoord3f(1,1,1); glVertex3f(m_size, m_size, m_size);

		// Right
		glTexCoord3f(1,-1,1); glVertex3f(m_size, -m_size, m_size);
		glTexCoord3f(1,1,1); glVertex3f(m_size, m_size, m_size);
		glTexCoord3f(1,1,-1); glVertex3f(m_size, m_size, -m_size);
		glTexCoord3f(1,-1,-1); glVertex3f(m_size, -m_size, -m_size);

		// Left
		glTexCoord3f(-1,-1,-1); glVertex3f(-m_size, -m_size, -m_size);
		glTexCoord3f(-1,1,-1); glVertex3f(-m_size, m_size, -m_size); 
		glTexCoord3f(-1,1,1); glVertex3f(-m_size, m_size, m_size);
		glTexCoord3f(-1,-1,1); glVertex3f(-m_size, -m_size, m_size);
	}
	glEnd();

	glDisable(GL_TEXTURE_CUBE_MAP);

	glPopAttrib();	
	glPopMatrix();
}


void CSkybox::Release() {
	glDeleteTextures(1, &m_tex);
}


void CSkybox::loadTexture(char* filename, int index) {
		
		CTargaImage img;

		if(!img.Load(filename)){			
			return;
		}				
	
		//if(index!=2 && index!=3)
		//	img.FlipVertical();
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+index, 0, GL_RGB, img.GetWidth(), img.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, img.GetImage());
		
		//gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, img.GetWidth(), img.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE, img.GetImage());
		img.Release();				
	}

