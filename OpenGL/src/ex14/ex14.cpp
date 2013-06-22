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
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <string>
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cgl/gl/common.hpp"

struct Point
{
	int x,y;

	Point() {}
	Point(int _x, int _y) : x(_x), y(_y) {}
};

struct Uniforms
{
	GLuint modelViewMatrix;
	GLuint projectionMatrix;
} g_unifroms;

Point g_prevMouse;
double g_rotY;
double g_rotX;
GLuint g_attribPosition, g_attribColor, g_uniformModelViewMatrix, g_uniformProjectionMatrix;
GLuint g_vbObject, g_ebObject, g_cbObject, g_vao;
GLuint g_programPipelines[3], g_vertFragProgram;
glm::mat4 g_modelView(1), g_projection;
cgl::SimpleMesh RGBCube;

namespace primitives
{
	enum
	{
		point,
		line,
		triangle,
	};
};

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

void loadRGBCube()
{
	float vertices[][3] = {
		{-1,-1,-1}, //0
		{-1,-1,+1}, //1
		{-1,+1,-1}, //2
		{-1,+1,+1}, //3
		{+1,-1,-1}, //4
		{+1,-1,+1}, //5
		{+1,+1,-1}, //6
		{+1,+1,+1}};//7

	float colors[][3] = {
		{0,0,0}, //0
		{0,0,1}, //1
		{0,1,0}, //2
		{0,1,1}, //3
		{1,0,0}, //4
		{1,0,1}, //5
		{1,1,0}, //6
		{1,1,1}};//7
	
	GLuint faces[] = {
		0,4,5,1,
		0,1,3,2,
		0,2,6,4,
		7,6,2,3,
		7,5,4,6,
		7,3,1,5};

	std::vector<GLuint> facesVec;
	facesVec.assign(faces, faces+sizeof(faces)/sizeof(GLuint));
	RGBCube.init(facesVec);
	RGBCube.
		addAttrib("position", 3, vertices, sizeof(vertices))->
		addAttrib("color", 3, colors, sizeof(colors));
	
	std::vector<GLuint> attribLocs;
	attribLocs.push_back(g_attribPosition);
	attribLocs.push_back(g_attribColor);
	RGBCube.setAttribLocs(attribLocs);

	// Must do it here because using local buffers 
	RGBCube.initBuffers();
}

/**
 * Draw a unit size RGB cube
 */
void drawRGBCube() 
{
	RGBCube.render();
}

void initShaders() 
{
	// Build a separable program that contains the fixed parts (vertex). this
	// should have also contained fragment shader, but for some strange reason
	// it throws a GL error... 
	cgl::Program fixedProgram;
	{
		std::vector<cgl::Shader> shaders;
		shaders.push_back(cgl::Shader::fromFile(GL_VERTEX_SHADER,"ex14.vert"));		
		fixedProgram.build(shaders, true);	
	}

	//
	// Now build the 3 separable programs for each primitive type
	//
	std::string geomText = cgl::Shader::readFile("ex14.geom");
	std::string fragText = cgl::Shader::readFile("ex14.frag");	

	cgl::Program geomTriangleProgram;
	{
		std::vector<cgl::Shader> shaders;
		shaders.push_back(cgl::Shader(GL_FRAGMENT_SHADER, fragText));
		shaders.push_back(cgl::Shader(GL_GEOMETRY_SHADER, geomText).addHeader("#define PRIMITIVE TRIANGLE"));	
		geomTriangleProgram.build(shaders, true);	
	}

	cgl::Program geomLineProgram;
	{
		std::vector<cgl::Shader> shaders;
		shaders.push_back(cgl::Shader(GL_FRAGMENT_SHADER, fragText));
		shaders.push_back(cgl::Shader(GL_GEOMETRY_SHADER, geomText).addHeader("#define PRIMITIVE LINE"));	
		geomLineProgram.build(shaders, true);	
	}

	cgl::Program geomPointProgram;
	{
		std::vector<cgl::Shader> shaders;
		shaders.push_back(cgl::Shader(GL_FRAGMENT_SHADER, fragText));
		shaders.push_back(cgl::Shader(GL_GEOMETRY_SHADER, geomText).addHeader("#define PRIMITIVE POINT"));	
		geomPointProgram.build(shaders, true);	
	}

	//
	// Assemble the separable programs into 3 program pipelines!
	//
	glGenProgramPipelines(3, g_programPipelines);

	glUseProgramStages(g_programPipelines[primitives::triangle], GL_VERTEX_SHADER_BIT, fixedProgram.getId());
	glUseProgramStages(g_programPipelines[primitives::triangle], GL_FRAGMENT_SHADER_BIT | GL_GEOMETRY_SHADER_BIT, geomTriangleProgram.getId());

	glUseProgramStages(g_programPipelines[primitives::line], GL_VERTEX_SHADER_BIT, fixedProgram.getId());
	glUseProgramStages(g_programPipelines[primitives::line], GL_FRAGMENT_SHADER_BIT | GL_GEOMETRY_SHADER_BIT, geomLineProgram.getId());

	glUseProgramStages(g_programPipelines[primitives::point], GL_VERTEX_SHADER_BIT, fixedProgram.getId());
	glUseProgramStages(g_programPipelines[primitives::point], GL_FRAGMENT_SHADER_BIT | GL_GEOMETRY_SHADER_BIT, geomPointProgram.getId());

	//
	// Note: we use the fixed program to set the uniforms we need there.
	//
	g_vertFragProgram = fixedProgram.getId();
	g_attribPosition = glGetAttribLocation(fixedProgram.getId(), "aPosition");
	g_attribColor = glGetAttribLocation(fixedProgram.getId(), "aColor");
	g_uniformModelViewMatrix = glGetUniformLocation(fixedProgram.getId(), "uModelViewMatrix");
	g_uniformProjectionMatrix = glGetUniformLocation(fixedProgram.getId(), "uProjectionMatrix");
}

