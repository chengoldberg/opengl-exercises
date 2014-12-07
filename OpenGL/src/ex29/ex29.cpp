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
#include "CTargaImage.h"
#include "CTargaImage.cpp"

#define M_PI       3.14159265358979323846

#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))

#define MESH_PLANE_FILEPATH "../res/mesh/crate.off"
#define MESH_BOMBER_FILEPATH "../res/mesh/bomber1.off"
#define MESH_SPHERE_FILEPATH "../res/mesh/sphere.off"

enum EVao
{
	VAO_VERTEX,
	VAO_TOTAL
};

enum EBufferObject
{
	VBO_POSITION,
	EBO_TRIANGLES,
	BUFFER_OBJECTS_TOTAL
};

enum ETextureObjects
{
	TEX_FROM_FILE,
	TEXTURE_OBJECTS_TOTAL
};

enum EQueryObject
{
	QUERY_SAMPLES_PASSED,
	QUERY_UPLOAD_TIME,
	QUERY_DOWNLOAD_TIME,
	QUERY_CALC_TIME,
	QUERY_OBJECTS_TOTAL
};

bool g_showCheapMesh;
bool g_zoomMode;
GLuint g_queries[QUERY_OBJECTS_TOTAL];
glm::ivec2 g_prevMouse;
GLuint g_program, g_attribPosition, g_uniformModelViewMatrix, g_uniformProjectionMatrix, g_uniformDiffuse;
cgl::SimpleMesh g_ocludeeMesh, g_ocluderMesh, g_cheapMesh;
GLuint g_timerQuery;
int g_totalElements;
glm::mat4 g_modelView(1), g_projection;
std::vector<glm::vec3> g_vertices;
glm::ivec2 g_cameraRotationXY;
float g_cameraTranslation;
GLuint g_textures[TEXTURE_OBJECTS_TOTAL];
glm::ivec2 g_texSize;

// ============================== Helper Functions =========================

GLuint buildProgram(const char* srcVert, const char* srcFrag) 
{
	printf("\nCreating program");

	bool OK = true;

	GLuint vsId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vsId, 1, (const GLchar**) &srcVert, NULL);
	glCompileShader(vsId);
	OK = OK && cgl::checkShaderCompilationStatus(vsId);

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

void initMeshes()
{
	g_ocludeeMesh.init(MESH_BOMBER_FILEPATH, g_program);
	g_ocluderMesh.init(MESH_PLANE_FILEPATH, g_program);
	g_cheapMesh.init(MESH_SPHERE_FILEPATH, g_program);
}

void initShaders() {
	const char* srcVert = 
		"#version 150\n"
		"in vec3 aPosition;\n"
		"uniform mat4 uModelViewMatrix;\n"
		"uniform mat4 uProjectionMatrix;\n"
		"out vec4 fColor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition, 1);\n"
		"	fColor = vec4(1,0,0,1);\n"
		"}\n";

	const char* srcFrag = 
		"#version 150\n"
		"uniform vec4 uDiffuse;\n"
		"in vec4 fColor;\n"
		"out vec4 oColor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	oColor = uDiffuse;\n"
		"}\n";

	try
	{
		g_program = buildProgram(srcVert, srcFrag);	
	}
	catch(std::exception ex)
	{
		std::cout << ex.what();
		throw ex;
	}
	g_attribPosition = glGetAttribLocation(g_program, "aPosition");
	g_uniformModelViewMatrix = glGetUniformLocation(g_program, "uModelViewMatrix");
	g_uniformProjectionMatrix = glGetUniformLocation(g_program, "uProjectionMatrix");
	g_uniformDiffuse = glGetUniformLocation(g_program, "uDiffuse");
}


void init() 
{	
	g_showCheapMesh = false;
	glEnable(GL_DEPTH_TEST);
	g_zoomMode = false;
	glGenQueries(QUERY_OBJECTS_TOTAL, g_queries);

	// Init shaders
	initShaders();

	initMeshes();

	// Set background color to gray
	glClearColor(0.25, 0.25, 0.25, 1);

	// Init feedback vertex data
	
	// Place camera	
	g_modelView = glm::translate(g_modelView, glm::vec3(0,0,-1.5));

	glGenQueries(1, &g_timerQuery);
}
 
