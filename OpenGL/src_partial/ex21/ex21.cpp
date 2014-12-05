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
#include <iostream>
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/color_space.hpp>
#include <glm/gtc/random.hpp>
#include <vector>

#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))
#define MAX_VERTICES_AMOUNT_EXPONENT 16
#define VERTICES_AMOUNT (1<<MAX_VERTICES_AMOUNT_EXPONENT)
struct Point
{
	int x,y;

	Point() {}
	Point(int _x, int _y) : x(_x), y(_y) {}
};

enum EVao
{
	VAO_VERTEX_0,
	VAO_VERTEX_1,
	VAO_TOTAL
};

enum EVbo
{
	VBO_POSITION_0,
	VBO_POSITION_1,
	VBO_DIRECTION_0,
	VBO_DIRECTION_1,
	VBO_COLOR,
	VBO_TOTAL
};

Point g_prevMouse;
double g_rotY;
double g_rotX;
float g_particleSpeed;
bool g_particleMotionEnabled;
int g_drawVerticesAmountExponent;
GLuint g_program, g_attribPosition, g_attribColor, g_attribDirection, g_uniformModelViewMatrix, g_uniformProjectionMatrix, g_uniformSpeed;
GLuint g_vbo[VBO_TOTAL];
GLuint g_vao[VAO_TOTAL];
glm::mat4 g_modelView(1), g_projection;

/*
 * Rotate view along x and y axes 
 * @param x Angles to rotate around x
 * @param y Angles to rotate around y
 */
void rotate(double x, double y) 
{
	g_rotX += x;
	g_rotY += y;
}

// ============================== Helper Functions =========================

int checkProgramInfoLog(GLuint prog) {
	int len = 0, read = 0;
	std::string log;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
	if (len > 0)
	{
		log.resize(len);
		glGetProgramInfoLog(prog, len, &read, (GLchar*) log.data());
		printf("Program Info Log:\n%s\n", log.c_str());
	}
	int ret;
	glGetProgramiv(prog, GL_LINK_STATUS, &ret);
	
	return ret;
}

bool myCompileShader(GLuint shader) {
	GLint compiled;

	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	GLint blen = 0;	GLsizei slen = 0;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &blen);       

	if (blen > 1)
	{
	 GLchar* compiler_log = (GLchar*)malloc(blen);
	 glGetInfoLogARB(shader, blen, &slen, compiler_log);
	 printf("compiler_log:\n");
	 printf("%s\n", compiler_log);
	 free (compiler_log);
	}

	return compiled == 1;
}

GLuint createProgramFast(const char* srcVert, const char* srcFrag) {

	printf("\nCreating program");

	bool OK = true;

	//puts("Compiling Vertex Shader\n");
	GLuint vsId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vsId, 1, (const GLchar**) &srcVert, NULL);
	OK = OK && myCompileShader(vsId);
	
	//puts("Compiling Fragment Shader\n");
	GLuint fsId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fsId, 1, (const GLchar**) &srcFrag, NULL);
	OK = OK && myCompileShader(fsId);

	if(!OK) {
		printf("Failed to compile, quitting!\n");
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vsId);
	glAttachShader(program, fsId);

	//TODO: Declare transform feedback variables

	// Now ready to link (since oPosition target is bound)
	glLinkProgram(program);

	//glValidateProgram(program);

	if(checkProgramInfoLog(program) == 0) {
		printf("Failed to create program from shaders");
		return 0;
	} else {
		printf("Program created!\n");
	}

	return program;
}

void initVertexBufferObjects()
{
	// Set initial states for particles (allocate on heap because might be too big fro stack)
	std::vector<glm::vec3> vertices(VERTICES_AMOUNT);
	std::vector<glm::vec3> directions(VERTICES_AMOUNT);
	std::vector<glm::vec3> colors(VERTICES_AMOUNT);

	for(int i=0; i<VERTICES_AMOUNT; ++i)
	{
		vertices[i] = glm::linearRand(glm::vec3(-1,-1,-1),glm::vec3(1,1,1));	
		directions[i] = glm::normalize(glm::ballRand(2.0f));		
		//colors[i] = glm::rgbColor(glm::vec3(360*static_cast<float>(rand())/RAND_MAX,1,1));
		colors[i] = 0.5f*(vertices[i]+glm::vec3(1,1,1));
	}

	// Create identifiers
	glGenBuffers(VBO_TOTAL, g_vbo);

	// Transfer data
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION_0]);
	glBufferData(GL_ARRAY_BUFFER, VERTICES_AMOUNT*3*sizeof(float), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_DIRECTION_0]);
	glBufferData(GL_ARRAY_BUFFER, VERTICES_AMOUNT*3*sizeof(float), directions.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_COLOR]);
	glBufferData(GL_ARRAY_BUFFER, VERTICES_AMOUNT*3*sizeof(float), colors.data(), GL_STATIC_DRAW);	

	// Allocate data for "double buffer"
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION_1]);
	glBufferData(GL_ARRAY_BUFFER, VERTICES_AMOUNT*3*sizeof(float), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_DIRECTION_1]);
	glBufferData(GL_ARRAY_BUFFER, VERTICES_AMOUNT*3*sizeof(float), NULL, GL_STATIC_DRAW);

}