void initDebug()
{
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	glDebugMessageCallback(&debugOutput, NULL);
}

void init() 
{
	// Init GL Debug
	initDebug();

	// Init shaders
	initShaders();

	// Init meshes
	loadRGBCube();

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
	//glUseProgram(g_program);

	// Create camera transformation
	setupCamera();		

	// Save camera transformation
	glm::mat4 saveMat = g_modelView;

	// Init states		
	//glPolygonMode(GL_BACK, GL_POINT); // Not allowed anymore
	//glLineWidth(1); // Line width can't be larger than 1 (!)
	glPointSize(5);

	// Draw Line RGB cube
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINES_ADJACENCY);		
	RGBCube.setDrawMode(GL_LINES_ADJACENCY);
	g_modelView = glm::translate(g_modelView, glm::vec3(-3,0,0));
	glProgramUniformMatrix4fv(g_vertFragProgram, g_uniformModelViewMatrix, 1, false, glm::value_ptr(g_modelView));
	glBindProgramPipeline(g_programPipelines[primitives::line]);
	drawRGBCube();
	glBindProgramPipeline(0);

	// Draw Fill RGB cube
	
	glPointSize(1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);		
	g_modelView = glm::translate(g_modelView, glm::vec3(3,0,0));
	glProgramUniformMatrix4fv(g_vertFragProgram, g_uniformModelViewMatrix, 1, false, glm::value_ptr(g_modelView));
	glBindProgramPipeline(g_programPipelines[primitives::triangle]);
	drawRGBCube();
	glBindProgramPipeline(0);
	
	glPointSize(5);
	// Draw Point RGB cube
	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	g_modelView = glm::translate(g_modelView, glm::vec3(3,0,0));
	glProgramUniformMatrix4fv(g_vertFragProgram, g_uniformModelViewMatrix, 1, false, glm::value_ptr(g_modelView));
	glBindProgramPipeline(g_programPipelines[primitives::point]);
	drawRGBCube();
	glBindProgramPipeline(0);

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
	glProgramUniformMatrix4fv(g_vertFragProgram, g_uniformProjectionMatrix, 1, false, glm::value_ptr(g_projection));	
}

void motionFunc(int x, int y) 
{
	// Calc difference from previous mouse location
	Point prev = g_prevMouse;
	int dx = prev.x - x;
	int dy = prev.y - y;

	// Rotate model
	rotate(dx, dy);

	// Remember mouse location 
	g_prevMouse = Point(x,y);	
}

void mouseFunc(int button, int state, int x, int y) 
{
	if(button == GLUT_LEFT_BUTTON){
		g_prevMouse = Point(x,y);	
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

int main(int argc, char **argv) 
{
	try
	{
		glutInitContextVersion(4,3);
		glutInitContextProfile(GLUT_CORE_PROFILE);
		glutInitContextFlags(GLUT_DEBUG);

		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
		glutInitWindowPosition(0, 0);
		glutInitWindowSize(512, 512);

		glutCreateWindow("ex14 - Drawing RGB Cube with geometry shaders and Core features");

		glutReshapeFunc(reshape);
		glutDisplayFunc(display);
		glutIdleFunc(display);
		glutMotionFunc(motionFunc);
		glutMouseFunc(mouseFunc);
		glutKeyboardFunc(keyboardFunc);
		//glutInit
		//glutFullScreen();

		// Glew limitation 
		// Ref: http://openglbook.com/glgenvertexarrays-access-violationsegfault-with-glew/
		glewExperimental = GL_TRUE; 
		glewInit();

		init();

		glutMainLoop();
	}
	catch(std::exception ex)
	{
		std::cerr << ex.what() << std::endl;
		exit(-1);
	}
	catch(...)
	{
		exit(-1);
	}

	return 0;  
}
