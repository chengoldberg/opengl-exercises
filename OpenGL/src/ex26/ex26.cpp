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
#define TES_SHADER_FILENAME "../res/shader/ex26.tes"

#define MESH_FILEPATH "../res/mesh/sphere.off"

enum EVao
{
	VAO_VERTEX,
	VAO_TOTAL
};

enum EBufferObject
{
	VBO_POSITION,
	VBO_COLOR,
	EBO_TRIANGLES,
	BUFFER_OBJECTS_TOTAL
};

bool g_wireframeEnabled;
bool g_zoomMode;
float g_levelsOuter, g_levelsInner;
glm::ivec2 g_prevMouse;
GLuint g_program, g_attribPosition, g_attribColor, g_uniformModelViewMatrix, g_uniformProjectionMatrix;
GLuint g_vbo[BUFFER_OBJECTS_TOTAL];
GLuint g_vao[VAO_TOTAL];
int g_totalElements;
glm::mat4 g_modelView(1), g_projection;
std::vector<glm::vec3> g_vertices;
glm::ivec2 g_cameraRotationXY;
float g_cameraTranslation;

void rotate(double x, double y) 
{
	g_cameraRotationXY.x += x;
	g_cameraRotationXY.y += y;
}

// ============================== Helper Functions =========================

GLuint buildProgram(const char* srcVert, const char* srcTES, const char* srcFrag) 
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

	GLuint tesId = glCreateShader(GL_TESS_EVALUATION_SHADER);
	glShaderSource(tesId, 1, (const GLchar**) &srcTES, NULL);
	glCompileShader(tesId);
	OK = OK && cgl::checkShaderCompilationStatus(tesId);

	if(!OK) 
	{
		throw std::exception("Failed to compile, quitting!\n");
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vsId);
	glAttachShader(program, tesId);
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

void initVertexBufferObjects()
{
	std::vector<glm::vec3> vertices;
	std::vector<glm::ivec3> triangles;
	cgl::loadMeshFromOffFile(MESH_FILEPATH, vertices, triangles);
	
	std::vector<glm::vec3> colors(vertices.size());
	for(int i=0; i<vertices.size(); ++i)
	{
		colors[i] = glm::rgbColor(glm::vec3(360*static_cast<float>(rand())/RAND_MAX,1,1));
	}
	// Create identifiers
	glGenBuffers(BUFFER_OBJECTS_TOTAL, g_vbo);

	// Transfer data
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_COLOR]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[EBO_TRIANGLES]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size()*sizeof(glm::ivec3), triangles.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	g_totalElements = triangles.size()*3;
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
		glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_COLOR]);
		glVertexAttribPointer(g_attribColor, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));
		glEnableVertexAttribArray(g_attribColor);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[EBO_TRIANGLES]);
	}
	glBindVertexArray(0);
}

void drawMesh() 
{
	glBindVertexArray(g_vao[VAO_VERTEX]);
	{
        glPatchParameteri(GL_PATCH_VERTICES, 3);
		float outer[] = {g_levelsOuter, g_levelsOuter, g_levelsOuter};
		float inner[] = {g_levelsInner};
        glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outer);
        glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL , inner);
		glDrawElements(GL_PATCHES, g_totalElements, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
	}
	glBindVertexArray(0);
}

void initShaders() {
	
	const char* srcVert = 
		"#version 330\n"
		"in vec3 aPosition;\n"
		"in vec3 aColor;\n"
		"out vec4 tColor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	gl_Position = vec4(aPosition,1);\n"
		"	tColor = vec4(aColor, 1);\n"
		"}\n";

	const char* srcFrag = 
		"#version 330\n"
		"in vec4 fColor;\n"
		"out vec4 oColor;\n"
		"\n"
		"void main()\n"
		"{\n"
//		"	oColor = vec4(1,1,1,1);\n"
		"	oColor = fColor;\n"
		"}\n";

	std::string srcTES  = cgl::Shader::readFile(TES_SHADER_FILENAME);

	try
	{
		g_program = buildProgram(srcVert, srcTES.c_str(), srcFrag);	
	}
	catch(std::exception ex)
	{
		std::cout << ex.what();
		throw ex;
	}
	g_attribPosition = glGetAttribLocation(g_program, "aPosition");
	g_attribColor = glGetAttribLocation(g_program, "aColor");
	g_uniformModelViewMatrix = glGetUniformLocation(g_program, "uModelViewMatrix");
	g_uniformProjectionMatrix = glGetUniformLocation(g_program, "uProjectionMatrix");
}


void init() 
{	
	glEnable(GL_CULL_FACE);
	g_wireframeEnabled = true;
	g_levelsOuter = 1;
	g_levelsInner = 1;
	g_zoomMode = false;

	// Init shaders
	initShaders();

	// Set background color to gray
	glClearColor(0.25, 0.25, 0.25, 1);

	// Init states		
	glPointSize(2);

	// Init feedback vertex data
	initVertexBufferObjects();

	// Init vertex data
	initVertexArrayObjects();

	// Place camera	
	g_modelView = glm::translate(g_modelView, glm::vec3(0,0,-1.5));
}
 
/**
* Create camera transformation such that the model is rotated around the
* world's X-axis and Y-axis. 
*/
void setupCamera() 
{	
	g_modelView[3] *= 1+g_cameraTranslation;

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

	glPolygonMode(GL_FRONT_AND_BACK, g_wireframeEnabled?GL_LINE:GL_FILL);
	
	// Apply shaders
	glUseProgram(g_program);

	// Create camera transformation
	setupCamera();		
	// Save camera transformation

	//glm::mat4 modelView = glm::translate(g_modelView, 2.0f*glm::vec3(-g_texSize.x/2.0f, -g_texSize.y/2.0f, 0));

	// Update model-view matrix
	glUniformMatrix4fv(g_uniformModelViewMatrix, 1, false, glm::value_ptr(g_modelView));	
	
	// draw Meshes
	drawMesh();

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {

	// Setup viewport
	glViewport(0,0,width,height);

	// Setup projection transformation
	g_projection = glm::ortho(-5.0f,5.0f,-5.0f, 5.0f, -50.0f, 50.0f);	
	//g_projection = glm::ortho(0.0f,100.0f,0.0f, 100.0f, -1.0f, 1.0f);	

	//g_projection = glm::perspective(90.0f, (float)width/height, 0.01f, 10.0f);
	//initFrameBufferObject(width, height);

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
		rotate(dx, dy);
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
	case GLUT_KEY_DOWN: 
		g_levelsOuter -= 1;
		break;
	case GLUT_KEY_UP:
		g_levelsOuter += 1;
		break;
	case GLUT_KEY_LEFT: 
		g_levelsInner -= 1;
		break;
	case GLUT_KEY_RIGHT:
		g_levelsInner += 1;
		break;
	case GLUT_KEY_F1:
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

	glutInitContextVersion(4,0);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_DEBUG);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(512, 512);

	int windowId = glutCreateWindow("ex26 - Icosahedron Tessellation");

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
