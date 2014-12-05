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
#define MAX_VERTICES_AMOUNT_EXPONENT 12
#define VERTICES_AMOUNT (1<<MAX_VERTICES_AMOUNT_EXPONENT)
#define GEOMETRY_SHADER_FILENAME "../res/shader/ex23.geom"

enum EVao
{
	VAO_VERTEX,
	VAO_TOTAL
};

enum EVbo
{
	VBO_POSITION,
	VBO_COLOR,
	VBO_TOTAL
};

bool g_wireframeEnabled;
bool g_blendEnabled;
bool g_zoomMode;
float g_cameraTranslation;
glm::ivec2 g_prevMouse;
glm::ivec2 g_cameraRotationXY;
float g_particleSpeed;
bool g_particleMotionEnabled;
int g_drawVerticesAmountExponent;
GLuint g_program, g_attribPosition, g_attribColor, g_uniformModelViewMatrix, g_uniformProjectionMatrix, g_uniformSpeed;
GLuint g_vbo[VBO_TOTAL];
GLuint g_vao[VAO_TOTAL];
glm::mat4 g_modelView(1), g_projection;

// ============================== Helper Functions =========================

GLuint buildProgram(const char* srcVert, const char* srcGeom, const char* srcFrag) {

	printf("\nCreating program");

	bool OK = true;

	GLuint vsId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vsId, 1, (const GLchar**) &srcVert, NULL);
	glCompileShader(vsId);
	OK = OK && cgl::checkShaderCompilationStatus(vsId);
#ifdef PARTIAL
	//TODO: Create Geometry shader from source
#else
	GLuint gsId = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(gsId, 1, (const GLchar**) &srcGeom, NULL);
	glCompileShader(gsId);
	OK = OK && cgl::checkShaderCompilationStatus(gsId);
#endif
	GLuint fsId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fsId, 1, (const GLchar**) &srcFrag, NULL);
	glCompileShader(fsId);
	OK = OK && cgl::checkShaderCompilationStatus(fsId);

	if(!OK) 
	{
		printf("Failed to compile, quitting!\n");
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vsId);
#ifdef PARTIAL
	//TODO: Attach geomtry shader
#else
	glAttachShader(program, gsId);
#endif
	glAttachShader(program, fsId);

	// Now ready to link 
	glLinkProgram(program);

	glValidateProgram(program);

	if(cgl::checkProgramInfoLog(program) == 0) 
	{
		printf("Failed to create program from shaders");
		return 0;
	} 
	else 
	{
		printf("Program created!\n");
	}

	return program;
}

void initVertexBufferObjects()
{
	// Set initial states for particles (allocate on heap because might be too big fro stack)
	std::vector<glm::vec3> vertices(VERTICES_AMOUNT);
	std::vector<glm::vec3> colors(VERTICES_AMOUNT);

	for(int i=0; i<VERTICES_AMOUNT; ++i)
	{
		vertices[i] = glm::linearRand(glm::vec3(-1,-1,-1),glm::vec3(1,1,1));			
		colors[i] = glm::rgbColor(glm::vec3(360*static_cast<float>(rand())/RAND_MAX,1,1));
	}

	// Create identifiers
	glGenBuffers(VBO_TOTAL, g_vbo);

	// Transfer data
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION]);
	glBufferData(GL_ARRAY_BUFFER, VERTICES_AMOUNT*3*sizeof(float), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_COLOR]);
	glBufferData(GL_ARRAY_BUFFER, VERTICES_AMOUNT*3*sizeof(float), colors.data(), GL_STATIC_DRAW);	
}

void initVertexArrayObjects()
{
	// Keep all vertex attribute states in VAO
	glGenVertexArrays(2, g_vao);

	// Define the two drawing states
	glBindVertexArray(g_vao[VAO_VERTEX]);
	{
		glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION]);
		glVertexAttribPointer(g_attribPosition, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));

		glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_COLOR]);
		glVertexAttribPointer(g_attribColor, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));	

		glEnableVertexAttribArray(g_attribPosition);
		glEnableVertexAttribArray(g_attribColor);		
	}
	glBindVertexArray(0);
}

void drawParticles() 
{
	glBindVertexArray(g_vao[VAO_VERTEX]);
	{
		glDrawArrays(GL_POINTS, 0, 1<<g_drawVerticesAmountExponent);				
	}
	glBindVertexArray(0);
}

