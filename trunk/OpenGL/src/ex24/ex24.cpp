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
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <string>
#include <iostream>
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/color_space.hpp>
#include <glm/gtc/random.hpp>
#include <vector>
#include "cgl/gl/common.hpp"

#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))
#define MAX_VERTICES_AMOUNT_EXPONENT 15
#define VERTICES_AMOUNT (1<<MAX_VERTICES_AMOUNT_EXPONENT)
#define GEOMETRY_SHADER_FILENAME "../res/shader/ex24_simple.geom"

enum EVao
{
	VAO_VERTEX,
	VAO_TOTAL
};

enum EVbo
{
	VBO_POSITION,
	VBO_TOTAL
};

bool g_wireframeEnabled;
bool g_blendEnabled;
bool g_zoomMode;
glm::ivec2 g_prevMouse;
GLuint g_program, g_attribPosition, g_uniformModelViewMatrix, g_uniformProjectionMatrix;
GLuint g_vbo[VBO_TOTAL];
GLuint g_vao[VAO_TOTAL];
glm::mat4 g_modelView(1), g_projection;
std::vector<glm::vec3> g_vertices;

// ============================== Helper Functions =========================

GLuint buildProgram(const char* srcVert, const char* srcGeom, const char* srcFrag) 
{
	printf("\nCreating program");

	bool OK = true;

	GLuint vsId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vsId, 1, (const GLchar**) &srcVert, NULL);
	glCompileShader(vsId);
	OK = OK && cgl::checkShaderCompilationStatus(vsId);

	GLuint gsId = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(gsId, 1, (const GLchar**) &srcGeom, NULL);
	glCompileShader(gsId);
	OK = OK && cgl::checkShaderCompilationStatus(gsId);
	
	GLuint fsId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fsId, 1, (const GLchar**) &srcFrag, NULL);
	glCompileShader(fsId);
	OK = OK && cgl::checkShaderCompilationStatus(fsId);

	if(!OK) 
	{
		throw std::exception("Failed to compile, quitting!\n");
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vsId);
	glAttachShader(program, gsId);
	glAttachShader(program, fsId);

	// Now ready to link 
	glLinkProgram(program);

	//glValidateProgram(program);

	if(cgl::checkProgramInfoLog(program) == 0) 
	{
		throw std::exception("Failed to create program from shaders");
	} 
	else 
	{
		std::cout << "Program created!" << std::endl;
	}

	return program;
}

void updateVertexBufferObjects()
{
	// Transfer data
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION]);
	glBufferData(GL_ARRAY_BUFFER, VERTICES_AMOUNT*3*sizeof(float), g_vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initVertexBufferObjects()
{
	// Set initial states for particles (allocate on heap because might be too big fro stack)
	g_vertices.push_back(glm::vec3(0,0,0));
	g_vertices.push_back(glm::vec3(1,1,0));
	g_vertices.push_back(glm::vec3(3,1,0));
	g_vertices.push_back(glm::vec3(3,4,0));

	// Create identifiers
	glGenBuffers(VBO_TOTAL, g_vbo);

	updateVertexBufferObjects();
}

void initVertexArrayObjects()
{
	// Keep all vertex attribute states in VAO
	glGenVertexArrays(VAO_TOTAL, g_vao);

	// Define the two drawing states
	glBindVertexArray(g_vao[VAO_VERTEX]);
	{
		glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION]);
		glVertexAttribPointer(g_attribPosition, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));
		glEnableVertexAttribArray(g_attribPosition);
	}
	glBindVertexArray(0);
}

void drawLines() 
{
	glBindVertexArray(g_vao[VAO_VERTEX]);
	{
		glDrawArrays(GL_LINE_STRIP, 0, g_vertices.size());				
	}
	glBindVertexArray(0);
}

