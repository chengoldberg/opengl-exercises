/*
 * Copyright (C) 2010  Chen Goldberg
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

#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <math.h>

#include "GameLogic.h"
#include "cgl/cl/common.h"
#include "CL/cl_platform.h"

#pragma comment (lib, "glew32.lib")
using namespace std;

#define RAYCAST_PROGRAM_FILENAME "../res/cl_programs/raycast.cl"
#define RAYCAST_KERNEL_NAME "render"

#define RANDOM ((double)rand() / ((double)(RAND_MAX)+(double)(1)))

#define TEXTURE_WIDTH 512
#define TEXTURE_HEIGHT 512
#define CHECKER_SIZE 2

#define IMAGE_SIZE 512

unsigned int	g_frame;
GameLogic		g_game;
GLuint			g_imageTex; 
int				g_height, g_width;

char*			EProcessor[] = {"CPU", "OpenCL"};
bool			g_isOpenCL, g_isAnimate;
GLuint			g_texture;	// Texture Object IDs

float colorDarkGray[] = {0.2f,0.2f,0.2f,0};

// OpenCL global variables
cl_program		g_clProgram;
cl_kernel		g_clKernel;
cl_mem			g_clTexMem, g_clMaterialsMem;
cl_context		g_clContext;
cl_command_queue g_clCommandQueue;

// ============================== CL Program Structures =======================

typedef struct {
   cl_float3 emission;
   cl_float3 ambient;   
   cl_float3 diffuse;
   cl_float3 specular;
   float shininess;
   float __PADDING[3];
} Material;

typedef struct {
   cl_float4 center;
   Material mat; //NOTE: you must arrange fields from big to small, otherwise, if radius was before mat, then access to mat would have been wrong!
   float radius;   
   float __PADDING[3];
} Sphere;

typedef struct {
   cl_float4 pos;
   cl_float4 color;
} Light;

typedef struct {
   cl_float4 pos;   
} Camera;

typedef struct {
   cl_float4 backgroundCol;
   cl_float3 ambientLight;
   Light light;
   Sphere surface;
   Camera camera;
} Scene;

// ============================== OpenCL Procedures =========================

bool renderSceneCL()
{
	cl_int errNum;

	// Acquire the GL objects
	glFinish();

	cl_mem objects[] = {g_clTexMem};
	const int objectsNum = 1;

	errNum = clEnqueueAcquireGLObjects(g_clCommandQueue,objectsNum,objects,0,NULL,NULL);
	if(errNum != CL_SUCCESS)
	{
		std::cerr << "Failed to acquire GL objects" << std::endl;
		return false;
	}

	// Enqueue kernel
	size_t globalWorkSize[] = { 512, 512 };
	size_t* localWorkSize = NULL;

	errNum = clEnqueueNDRangeKernel(g_clCommandQueue, g_clKernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL,NULL);
	if(errNum != CL_SUCCESS)
	{
		std::cerr << "Error executing kernel" << std::endl;
		//return false;
	}

	errNum = clEnqueueReleaseGLObjects(g_clCommandQueue, objectsNum, objects,0,NULL,NULL);	
	clFinish(g_clCommandQueue);
	
	return true;
}

void initOpenCL()
{
	g_clContext = CommonCL::createContext();
	
	cl_device_id device = CommonCL::obtainFirstDeviceFromContext(g_clContext);
	
	g_clCommandQueue = CommonCL::createCommandQueue(g_clContext, device);
	
	cl_program program = CommonCL::createProgram(g_clContext, device, RAYCAST_PROGRAM_FILENAME);

	if(program == NULL)
	{
		exit(-1);
	}

	g_clKernel = CommonCL::createKernel(program, RAYCAST_KERNEL_NAME);

	// Interop VBO to OpenCL mem object
	cl_int errNum;

	g_clTexMem = clCreateFromGLTexture2D(g_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, g_texture, &errNum);
	if(errNum != CL_SUCCESS)
	{
		std::cerr << "Failed to create from GL texture" << std::endl;
	}

	Material material;
	memset(&material, 0, sizeof(material));
	material.ambient.s[0] = 0.1f;material.ambient.s[1] = 0.1f;material.ambient.s[2] = 0.1f;
	material.diffuse.s[0] = 1.0; material.diffuse.s[1] = 0.0; material.diffuse.s[2] = 1.0;
	material.specular.s[0] = 1.0; material.specular.s[1] = 1.0; material.specular.s[2] = 1.0;
	material.shininess = 50.0;

	Scene scene;
	memset(&scene, 0, sizeof(scene));
	scene.surface.center.s[0] = 200; scene.surface.center.s[1] = 200; scene.surface.center.s[2] = -200; scene.surface.center.s[3] = 1.0;
	scene.surface.radius = 99;
	scene.surface.mat = material;
	scene.light.pos.s[0] = 10;scene.light.pos.s[1] = 10;scene.light.pos.s[2] = 10;
	scene.light.color.s[0] = 1;scene.light.color.s[1] = 1; scene.light.color.s[2] = 1;

	g_clMaterialsMem = clCreateBuffer(g_clContext, CL_MEM_COPY_HOST_PTR, sizeof(scene), &scene, &errNum); 
	if(errNum != CL_SUCCESS)
	{
		std::cerr << "Failed to create buffer" << std::endl;
	}

	// Set the kernel arguments
	errNum = clSetKernelArg(g_clKernel, 0, sizeof(cl_mem), &g_clTexMem);	
	errNum = clSetKernelArg(g_clKernel, 1, sizeof(cl_mem), &g_clMaterialsMem);	
	if(errNum != CL_SUCCESS)
	{
		std::cerr << "Error setting Kernel arguments" << std::endl;
		return;
	}
}


// ============================== Drawing Procedures =========================

//
// Create camera transformation that captures the player's POV
//
void setupCamera() 
{
	// Convert player's angle to world angle
	double angle = g_game.getAngle()*180.0/M_PI - 90;

	//glRotated(-25,1,0,0);
	glRotated(angle, 0,1,0);
	glTranslated(-g_game.getPlayerLoc()[0], -2, -g_game.getPlayerLoc()[1]);
}	

void drawScreenAlignedQuad()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();		

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_texture);
	glBegin(GL_QUADS);

	glTexCoord2d(0,0);
	glVertex2d(-1,-1);
	
	glTexCoord2d(0,1);
	glVertex2d(-1,+1);
	
	glTexCoord2d(1,1);
	glVertex2d(+1,+1);
	
	glTexCoord2d(1,0);
	glVertex2d(+1,-1);
	
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,0);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glDisable(GL_BLEND);
}

void renderWorld() 
{
	float l0_amb[] = {0.5f,0.5f,0.5f,1};
	float l0_diff[] = {1,0,1,1};
	float l0_spec[] = {0.05f,0.05f,0.05f,1};

	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, l0_diff);

	glPushMatrix();
	{		
		glTranslated(0,0,-10);	
		GLUquadric* q = gluNewQuadric();
		gluSphere(q,1,10,10);
	}
	glPopMatrix();	
}

void renderScene()
{
	// Create camera transformation
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	setupCamera();	

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);			
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);			

	// Render to texture
	renderSceneCL();

	// Draw world
	renderWorld();
	
	glDisable(GL_DEPTH_TEST);			
	drawScreenAlignedQuad();
}

// ============================== Init Procedures =========================

//
// Puts a CPU generated iamge in texture object g_textures[TEX_GENERATED] 
//
void initTexture() 
{
	glGenTextures(1, &g_texture);

	static GLubyte pixels[TEXTURE_WIDTH][TEXTURE_HEIGHT][4];

	int span = (int) powl(2,CHECKER_SIZE);
	for(int i = 0;i<TEXTURE_WIDTH;++i)		
		for(int j = 0;j<TEXTURE_HEIGHT;++j) {
			bool isBlack = ((i/span)%2==1) ^ ((j/span)%2==1);
			pixels[i][j][0] = isBlack?0:255;
			pixels[i][j][1] = isBlack?0:255;
			pixels[i][j][2] = isBlack?0:255;
			pixels[i][j][3] = isBlack?0:255;	
		}

	glBindTexture(GL_TEXTURE_2D, g_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); //SOLVES bug in NVIDIA drivers!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	glBindTexture(GL_TEXTURE_2D, 0);
}


void init() 
{
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_NORMALIZE);
	glPointSize(1);	

	// Setup sky light		
	float colorBlack[] = {0,0,0,1};
	float colorWhite[] = {1,1,1,1};
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, colorWhite);
	glLightfv(GL_LIGHT0, GL_AMBIENT, colorBlack);
	glLightfv(GL_LIGHT0, GL_SPECULAR, colorWhite);

	g_isOpenCL = true;
	g_isAnimate = false;

	initTexture();
	initOpenCL();
}

// ============================== GLUT Callbacks =========================

void display(void) 
{
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_BLEND);

	// Set background color 
	glClearColor(0.5,0.5,0.5,1);		

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			
		
	renderScene();

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) 
{
	g_width = width; g_height = height;
		
	// Create perspective projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(60, (double)width/height, 0.1, 1000);
	glViewport(0,0,width,height);
}

void timer(int value) 
{
	//glutTimerFunc(16,timer,0);
	glutTimerFunc(0,timer,0);

	// Update game model
	g_game.update();		

	// Advance time counter
	g_frame++;		

	display();
}

void keyboardFunc(int key, int x, int y) {
	switch(key) {
	case GLUT_KEY_DOWN: 
		g_game.setMoveBackward(true);
		break;
	case GLUT_KEY_UP:
		g_game.setMoveForward(true);
		break;
	case GLUT_KEY_LEFT:
		if(glutGetModifiers() == GLUT_ACTIVE_ALT)
			g_game.setStrafeLeft(true);
		else
			g_game.setTurnLeft(true);
		break;
	case GLUT_KEY_RIGHT:
		if(glutGetModifiers() == GLUT_ACTIVE_ALT)
			g_game.setStrafeRight(true);
		else
			g_game.setTurnRight(true);
		break;
	case GLUT_KEY_F1:
		break;
	case GLUT_KEY_F2:
		break;
	case 27:	// Escape key
		exit(0);
		break;
	}
}

void keyboardUpFunc(int key, int x, int y) {
	switch(key) {
	case GLUT_KEY_DOWN: 
		g_game.setMoveBackward(false);
		break;
	case GLUT_KEY_UP:
		g_game.setMoveForward(false);
		break;
	case GLUT_KEY_LEFT:
		g_game.setTurnLeft(false);
		g_game.setStrafeLeft(false);
		break;
	case GLUT_KEY_RIGHT:
		g_game.setTurnRight(false);
		g_game.setStrafeRight(false);
		break;		
	}
}

int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(512, 512);

	glutCreateWindow("ex16 - OpenCL raycasting");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	//glutIdleFunc(display);
	glutTimerFunc(16,timer,0);
	glutSpecialFunc(keyboardFunc);
	glutSpecialUpFunc(keyboardUpFunc);
	
	//glutFullScreen();	

	glewInit();

	init();
	
	printf("\n");
	printf("Implementation Details\n");
	printf("======================\n");
	printf("OpenGL 2.0 Supported: %d\n", GLEW_VERSION_2_0);
	printf("\nUsage\n");
	printf("======================\n");

	if(!GLEW_VERSION_2_0)
		return -1;

	glutMainLoop();

	return 0;  
}