/**
* Create camera transformation such that the model is rotated around the
* world's X-axis and Y-axis. 
*/
void setupCamera() 
{	
	g_modelView = glm::translate(g_modelView, g_cameraTranslation*glm::vec3(glm::transpose(g_modelView)[2]));

	glm::vec3 vecY(g_modelView[0][1], g_modelView[1][1], g_modelView[2][1]);
	g_modelView = glm::rotate(g_modelView, (float)-g_cameraRotationXY.x, vecY);

	glm::vec3 vecX(g_modelView[0][0], g_modelView[1][0], g_modelView[2][0]);
	g_modelView = glm::rotate(g_modelView, (float)-g_cameraRotationXY.y, vecX);

	g_cameraRotationXY = glm::ivec2(0,0);
	g_cameraTranslation = 0;
}

void display(void) 
{
	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		

	// Apply shaders
	glUseProgram(g_program);

	// Create camera transformation
	setupCamera();		
	// Save camera transformation
	
	// Update model-view matrix	
	
	// Print measured drawing time of last draw (if any) to prevent stall
	static int frameCounter = 1;
	if(frameCounter++ % 33 == 0)
	{
		GLuint64 drawTime;
		glGetQueryObjectui64v(g_timerQuery, GL_QUERY_RESULT, &drawTime);
		std::cout << "Draw time: " << drawTime/1000000 << "ms" << std::endl;
	}

	// draw Meshes
	glBeginQuery(GL_TIME_ELAPSED, g_timerQuery);	

	const float radius = 14;
	for(int i=0;i<12;++i)
	{
		float alpha = (i/12.0f)*2*M_PI;
		glUniformMatrix4fv(g_uniformModelViewMatrix, 1, false, 
			glm::value_ptr(glm::translate(g_modelView, radius*glm::vec3(cos(alpha),0,sin(alpha)))));	
		glUniform4f(g_uniformDiffuse, 1,1,0,1);
		g_ocluderMesh.render();
	}

	glUniformMatrix4fv(g_uniformModelViewMatrix, 1, false, 
		glm::value_ptr(glm::scale(
		glm::translate(g_modelView, glm::vec3(0,0,0)), glm::vec3(3,3,3))));	
	glUniform4f(g_uniformDiffuse, 1,0,0,1);

	if(!g_showCheapMesh)
	{
		glColorMask(false, false, false, false);
		glDepthMask(false);
	}

	glBeginQuery(GL_SAMPLES_PASSED, g_queries[QUERY_SAMPLES_PASSED]);
	g_cheapMesh.render();
	glEndQuery(GL_SAMPLES_PASSED);
	
	if(!g_showCheapMesh)
	{
		glColorMask(true, true, true, true);
		glDepthMask(true);
		glUniformMatrix4fv(g_uniformModelViewMatrix, 1, false, 
			glm::value_ptr(glm::translate(g_modelView, glm::vec3(0,0,0))));	
		glUniform4f(g_uniformDiffuse, 0,0,1,1);

		glBeginConditionalRender(g_queries[QUERY_SAMPLES_PASSED], GL_QUERY_WAIT);
		g_ocludeeMesh.render();
		glEndConditionalRender();
	}

	glEndQuery(GL_TIME_ELAPSED);

	if(g_showCheapMesh)
	{
		int samplesPassed;
		glGetQueryObjectiv(g_queries[QUERY_SAMPLES_PASSED], GL_QUERY_RESULT, &samplesPassed);
		std::cout << samplesPassed << std::endl;
	}
	
	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {

	// Setup viewport
	glViewport(0,0,width,height);

	// Setup projection transformation
	//g_projection = glm::ortho(-5.0f,5.0f,-5.0f, 5.0f, -1.0f, 1.0f);	
	//g_projection = glm::ortho(0.0f,100.0f,0.0f, 100.0f, -1.0f, 1.0f);	
	g_projection = glm::perspective(90.0f, (float)width/height, 0.01f, 1000.0f);

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

	if(g_zoomMode)
	{
		g_cameraTranslation += dy/10.0f;
	}
	else
	{
		// Rotate model
		g_cameraRotationXY.x += dx;
		g_cameraRotationXY.y += dy;
	}

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
	case GLUT_KEY_F1:
		g_showCheapMesh ^= true;
		break;

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

	glutInitContextVersion(3,2);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_DEBUG);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH)-512)/2, (glutGet(GLUT_SCREEN_HEIGHT)-512)/2);
	glutInitWindowSize(512, 512);

	int windowId = glutCreateWindow("ex29 - Conditional Rendering");

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
	std::cout << "Use left mouse button to look around and right mouse button to zoom" << std::endl;

	glutMainLoop();

	return 0;  
}
