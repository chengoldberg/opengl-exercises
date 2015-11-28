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
#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))

#define SCREEN_ALIGNED_QUAD_VERTEX_SHADER "../res/shader/screenAlignedQuad.vert"
#define SCREEN_ALIGNED_QUAD_FRAGMENT_SHADER "../res/shader/screenAlignedQuad.frag"

cgl::Program g_program;
cgl::SimpleMesh g_screenAlignedQuad;

void initShaders() 
{
	std::vector<cgl::Shader> shaders;
	shaders.push_back(cgl::Shader::fromFile(GL_VERTEX_SHADER, SCREEN_ALIGNED_QUAD_VERTEX_SHADER));
	shaders.push_back(cgl::Shader::fromFile(GL_FRAGMENT_SHADER, SCREEN_ALIGNED_QUAD_FRAGMENT_SHADER));	
	g_program.build(shaders);
}

void init() 
{
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
	
}
 
void display(void) 
{
	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT);		

	// Apply shaders
	g_program.use();

	g_screenAlignedQuad.render();

	// Swap double buffer
	glutSwapBuffers();
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
