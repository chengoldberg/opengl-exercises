/*
 * Copyright (C) 2013  Chen Goldberg
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

//#define GLM_PRECISION_HIG

#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <string>
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "cgl\gl\common.hpp"
#include <string>
#include <sstream>
#include "CTargaImage.h"
#include "CTargaImage.cpp"

#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))

#define MORPH_INTERPOLATON_VERTEX_SHADER "../res/shader/morphInterpolation.vert"
#define MORPH_INTERPOLATON_FRAGMENT_SHADER "../res/shader/morphInterpolation.frag"

int g_frame;
cgl::Program g_program;
cgl::SimpleMesh g_screenAlignedQuad;
GLuint g_textures[2];

struct MorphTemplate
{
	struct Segment
	{
		Segment(glm::vec2 p0, glm::vec2 p1) : m_p0(p0), m_p1(p1)
		{
			update();
		}

		void update()
		{
			glm::vec2 vec = m_p1-m_p0;
			m_n = glm::length(vec);
			m_v = vec/m_n;
		}

		glm::vec2 m_p0;
		glm::vec2 m_p1;
		glm::vec2 m_v;
		float m_n;
	};
	
	MorphTemplate(std::string name, std::vector<glm::vec2> lines) : m_name(name), m_scale(1), m_translate(0)
	{
		for(int i=0; i<lines.size(); i+=2)
		{
			m_segments.push_back(Segment(lines[i], lines[i+1]));
		}
	}

	MorphTemplate(std::string name, MorphTemplate& template0, MorphTemplate& template1, float alpha) : m_name(name), m_scale(1), m_translate(0)
	{
		for(int i=0; i<template0.m_segments.size(); ++i)
		{			
			m_segments.push_back(Segment(				
				glm::mix(template0.m_segments[i].m_p0, template1.m_segments[i].m_p0, alpha),
				glm::mix(template0.m_segments[i].m_p1, template1.m_segments[i].m_p1, alpha)));
		}
	}
	
	std::string getUniformMember(int index, std::string memberName)
	{
		std::stringstream ss;
		ss << "segments" << m_name << "[" << index << "]." << memberName;
		return ss.str();
	}

	void ApplyTransform(float scale, glm::vec2 translate)
	{
		m_scale = scale;
		m_translate = translate;
		for(int i=0; i<m_segments.size(); ++i)
		{
			m_segments[i].m_p0 = m_segments[i].m_p0*scale + translate;
			m_segments[i].m_p1 = m_segments[i].m_p1*scale + translate;
			m_segments[i].update();
		}
	}

	glm::mat3x2 getTransform()
	{
		return glm::mat3x2(1/m_scale, 0, -m_translate.x/m_scale, 0, 1/m_scale, -m_translate.y/m_scale);
	}

	void updateUniforms()
	{		
		for(int index=0; index < m_segments.size(); ++index)
		{
			glUniform2fv(glGetUniformLocation(g_program.getId(), getUniformMember(index, "p").c_str()), 1, (float*) &m_segments[index].m_p0);
			glUniform2fv(glGetUniformLocation(g_program.getId(), getUniformMember(index, "v").c_str()), 1, (float*) &m_segments[index].m_v);
			glUniform1f(glGetUniformLocation(g_program.getId(), getUniformMember(index, "n").c_str()), m_segments[index].m_n);
		}

		std::stringstream ss;
		ss << "trans" << m_name;
		glUniformMatrix3x2fv(glGetUniformLocation(g_program.getId(), ss.str().c_str()), 1, GL_TRUE, (GLfloat*) &getTransform());
	}


	std::string m_name;
	std::vector<Segment> m_segments;
	float m_scale;
	glm::vec2 m_translate;
};


class Morpher
{
public:

	void update(float alpha)
	{
		glUniform1f(glGetUniformLocation(g_program.getId(), "alpha"), alpha);
		MorphTemplate templateMid("Mid", *m_templateSrc, *m_templateDst, alpha);
		templateMid.updateUniforms();
		
	}

	MorphTemplate* m_templateSrc;
	MorphTemplate* m_templateDst;

	GLuint m_texIdSrc;
	GLuint m_texIdDst;
};

Morpher g_morpher;

void initShaders() 
{
	std::vector<cgl::Shader> shaders;
	shaders.push_back(cgl::Shader::fromFile(GL_VERTEX_SHADER, MORPH_INTERPOLATON_VERTEX_SHADER));
	shaders.push_back(cgl::Shader::fromFile(GL_FRAGMENT_SHADER, MORPH_INTERPOLATON_FRAGMENT_SHADER));	
	g_program.build(shaders);
}

void init() 
{
	g_frame = 0;

	// Init shaders
	initShaders();

	// Set background color to gray
	glClearColor(0.5f, 0.5f, 0.5f, 0);

	// Parse vertex attributes
	float quad[] = {
		+1,-1,
		-1,-1,
		+1,+1,
		-1,+1};

	g_screenAlignedQuad.addAttrib("aPosition", 2, quad, sizeof(quad), GL_FLOAT);
	g_screenAlignedQuad.setDrawMode(GL_TRIANGLE_STRIP);
	g_screenAlignedQuad.initAttribLocs(g_program.getId());
	g_screenAlignedQuad.initBuffers();
	
	std::vector<glm::vec2> linesSrc;
	linesSrc.push_back(glm::vec2(292, 160));linesSrc.push_back(glm::vec2(282, 116));
	linesSrc.push_back(glm::vec2(282, 116));linesSrc.push_back(glm::vec2(294, 56));
	linesSrc.push_back(glm::vec2(292, 160));linesSrc.push_back(glm::vec2(331, 163));

	std::vector<glm::vec2> linesDst;
	linesDst.push_back(glm::vec2(525, 303));linesDst.push_back(glm::vec2(509, 253));
	linesDst.push_back(glm::vec2(509, 253));linesDst.push_back(glm::vec2(559, 250));
	linesDst.push_back(glm::vec2(525, 303));linesDst.push_back(glm::vec2(564, 306));

	g_morpher.m_templateSrc = new MorphTemplate("Src", linesSrc);	
	g_morpher.m_templateDst = new MorphTemplate("Dst", linesDst);

	g_morpher.m_templateSrc->ApplyTransform(1, glm::vec2((292+525)/2-292, (160+303)/2-160));
	g_morpher.m_templateDst->ApplyTransform(1, glm::vec2((292+525)/2-525, (160+303)/2-303));

	g_program.use();
	g_morpher.m_templateSrc->updateUniforms();
	g_morpher.m_templateDst->updateUniforms();

	#define TEXTURE_FILENAME_EXERCISE "../res/tex_2d/exercise.tga"
	CTargaImage img;
	GLuint texId;
	
	glGenTextures(1, &texId);	

	if(!img.Load(TEXTURE_FILENAME_EXERCISE))
	{
		printf("Unable to load %s\n", TEXTURE_FILENAME_EXERCISE);
	}
	img.FlipVertical();
    glGenTextures(2, g_textures);
    glBindTexture(GL_TEXTURE_2D, g_textures[0]);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );       
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.GetWidth(), img.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, img.GetImage());
/*        
    glBindTexture(GL_TEXTURE_2D, textures[1])
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT )
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT )
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR )
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR )        
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_BGR, GL_UNSIGNED_BYTE, skeletonDst.img.tostring())
       */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_textures[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_textures[1]);

}
 
void display(void) 
{
	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT);		

	// Apply shaders
	g_program.use();	
	
	float alpha = 0.5+0.5*cos(g_frame/30.0);
	g_morpher.update(alpha);

	g_screenAlignedQuad.render();

	// Swap double buffer
	glutSwapBuffers();
	g_frame++;
}

void reshape(int width, int height) {

	// Setup viewport
	glViewport(0,0,width,height);
}

void keyboardFunc(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:	// Escape key
		exit(0);
		break;

	default:
		break;
	}
}

int main(int argc, char **argv) {

	glutInitContextVersion(3,2);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_DEBUG);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH)-512)/2, (glutGet(GLUT_SCREEN_HEIGHT)-512)/2);
	glutInitWindowSize(512, 512);

	glutCreateWindow("ex30 - Morph");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutKeyboardFunc(keyboardFunc);
	//glutFullScreen();

	// Glew limitation 
	// Ref: http://openglbook.com/glgenvertexarrays-access-violationsegfault-with-glew/
	glewExperimental = GL_TRUE; 
	glewInit();

	init();

	glutMainLoop();

	return 0;  
}