void initVertexArrayObjects()
{
	// Keep all vertex attribute states in VAO
	glGenVertexArrays(2, g_vao);

	// Define the two drawing states
	glBindVertexArray(g_vao[VAO_VERTEX_0]);
	{
		glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION_0]);
		glVertexAttribPointer(g_attribPosition, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));

		glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_DIRECTION_0]);
		glVertexAttribPointer(g_attribDirection, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));

		glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_COLOR]);
		glVertexAttribPointer(g_attribColor, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));	

		glEnableVertexAttribArray(g_attribPosition);
		glEnableVertexAttribArray(g_attribDirection);
		glEnableVertexAttribArray(g_attribColor);		
	}
	glBindVertexArray(0);

	glBindVertexArray(g_vao[VAO_VERTEX_1]);
	{
		glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION_1]);
		glVertexAttribPointer(g_attribPosition, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));

		glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_DIRECTION_1]);
		glVertexAttribPointer(g_attribDirection, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));

		glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_COLOR]);
		glVertexAttribPointer(g_attribColor, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));	

		glEnableVertexAttribArray(g_attribPosition);
		glEnableVertexAttribArray(g_attribDirection);
		glEnableVertexAttribArray(g_attribColor);		
	}
	glBindVertexArray(0);
}

void drawAndProcessParticles() 
{
	static int s_drawCounter = -1;
	s_drawCounter += 1;

	GLuint drawVao;
	drawVao = VAO_VERTEX_0;
	//TODO: Bind outputs to VBO and create feedback scope
	//TODO: Alternate input and output data

	glBindVertexArray(g_vao[drawVao]);
	glDrawArrays(GL_POINTS, 0, 1<<g_drawVerticesAmountExponent);				
	glBindVertexArray(0);
}

void initShaders() 
{
	const char* srcVert = 
		"#version 150\n"
		"out vec4  vColor;\n"
		"in vec3 aPosition;\n"
		"in vec3 aDirection;\n"
		"in vec3 aColor;\n"
		"uniform mat4 uModelViewMatrix;\n"
		"uniform mat4 uProjectionMatrix;\n"
		"uniform float uSpeed;\n"
		"out vec3 oPosition;\n"
		"out vec3 oDirection;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	vColor = vec4(aColor,1);\n"
		"	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition,1);\n"
		"	oPosition = aPosition + aDirection*uSpeed;\n"
		"	if(length(oPosition) < 2)\n"
		"		oDirection = aDirection;\n"
		"	else\n"
		"	{\n"
		"		vec3 normal = normalize(oPosition);"
		"		oPosition = normal*2;\n"
		"		oDirection = reflect(normalize(aDirection), normal);\n"
		"	}\n"
		"}\n";

	const char* srcFrag = 
		"#version 150\n"
		"in vec4  vColor;\n"
		"out vec4 oColor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	oColor = vColor;\n"
		"}\n";

	g_program = createProgramFast(srcVert, srcFrag);	
	g_attribPosition = glGetAttribLocation(g_program, "aPosition");
	g_attribDirection = glGetAttribLocation(g_program, "aDirection");
	g_attribColor = glGetAttribLocation(g_program, "aColor");
	g_uniformModelViewMatrix = glGetUniformLocation(g_program, "uModelViewMatrix");
	g_uniformProjectionMatrix = glGetUniformLocation(g_program, "uProjectionMatrix");
	g_uniformSpeed = glGetUniformLocation(g_program, "uSpeed");
}


void init() 
{
	g_drawVerticesAmountExponent = MAX_VERTICES_AMOUNT_EXPONENT;
	g_particleMotionEnabled = true;
	g_particleSpeed = 0.001f;

	// Init shaders
	initShaders();

	// Set background color to gray
	glClearColor(0, 0, 0, 0);

	// Init states		
	glPointSize(2);

	// Place camera at (0,0,10)
	g_modelView = glm::translate(g_modelView, glm::vec3(0,0,-10));

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
	glm::vec3 vecY(g_modelView[0][1], g_modelView[1][1], g_modelView[2][1]);
	g_modelView = glm::rotate(g_modelView, (float)-g_rotX, vecY);
	g_rotX = 0;	

	glm::vec3 vecX(g_modelView[0][0], g_modelView[1][0], g_modelView[2][0]);
	g_modelView = glm::rotate(g_modelView, (float)-g_rotY, vecX);
	g_rotY = 0;	
}

void display(void) 
{
	glEnable(GL_DEPTH_TEST);

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		

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
	drawAndProcessParticles();

	// Load camera transformation
	g_modelView = saveMat;

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {

	// Setup viewport
	glViewport(0,0,width,height);

	float w,h;
	w = 4;
	h = 4*((float)height/width);	
	g_projection = glm::ortho(-w/2.0f, w/2.0f, -h/2.0f, h/2.0f, -1.0f, 1000.0f);

	// Create projection transformation	
	glUseProgram(g_program); // Remember to use program when setting variables
	glUniformMatrix4fv(g_uniformProjectionMatrix, 1, false, glm::value_ptr(g_projection));
	glUseProgram(0);
}

void motionFunc(int x, int y) {

	// Calc difference from previous mouse location
	Point prev = g_prevMouse;
	int dx = prev.x - x;
	int dy = prev.y - y;

	// Rotate model
	rotate(dx, dy);

	// Remember mouse location 
	g_prevMouse = Point(x,y);	

	// Don't move particles during mouse move
	g_particleMotionEnabled = false;
}

void mouseFunc(int button, int state, int x, int y) {
	if(button == GLUT_LEFT_BUTTON){
		g_prevMouse = Point(x,y);	
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

	glutCreateWindow("ex21 - Particles Feedback");

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