void initShaders() {
	
	const char* srcVert = 
		"#version 150\n"
		"out vec4  vColor;\n"
		"in vec3 aPosition;\n"
		"in vec3 aColor;\n"
#ifdef PARTIAL		
		"uniform mat4 uProjectionMatrix;\n"
#endif
		"uniform mat4 uModelViewMatrix;\n"
		"uniform float uSpeed;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	vColor = vec4(aColor,1);\n"
#ifdef PARTIAL		
		"	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition,1);\n"
#else
		"	gl_Position = uModelViewMatrix * vec4(aPosition,1);\n"
#endif
		"}\n";

	const char* srcFrag = 
#ifdef PARTIAL
		"#version 150\n"
		"in vec4  vColor;\n"
		"out vec4 oColor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	oColor = vColor;\n"
		"}\n";
#else
		"#version 150\n"
		"in vec4  gColor;\n"
		"out vec4 oColor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	oColor = gColor;\n"
		"}\n";
#endif
	std::string srcGeomStr = cgl::Shader::readFile(GEOMETRY_SHADER_FILENAME);

	g_program = buildProgram(srcVert, srcGeomStr.c_str(), srcFrag);	
	g_attribPosition = glGetAttribLocation(g_program, "aPosition");
	g_attribColor = glGetAttribLocation(g_program, "aColor");
	g_uniformModelViewMatrix = glGetUniformLocation(g_program, "uModelViewMatrix");
	g_uniformProjectionMatrix = glGetUniformLocation(g_program, "uProjectionMatrix");
	g_uniformSpeed = glGetUniformLocation(g_program, "uSpeed");
}


void init() 
{	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	g_zoomMode = false;
	g_drawVerticesAmountExponent = MAX_VERTICES_AMOUNT_EXPONENT;
	g_particleMotionEnabled = true;
	g_particleSpeed = 0.001f;

	// Init shaders
	initShaders();

	// Set background color to gray
	glClearColor(0, 0, 0, 0);

	// Init states		
	glPointSize(2);

	// Place camera	
	g_modelView = glm::translate(g_modelView, glm::vec3(0,0,-1.5));

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
	// Want to translate along local Z (0,0,1,0), which is the 3rd row of the matrix
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

	if(g_blendEnabled)
	{
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	}
	glPolygonMode(GL_FRONT_AND_BACK, g_wireframeEnabled?GL_LINE:GL_FILL);
	
	// Apply shaders
	glUseProgram(g_program);

	// Create camera transformation
	setupCamera();		
	// Save camera transformation
	glm::mat4 saveMat = g_modelView;
	
	// Update model-view matrix
	glUniformMatrix4fv(g_uniformModelViewMatrix, 1, false, glm::value_ptr(g_modelView));	
	// Update particles speed
	glUniform1f(g_uniformSpeed, g_particleSpeed*static_cast<float>(g_particleMotionEnabled));	
	drawParticles();

	// Load camera transformation
	g_modelView = saveMat;

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {

	// Setup viewport
	glViewport(0,0,width,height);

	// Setup projection transformation
	g_projection = glm::perspective(90.0f, (float)width/height, 0.01f, 10.0f);

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
		g_cameraTranslation += dy/100.0f;
	}
	else
	{
		// Rotate model
		g_cameraRotationXY.x += dx;
		g_cameraRotationXY.y += dy;
	}

	// Remember mouse location 
	g_prevMouse = glm::ivec2(x,y);	

	// Don't move particles during mouse move
	g_particleMotionEnabled = false;
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

	// Resume particles motion
	g_particleMotionEnabled = true;
}

void keyboardSpecialFunc(int key, int x, int y) 
{
	switch(key) {
	case GLUT_KEY_DOWN: 
		g_particleSpeed -= 0.001f;
		break;
	case GLUT_KEY_UP:
		g_particleSpeed += 0.001f;
		break;
	case GLUT_KEY_LEFT:
		g_drawVerticesAmountExponent = std::max<int>(g_drawVerticesAmountExponent-1, 0);
		std::cout << "Vertices drawn: " << (1<<g_drawVerticesAmountExponent) << std::endl;
		break;
	case GLUT_KEY_RIGHT:
		g_drawVerticesAmountExponent = std::min<int>(g_drawVerticesAmountExponent+1, MAX_VERTICES_AMOUNT_EXPONENT);
		std::cout << "Vertices drawn: " << (1<<g_drawVerticesAmountExponent) << std::endl;
		break;
	case GLUT_KEY_F1:
		g_blendEnabled ^= true;
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

	glutInitContextVersion(3,2);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_DEBUG);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH)-512)/2, (glutGet(GLUT_SCREEN_HEIGHT)-512)/2);
	glutInitWindowSize(512, 512);

	glutCreateWindow("ex23 - Geometry Shader Circle Points");

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

	init();

	std::cout << "Usage:" << std::endl;
	std::cout << "left arrow/right arrow		Draw and process less/more particles" << std::endl;
	std::cout << "down arrow/up arrow			Speed up or down particles" << std::endl;
	std::cout << "Use mouse to look around" << std::endl;

	glutMainLoop();

	return 0;  
}
