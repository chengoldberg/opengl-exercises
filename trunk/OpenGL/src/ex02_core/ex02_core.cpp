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

#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))

struct Point
{
	int x,y;

	Point() {}
	Point(int _x, int _y) : x(_x), y(_y) {}
};

Point g_prevMouse;
double g_rotY;
double g_rotX;
GLuint g_program, g_attribPosition, g_attribColor, g_uniformModelViewMatrix, g_uniformProjectionMatrix;
GLuint g_vbObject, g_ebObject, g_cbObject, g_vao;
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
	glLinkProgram(program);

	//glValidateProgram(program);

	if(checkProgramInfoLog(program) == 0) {
		printf("Failed to create program from shaders");
		return 0;
	} else {
		printf("Program created!");
	}

	return program;
}

const unsigned int cubeVerticesNum = 8;
const unsigned int cubeFacesNum = 2*6;

void loadRGBCube()
{
	float vertices[cubeVerticesNum][3] = {
		{-1,-1,-1}, //0
		{-1,-1,+1}, //1
		{-1,+1,-1}, //2
		{-1,+1,+1}, //3
		{+1,-1,-1}, //4
		{+1,-1,+1}, //5
		{+1,+1,-1}, //6
		{+1,+1,+1}};//7

	float colors[cubeVerticesNum][3] = {
		{0,0,0}, //0
		{0,0,1}, //1
		{0,1,0}, //2
		{0,1,1}, //3
		{1,0,0}, //4
		{1,0,1}, //5
		{1,1,0}, //6
		{1,1,1}};//7
	
	GLuint faces[cubeFacesNum][3] = {
		{0,4,5},{5,1,0},
		{0,1,3},{3,2,0},
		{0,2,6},{6,4,0},
		{7,6,2},{2,3,7},
		{7,5,4},{4,6,7},
		{7,3,1},{1,5,7}};

	// Transfer data to GPU
	glGenBuffers(1, &g_vbObject);
	glBindBuffer(GL_ARRAY_BUFFER, g_vbObject);
	glBufferData(GL_ARRAY_BUFFER, cubeVerticesNum*3*sizeof(float), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &g_cbObject);
	glBindBuffer(GL_ARRAY_BUFFER, g_cbObject);
	glBufferData(GL_ARRAY_BUFFER, cubeVerticesNum*3*sizeof(float), colors, GL_STATIC_DRAW);	

	glGenBuffers(1, &g_ebObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeFacesNum*3*sizeof(unsigned int), faces, GL_STATIC_DRAW);
	
	// Keep all vertex attribute states in VAO
	glGenVertexArrays(1, &g_vao);
	glBindVertexArray(g_vao);

	glBindBuffer(GL_ARRAY_BUFFER, g_vbObject);
	glVertexAttribPointer(g_attribPosition, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));

	glBindBuffer(GL_ARRAY_BUFFER, g_cbObject);
	glVertexAttribPointer(g_attribColor, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));	

	glEnableVertexAttribArray(g_attribPosition);
	glEnableVertexAttribArray(g_attribColor);		

	glBindVertexArray(0);
}

/**
 * Draw a unit size RGB cube
 */
void drawRGBCube() 
{
	// Define the RGB cube on the GPU
	if(g_vao == 0)
		loadRGBCube();

	// Bind and Draw
	glBindVertexArray(g_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebObject);
	glDrawElements(GL_TRIANGLES, cubeFacesNum*3, GL_UNSIGNED_INT, BUFFER_OFFSET(0));				
	glBindVertexArray(0);
}

void initShaders() {
	
	const char* srcVert = 
		"varying vec4  vColor;"
		"attribute vec3 aPosition;"
		"attribute vec3 aColor;"
		"uniform mat4 uModelViewMatrix;"
		"uniform mat4 uProjectionMatrix;"
		""
		"void main()"
		"{"
		"	vColor = vec4(aColor,1);"
		"	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition,1);"
		"}";

	const char* srcFrag = 
		"varying vec4  vColor;"
		""
		"void main()"
		"{"
		"	gl_FragColor = vColor;"
		"}";

	g_program = createProgramFast(srcVert, srcFrag);	
	g_attribPosition = glGetAttribLocation(g_program, "aPosition");
	g_attribColor = glGetAttribLocation(g_program, "aColor");
	g_uniformModelViewMatrix = glGetUniformLocation(g_program, "uModelViewMatrix");
	g_uniformProjectionMatrix = glGetUniformLocation(g_program, "uProjectionMatrix");
}


void init() 
{
	// Init shaders
	initShaders();

	// Set background color to gray
	glClearColor(0.5f, 0.5f, 0.5f, 0);

	// Place camera at (0,0,10)
	g_modelView = glm::translate(g_modelView, glm::vec3(0,0,-10));
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
	//glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		

	// Apply shaders
	glUseProgram(g_program);

	// Create camera transformation
	setupCamera();		

	// Save camera transformation
	glm::mat4 saveMat = g_modelView;

	// Init states		
	//glPolygonMode(GL_BACK, GL_POINT); // Not allowed anymore
	glLineWidth(1); // Line width can't be larger than 1 (!)
	glPointSize(5);

	// Draw Line RGB cube
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);		
	g_modelView = glm::translate(g_modelView, glm::vec3(-3,0,0));
	glUniformMatrix4fv(g_uniformModelViewMatrix, 1, false, glm::value_ptr(g_modelView));
	drawRGBCube();

	// Draw Fill RGB cube
	glPointSize(1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);		
	g_modelView = glm::translate(g_modelView, glm::vec3(3,0,0));
	glUniformMatrix4fv(g_uniformModelViewMatrix, 1, false, glm::value_ptr(g_modelView));
	drawRGBCube();
	glPointSize(5);

	// Draw Point RGB cube
	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	g_modelView = glm::translate(g_modelView, glm::vec3(3,0,0));
	glUniformMatrix4fv(g_uniformModelViewMatrix, 1, false, glm::value_ptr(g_modelView));
	drawRGBCube();

	// Load camera transformation
	g_modelView = saveMat;

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {

	// Setup viewport
	glViewport(0,0,width,height);

	float w,h;
	w = 10;
	h = 10*((float)height/width);	
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
}

void mouseFunc(int button, int state, int x, int y) {
	if(button == GLUT_LEFT_BUTTON){
		g_prevMouse = Point(x,y);	
	}
}

int main(int argc, char **argv) {

	glutInitContextVersion(3,3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_DEBUG);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(500, 500);

	glutCreateWindow("ex2 - Drawing RGB Cube - Core profile");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutMotionFunc(motionFunc);
	glutMouseFunc(mouseFunc);
	//glutInit
	//glutFullScreen();

	// Glew limitation 
	// Ref: http://openglbook.com/glgenvertexarrays-access-violationsegfault-with-glew/
	glewExperimental = GL_TRUE; 
	glewInit();

	init();

	glutMainLoop();

	return 0;  
}