void initShaders() {
	
	const char* srcVert = 
		"#version 330\n"
		"in vec3 aPosition;\n"
		"uniform mat4 uModelViewMatrix;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	gl_Position = uModelViewMatrix * vec4(aPosition,1);\n"
		"}\n";

	const char* srcFrag = 
		"#version 330\n"
		"out vec4 oColor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	oColor = vec4(1,1,1,1);\n"
		"}\n";

	std::string srcGeomStr = cgl::Shader::readFile(GEOMETRY_SHADER_FILENAME);

	try
	{
		g_program = buildProgram(srcVert, srcGeomStr.c_str(), srcFrag);	
	}
	catch(std::exception ex)
	{
		std::cout << ex.what();
		throw ex;
	}
	g_attribPosition = glGetAttribLocation(g_program, "aPosition");
	g_uniformModelViewMatrix = glGetUniformLocation(g_program, "uModelViewMatrix");
	g_uniformProjectionMatrix = glGetUniformLocation(g_program, "uProjectionMatrix");
}


void init() 
{	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	g_zoomMode = false;

	// Init shaders
	initShaders();

	// Set background color to gray
	glClearColor(0, 0, 0, 0);

	// Init states		
	glPointSize(2);

	// Place camera	
	//g_modelView = glm::translate(g_modelView, glm::vec3(0,0,-1.5));

	// Init feedback vertex data
	initVertexBufferObjects();

	// Init vertex data
	initVertexArrayObjects();
}
 
/**
* Create camera transformation such that the model is rotated around the
* world's X-axis and Y-axis. 
*/
void setupCamera() 
{	
	g_modelView = glm::mat4(1);
}

void display(void) 
{
	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		

	if(g_blendEnabled)
	{
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}
	glPolygonMode(GL_FRONT_AND_BACK, g_wireframeEnabled?GL_LINE:GL_FILL);
	
	// Apply shaders
	glUseProgram(g_program);

	// Create camera transformation
	setupCamera();		
	// Save camera transformation
	
	// Update model-view matrix
	glUniformMatrix4fv(g_uniformModelViewMatrix, 1, false, glm::value_ptr(g_modelView));	
	
	// draw lines
	drawLines();

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {

	// Setup viewport
	glViewport(0,0,width,height);

	// Setup projection transformation
	g_projection = glm::ortho(-5.0f,5.0f,-5.0f, 5.0f, -1.0f, 1.0f);
	

	// Create projection transformation	
	glUseProgram(g_program); // Remember to use program when setting variables
	glUniformMatrix4fv(g_uniformProjectionMatrix, 1, false, glm::value_ptr(g_projection));
	glUseProgram(0);
}

void motionFunc(int x, int y) 
{
	// Calc difference from previous mouse location
	glm::ivec2 prev = g_prevMouse;
	int dx = prev.x - x;
	int dy = prev.y - y;

	// Remember mouse location 
	g_prevMouse = glm::ivec2(x,y);	
}

void mouseFunc(int button, int state, int x, int y) 
{
	if(button == GLUT_LEFT_BUTTON)
	{
		g_zoomMode = false;
		g_prevMouse = glm::ivec2(x,y);			
	}
	if(button == GLUT_RIGHT_BUTTON)
	{
		g_zoomMode = true;
		g_prevMouse = glm::ivec2(x,y);	
	}
}

void keyboardSpecialFunc(int key, int x, int y) 
{
	switch(key) {
	case GLUT_KEY_DOWN: 
		break;
	}
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

void timer(int value) 
{
	glutPostRedisplay();
	glutTimerFunc(16,timer,0);
}

int main(int argc, char **argv) {

	glutInitContextVersion(3,3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_DEBUG);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(512, 512);

	int windowId = glutCreateWindow("ex24 - Decorated Geometry Lines");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);	
	glutMotionFunc(motionFunc);
	glutMouseFunc(mouseFunc);
	glutSpecialFunc(keyboardSpecialFunc);
	glutKeyboardFunc(keyboardFunc);
	glutTimerFunc(16,timer,0);
	//glutInit
	//glutFullScreen();

	// Glew limitation 
	// Ref: http://openglbook.com/glgenvertexarrays-access-violationsegfault-with-glew/
	glewExperimental = GL_TRUE; 
	glewInit();

	try
	{
		init();
	}
	catch(std::exception ex)
	{
		// Wait key
		glutDestroyWindow(windowId);
		std::getchar();
		return -1;
	}

	std::cout << "Usage:" << std::endl;
	std::cout << "left arrow/right arrow		Draw and process less/more particles" << std::endl;
	std::cout << "down arrow/up arrow			Speed up or down particles" << std::endl;
	std::cout << "Use mouse to look around" << std::endl;

	glutMainLoop();

	return 0;  
}
