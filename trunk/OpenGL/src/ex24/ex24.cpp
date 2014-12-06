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
#define GEOMETRY_SHADER_FILENAME "../res/shader/ex24.geom"

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
bool g_multisampleEnabled;
bool g_zoomMode;
float g_radius;
glm::ivec2 g_prevMouse;
GLuint g_program, g_attribPosition, g_uniformModelViewMatrix, g_uniformProjectionMatrix, g_uniformRadius;
GLuint g_vbo[VBO_TOTAL];
GLuint g_vao[VAO_TOTAL];
GLuint g_fbo;
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
	glBufferData(GL_ARRAY_BUFFER, g_vertices.size()*sizeof(glm::vec3), g_vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initVertexBufferObjects()
{
	// Set initial states for particles (allocate on heap because might be too big fro stack)
	/*
	g_vertices.push_back(glm::vec3(0,0,0));
	g_vertices.push_back(glm::vec3(1,1,0));
	g_vertices.push_back(glm::vec3(3,1,0));
	g_vertices.push_back(glm::vec3(3,4,0));
	g_vertices.push_back(glm::vec3(0,3,0));
	g_vertices.push_back(glm::vec3(-1,3.5,0));
	g_vertices.push_back(glm::vec3(-2,1.5,0));
	g_vertices.push_back(glm::vec3(-3,0,0));
	g_vertices.push_back(glm::vec3(-2,-1,0));
	g_vertices.push_back(glm::vec3(-2,0,0));
	g_vertices.push_back(glm::vec3(-1,0,0));
	*/

	float data [][2] = {{9.464286,38.928572},{11.071428,39.821429},{21.071429,48.214286},{26.250000,46.250000},{29.107143,44.642857},{28.571429,36.964286},{38.928572,40.892857},{48.928572,39.107143},{46.250000,36.785714},{49.642857,34.642857},{53.571429,38.214286},{68.035714,37.321429},{78.928572,41.071429},{82.142857,42.321429},{90.000000,44.821429},{93.571429,47.857143},{94.107143,48.750000},{90.892857,48.750000},{94.642857,51.607143},{92.857143,53.214286},{90.892857,51.964286},{87.500000,55.714286},{75.535714,61.785714},{65.892857,63.928572},{62.857143,72.678572},{50.535714,70.892857},{41.250000,67.500000},{43.571429,62.857143},{38.392857,61.428572},{33.214286,65.357143},{26.250000,61.071429},{29.642857,57.678572},{20.535714,54.285714},{9.642857,63.928572},{6.428571,62.857143},{6.785714,59.285714},{9.464286,50.892857},{6.964286,42.321429}};
	for(int i=0;i<sizeof(data)/8;++i)
	{
		g_vertices.push_back(glm::vec3(data[i][0],data[i][1],0));
	}

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

void initFrameBufferObject(int width, int height)
{
	GLuint rbo;

	glGenFramebuffers(1, &g_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);

	// Now a depth buffer!	
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);

	GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		printf("Bad framebuffer init!\n");

	// Get back to defualt framebuffer!
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void drawLines() 
{
	glBindVertexArray(g_vao[VAO_VERTEX]);
	{
		//glDrawArrays(GL_LINE_STRIP, 0, g_vertices.size());				
		glDrawArrays(GL_LINE_STRIP_ADJACENCY, 0, g_vertices.size());						
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
		"#define M_PI 3.1415926535897932384626433832795\n"
		"out vec4 oColor;\n"
		"in vec4 gColor;\n"
		"in float gDist;\n"
		"\n"
		"void main()\n"
		"{\n"
//		"	oColor = vec4(1,1,1,1)*cos(gDist*M_PI/2);\n"
		"	oColor = vec4(1,1,1,1);\n"
		"	if(gDist>0.5) oColor=vec4(1,0,0,1);\n"
		"	//oColor = gColor;\n"
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
	g_uniformRadius = glGetUniformLocation(g_program, "uRadius");
}


void init() 
{	
	g_wireframeEnabled = true;
	g_multisampleEnabled = false;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	g_radius = 2;
	g_zoomMode = false;

	// Init shaders
	initShaders();

	// Set background color to gray
	glClearColor(0, 0, 0, 0);

	// Init states		
	glPointSize(2);

	glEnable(GL_MULTISAMPLE);

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

	if(g_multisampleEnabled)
	{
		glEnable(GL_MULTISAMPLE);
	}
	else
	{
		glDisable(GL_MULTISAMPLE);
	}
	glPolygonMode(GL_FRONT_AND_BACK, g_wireframeEnabled?GL_LINE:GL_FILL);
	
	// Apply shaders
	glUseProgram(g_program);

	// Create camera transformation
	setupCamera();		
	// Save camera transformation
	
	// Update model-view matrix
	glUniformMatrix4fv(g_uniformModelViewMatrix, 1, false, glm::value_ptr(g_modelView));	
	
	glUniform1f(g_uniformRadius, g_radius);

	//glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
	// draw lines
	drawLines();
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	/*
	for(auto& vertex :g_vertices)
	{
		vertex.x += glm::gaussRand(0.0f,0.2f);
		vertex.y += glm::gaussRand(0.0f,0.2f);
	}
	updateVertexBufferObjects();
	*/
	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {

	// Setup viewport
	glViewport(0,0,width,height);

	// Setup projection transformation
	//g_projection = glm::ortho(-5.0f,5.0f,-5.0f, 5.0f, -1.0f, 1.0f);	
	float aspectRatio = static_cast<float>(width)/height;
	g_projection = glm::ortho(0.0f,100.0f,50-50/aspectRatio, 50+50/aspectRatio, -1.0f, 1.0f);	

	initFrameBufferObject(width, height);

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
		g_radius -= 0.15;
		break;
	case GLUT_KEY_UP:
		g_radius += 0.15;
		break;
	case GLUT_KEY_F1:
		g_multisampleEnabled ^= true;
		break;
	case GLUT_KEY_F2:
		g_wireframeEnabled ^= true;
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
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
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
	std::cout << "F1							Toggle Multisample" << std::endl;
	std::cout << "F2							Toggle Wireframe" << std::endl;
	std::cout << "down arrow/up arrow			Change radius" << std::endl;
	std::cout << "Use mouse to look around" << std::endl;

	glutMainLoop();

	return 0;  
}
