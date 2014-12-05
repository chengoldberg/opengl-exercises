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

#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))

#define MESH_FILEPATH "../res/mesh/sphere.off"
#define TEX_FILEPATH "../res/tex_2d/tomb.tga"

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

bool g_wireframeEnabled;
bool g_blendEnabled;
bool g_zoomMode;
glm::ivec2 g_prevMouse;
GLuint g_program, g_attribPosition, g_uniformModelViewMatrix, g_uniformProjectionMatrix, g_uniformTexture;
GLuint g_vbo[BUFFER_OBJECTS_TOTAL];
GLuint g_vao[VAO_TOTAL];
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

void initVertexBufferObjects()
{
	std::vector<glm::vec3> vertices;
	std::vector<glm::ivec3> triangles;
	cgl::loadMeshFromOffFile(MESH_FILEPATH, vertices, triangles);

	// Create identifiers
	glGenBuffers(BUFFER_OBJECTS_TOTAL, g_vbo);

	// Transfer data
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION]);
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
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[EBO_TRIANGLES]);
	}
	glBindVertexArray(0);
}

void drawMesh() 
{
	glBindVertexArray(g_vao[VAO_VERTEX]);
	{
#ifdef PARTIAL
		//TODO: Make more efficient using instanced rendering
		for(int i=0; i<g_texSize.x*g_texSize.y;++i)
		{
			static GLint uniformInstanceId = glGetUniformLocation(g_program, "uInstanceId");
			glUniform1i(uniformInstanceId, i);
			glDrawElements(GL_TRIANGLES, g_totalElements, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
		}
#else
		glDrawElementsInstanced(GL_TRIANGLES, g_totalElements, GL_UNSIGNED_INT, BUFFER_OFFSET(0), g_texSize.x*g_texSize.y);
#endif
	}
	glBindVertexArray(0);
}

void initTexFromFile() 
{
	CTargaImage img;

	char *filename = TEX_FILEPATH;

	if(!img.Load(filename))
	{
		printf("Unable to load %s\n", filename);
		return;
	}

	glGenTextures(TEXTURE_OBJECTS_TOTAL, g_textures);
	glBindTexture(GL_TEXTURE_2D, g_textures[TEX_FROM_FILE]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	g_texSize.x = img.GetWidth();
	g_texSize.y = img.GetHeight();

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.GetWidth(), img.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, img.GetImage());		

	img.Release();						
}

void initShaders() {
#ifdef PARTIAL		
	//TODO: Make more efficient using instanced rendering
#endif
	const char* srcVert = 
#ifdef PARTIAL		
		"#version 150\n"
		"in vec3 aPosition;\n"
		"uniform mat4 uModelViewMatrix;\n"
		"uniform mat4 uProjectionMatrix;\n"
		"uniform sampler2D uTexture;\n"
		"uniform int uInstanceId;\n"
		"out vec4 fColor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	ivec2 texSize = textureSize(uTexture, 0);\n"
		"	ivec2 coord = ivec2(mod(uInstanceId,texSize.x), uInstanceId/texSize.x);\n"
		"	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition+vec3(coord.x*2,coord.y*2,0),1);\n"
		"	// For some reason ATI won't make uTexture active if this line is removed!;\n"
		"	fColor = vec4(texture(uTexture, vec2(0,0)).xyz, 1)*0;\n"
		"	fColor = vec4(texelFetch(uTexture, coord, 0).xyz, 1);\n"
		"}\n";
#else
		"#version 150\n"
		"uniform sampler2D uTexture;\n"
		"in vec3 aPosition;\n"
		"uniform mat4 uModelViewMatrix;\n"
		"uniform mat4 uProjectionMatrix;\n"
		"out vec4 fColor;\n"
		"\n"
		"void main()\n"
		"{\n"

		"	ivec2 texSize = textureSize(uTexture, 0);\n"
		"	ivec2 coord = ivec2(gl_InstanceID%texSize.x, gl_InstanceID/texSize.x);\n"
		"	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition+vec3(coord.x*2,coord.y*2,0),1);\n"
		"	// For some reason ATI won't make uTexture active if this line is removed!;\n"
		"	fColor = vec4(texture(uTexture, vec2(0,0)).xyz, 1)*0;\n"
		"	fColor = vec4(texelFetch(uTexture, coord, 0).xyz, 1);\n"

		"}\n";
#endif

	const char* srcFrag = 
		"#version 150\n"
		"in vec4 fColor;\n"
		"out vec4 oColor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	oColor = fColor;\n"
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
	g_uniformTexture = glGetUniformLocation(g_program, "uTexture");
}


void init() 
{	
	g_wireframeEnabled = true;
	g_blendEnabled = false;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	g_zoomMode = false;

	// Init shaders
	initShaders();

	// Set background color to gray
	glClearColor(0.25, 0.25, 0.25, 1);

	// Init feedback vertex data
	initVertexBufferObjects();

	// Init vertex data
	initVertexArrayObjects();

	initTexFromFile();

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

	glPolygonMode(GL_FRONT_AND_BACK, g_wireframeEnabled?GL_LINE:GL_FILL);
	
	// Apply shaders
	glUseProgram(g_program);

	// Create camera transformation
	setupCamera();		
	// Save camera transformation
	
	// Update texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textures[TEX_FROM_FILE]);
	glUniform1i(g_uniformTexture, 0);

	
	glm::mat4 modelView = glm::translate(g_modelView, 2.0f*glm::vec3(-g_texSize.x/2.0f, -g_texSize.y/2.0f, 0));

	// Update model-view matrix
	glUniformMatrix4fv(g_uniformModelViewMatrix, 1, false, glm::value_ptr(modelView));	
	
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
	drawMesh();
	glEndQuery(GL_TIME_ELAPSED);
	
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

	int windowId = glutCreateWindow("ex25 - Instanced Rendering Grid");

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
