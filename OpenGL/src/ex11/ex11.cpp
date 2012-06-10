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
#include "CTargaImage.h"
#include "common.h"
#include "Vec3f.h"
#include "cl_common.h"

#pragma comment (lib, "glew32.lib")
using namespace std;

#define TEXTURE_FILENAME_IMAGE "../res/tex_2d/tomb.tga"
#define PARTICLES_PROGRAM_FILENAME "../res/cl_programs/particles.cl"
#define PARTICLES_KERNEL_NAME "animate"

#define RANDOM ((double)rand() / ((double)(RAND_MAX)+(double)(1)))
#define PARTICLES_SIZE 100000

#define TEXTURE_WIDTH 256
#define TEXTURE_HEIGHT 256
#define CHECKER_SIZE 2
#define TEX_GENERATED 2

#define IMAGE_SIZE 256
#define CANVAS_SIZE 10.0
#define GRAVITY 0.0005
#define THRESHOLD_VELOCITY 0.0001
#define BOUNCE_FACTOR 0.33

unsigned int	g_frame;
GameLogic		g_game;
GLuint			g_imageTex; 
int				g_height, g_width;

char*			EProcessor[] = {"CPU", "OpenCL"};
bool			g_isOpenCL, g_isAnimate;

Vec3f			g_particlesPosition[PARTICLES_SIZE];
Vec3f			g_particlesColor[PARTICLES_SIZE];
Vec3f			g_particlesVelocity[PARTICLES_SIZE];
unsigned int	g_particlesWaitTime[PARTICLES_SIZE];
GLuint			g_particlesPositionVBO, g_particlesColorsVBO;

GLuint			g_textures[TEX_GENERATED];	// Texture Object IDs

float colorDarkGray[] = {0.2f,0.2f,0.2f,0};

// OpenCL global variables
cl_program		g_clProgram;
cl_kernel		g_clKernel;
cl_mem			g_clParticlesPositionVBOMem, g_clParticlesColorVBOMem;
cl_mem			g_clParticlesVelocitiesMem, g_clParticlesWaitTimeMem;
cl_mem			g_clFloorTexMem[TEX_GENERATED];
cl_context		g_clContext;
cl_command_queue g_clCommandQueue;

// ============================== Helper Functions =========================

// ============================== Drawing Procedures =========================

void drawParticles() 
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, g_particlesPositionVBO);
	//glVertexPointer(3, GL_FLOAT, 0, g_particlesPosition);
	glVertexPointer(4, GL_FLOAT, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, g_particlesColorsVBO);
	glColorPointer(4, GL_FLOAT, 0, 0);
	//glColorPointer(4, GL_FLOAT, 0, g_particlesColor);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDrawArrays(GL_POINTS, 0, PARTICLES_SIZE);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void drawFloor(const int index)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_textures[index]);
	glBegin(GL_QUADS);

	glTexCoord2d(0,0);
	glVertex3d(-CANVAS_SIZE/2,0,0);
	
	glTexCoord2d(0,1);
	glVertex3d(-CANVAS_SIZE/2,0,+CANVAS_SIZE);
	
	glTexCoord2d(1,1);
	glVertex3d(+CANVAS_SIZE/2,0,+CANVAS_SIZE);
	
	glTexCoord2d(1,0);
	glVertex3d(+CANVAS_SIZE/2,0,0);	
	
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,0);
}

void drawWorld() 
{
	glPushMatrix();
	{
		glTranslated(0,0,0);	
		drawParticles();
	}
	glPopMatrix();
/*
	glPushMatrix();
	{
		glColor3b(0,1,0);
		glTranslated(-10,0,-10);	
		GLUquadric* q = gluNewQuadric();
		gluSphere(q,1,10,10);
	}
	glPopMatrix();

	glPushMatrix();
	{
		glColor3b(0,1,0);
		glTranslated(10,0,-10);	
		GLUquadric* q = gluNewQuadric();
		gluCylinder(q,1,1,10,5,5);
	}
	glPopMatrix();
*/	
	glPushMatrix();
	{
		drawFloor(0);
	}
	glPopMatrix();
	glPushMatrix();
	{
		glRotated(-90,1,0,0);
		drawFloor(1);
	}
	glPopMatrix();
}


// ============================== Animation =========================
#define NAN_HACK 999999999
void animateParticles() 
{
	for(int gid=0;gid<PARTICLES_SIZE;gid++) 
	{	
		// is rounded to zero?
		if(g_particlesVelocity[gid].get(0) == NAN_HACK)
			continue;

		if(g_particlesWaitTime[gid]>g_frame)
			continue;

		// Advance 	
		g_particlesPosition[gid].add(g_particlesVelocity[gid]);	

		// Check collision with surface
		if(g_particlesPosition[gid].get(1)<=0)
		{
			// Reflect
			g_particlesPosition[gid].mul(1,-1,1);
			g_particlesVelocity[gid].mul(1,-1,1);
		
			// Decrease bounce
			g_particlesVelocity[gid].mul(BOUNCE_FACTOR,BOUNCE_FACTOR,BOUNCE_FACTOR);

			// Round to zero?		
			if(g_particlesVelocity[gid].getNorm() < THRESHOLD_VELOCITY)
			{				
				g_particlesVelocity[gid].set(0, NAN_HACK);
				g_particlesPosition[gid].set(1, 0.001);
			}

			// Find image coordinates
			//...
		}	
		if(g_particlesPosition[gid].get(2)<=0)
		{
			// Reflect
			g_particlesPosition[gid].mul(1,1,-1);
			g_particlesVelocity[gid].mul(1,1,-1);
		
			// Decrease bounce
			g_particlesVelocity[gid].mul(BOUNCE_FACTOR,BOUNCE_FACTOR,BOUNCE_FACTOR);

			// Round to zero?		
			if(g_particlesVelocity[gid].getNorm() < THRESHOLD_VELOCITY)
			{				
				g_particlesVelocity[gid].set(0, NAN_HACK);
				g_particlesPosition[gid].set(1, 0.001);
			}

			// Find image coordinates
			//...
		}	
		g_particlesVelocity[gid].add(0,-GRAVITY,0);
	}

	// Update positions VBO
	glBindBuffer(GL_ARRAY_BUFFER, g_particlesPositionVBO);
	glBufferData(GL_ARRAY_BUFFER, PARTICLES_SIZE*sizeof(Vec3f), g_particlesPosition, GL_DYNAMIC_DRAW);		
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ============================== OpenCL Procedures =========================

bool animateParticlesOpenCL()
{
	cl_int errNum;

	// Acquire the GL objects
	glFinish();

	cl_mem objects[] = {g_clParticlesPositionVBOMem,g_clParticlesColorVBOMem,g_clFloorTexMem[0],g_clFloorTexMem[1]};
	const int objectsNum = 4;

	errNum = clEnqueueAcquireGLObjects(g_clCommandQueue,objectsNum,objects,0,NULL,NULL);
	if(errNum != CL_SUCCESS)
	{
		std::cerr << "Failed to acquire GL objects" << std::endl;
		return false;
	}

	errNum = clSetKernelArg(g_clKernel, 6, sizeof(cl_mem), &g_frame);

	// Enqueue kernel
	size_t globalWorkSize[] = { PARTICLES_SIZE };
//	size_t localWorkSize[] = { 4 };
	size_t* localWorkSize = NULL;

	errNum = clEnqueueNDRangeKernel(g_clCommandQueue, g_clKernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL,NULL);
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
	
	cl_program program = CommonCL::createProgram(g_clContext, device, PARTICLES_PROGRAM_FILENAME);

	if(program == NULL)
	{
		exit(-1);
	}

	g_clKernel = CommonCL::createKernel(program, PARTICLES_KERNEL_NAME);

	// Interop VBO to OpenCL mem object
	cl_int errNum;

	g_clParticlesPositionVBOMem = clCreateFromGLBuffer(g_clContext, CL_MEM_READ_WRITE, g_particlesPositionVBO, &errNum);
	if(errNum != CL_SUCCESS)
	{
		std::cerr << "Failed to create MEM from position VBO" << std::endl;
	}

	g_clParticlesColorVBOMem = clCreateFromGLBuffer(g_clContext, CL_MEM_READ_ONLY, g_particlesColorsVBO, &errNum);
	if(errNum != CL_SUCCESS)
	{
		std::cerr << "Failed to create MEM from color VBO" << std::endl;
	}

	for(int i=0;i<2;++i)
	{
		g_clFloorTexMem[i] = clCreateFromGLTexture2D(g_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, g_textures[i], &errNum);
		if(errNum != CL_SUCCESS)
		{
			std::cerr << "Failed to create from GL texture" << std::endl;
		}
	}

	g_clParticlesVelocitiesMem = clCreateBuffer(g_clContext, CL_MEM_COPY_HOST_PTR, PARTICLES_SIZE*sizeof(Vec3f), g_particlesVelocity, &errNum); 
	if(errNum != CL_SUCCESS)
	{
		std::cerr << "Failed to create buffer" << std::endl;
	}

	g_clParticlesWaitTimeMem = clCreateBuffer(g_clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, PARTICLES_SIZE*sizeof(int), g_particlesWaitTime, &errNum); 
	if(errNum != CL_SUCCESS)
	{
		std::cerr << "Failed to create buffer" << std::endl;
	}


	// Set the kernel arguments
	errNum = clSetKernelArg(g_clKernel, 0, sizeof(cl_mem), &g_clParticlesPositionVBOMem);
	errNum = clSetKernelArg(g_clKernel, 1, sizeof(cl_mem), &g_clParticlesVelocitiesMem);	
	errNum = clSetKernelArg(g_clKernel, 2, sizeof(cl_mem), &g_clFloorTexMem[0]);	
	errNum = clSetKernelArg(g_clKernel, 3, sizeof(cl_mem), &g_clFloorTexMem[1]);	
	errNum = clSetKernelArg(g_clKernel, 4, sizeof(cl_mem), &g_clParticlesColorVBOMem);
	errNum = clSetKernelArg(g_clKernel, 5, sizeof(cl_mem), &g_clParticlesWaitTimeMem);
	errNum = clSetKernelArg(g_clKernel, 6, sizeof(cl_mem), &g_frame);
	if(errNum != CL_SUCCESS)
	{
		std::cerr << "Error setting Kernel arguments" << std::endl;
		return;
	}
}

// ============================== Init Procedures =========================

void initParticles() 
{

	CTargaImage img;
	char* filename = TEXTURE_FILENAME_IMAGE;
	if(!img.Load(filename))
	{
		printf("Unable to load %s\n", filename);
		return;
	}	
	const unsigned int iHeight = img.GetHeight();
	const unsigned int iWidth = img.GetWidth();
	const unsigned int bpp = img.GetImageFormat()+3;

	const float vpWidth = 0.8f;
	const float vpHeight = 0.8f;

	Vec3f UPv(0,1,0);
	Vec3f COP(0,3,18);
	Vec3f POI(0,8.0f,0);
	const float Pt = 1.8f;
	Vec3f Fv = Vec3f::subtract(POI,COP);
	Fv.normalize();
	Vec3f P(COP);
	P.mac(Fv,Pt);
	Vec3f Rv = Vec3f::crossProduct(Fv,UPv);
	Rv.normalize();

	Vec3f Uv = Vec3f::crossProduct(Rv,Fv);
	//Vec3f::crossProduct(&Rv,&Fv,&Uv);
	//Vec3f::crossProduct(&Fv,&Rv,&Uv);
	Uv.normalize();

	Vec3f P0(P);
	P0.mac(Rv,-vpWidth/2.0f);
	P0.mac(Uv,vpHeight/2.0f);

	for(int i=0;i<PARTICLES_SIZE;i++) 
	{				
		const float x = (float)RANDOM;
		const float y = (float)RANDOM;
		const float h = (float)RANDOM;

		//g_particlesPosition[i] = Vec3f(x*4-2,15,y*4-2);
		Vec3f pos(P0);
		pos.mac(Rv,x*vpWidth);
		//pos.mac(Uv,-y*vpHeight);
		pos.mac(Uv,-y*vpHeight);
		g_particlesPosition[i] = pos;
				
		const unsigned int ix = (unsigned int)(x*iWidth);
		const unsigned int iy = iHeight-1-(unsigned int)(y*iHeight);
		unsigned char* ptr = img.GetImage()+ ix*bpp + iy*iWidth*bpp;

		g_particlesColor[i] = Vec3f(ptr[0]/255.0f,ptr[1]/255.0f,ptr[2]/255.0f);		

		g_particlesVelocity[i] = Vec3f::subtract(g_particlesPosition[i],COP);
		g_particlesVelocity[i].normalize();

		g_particlesPosition[i].mac(g_particlesVelocity[i],0*h*1.2f+0.001f);

		g_particlesVelocity[i].mul(0.15f,0.15f,0.15f);
		g_particlesVelocity[i].set(3,0);

		g_particlesWaitTime[i] = (unsigned int)(h*1000);
		//g_particlesVelocity[i].mul(0,0,0);
	}

	/*
	for(int i=0;i<PARTICLES_SIZE;i++) 
	{				
		g_particlesPosition[i] = Vec3f((float)(RANDOM-0.5f),10+(float)(RANDOM-0.5f),(float)(RANDOM-0.5f));
		g_particlesColor[i] = Vec3f((float)RANDOM,(float)RANDOM,(float)RANDOM);		
		g_particlesVelocity[i] = Vec3f((float)(RANDOM-0.5f)/15.0f,(float)(RANDOM-0.5f)/15.0f,(float)(RANDOM-0.5f)/15.0f);
		g_particlesVelocity[i].set(3,0);
	}
	*/

	// Create and populate VBOs
	glGenBuffers(1, &g_particlesPositionVBO);	
	glBindBuffer(GL_ARRAY_BUFFER, g_particlesPositionVBO);
	glBufferData(GL_ARRAY_BUFFER, PARTICLES_SIZE*sizeof(Vec3f), g_particlesPosition, GL_DYNAMIC_DRAW);		
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &g_particlesColorsVBO);	
	glBindBuffer(GL_ARRAY_BUFFER, g_particlesColorsVBO);
	glBufferData(GL_ARRAY_BUFFER, PARTICLES_SIZE*sizeof(Vec3f), g_particlesColor, GL_STATIC_DRAW);		
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//
// Puts a CPU generated iamge in texture object g_textures[TEX_GENERATED] 
//
void initTexGenerated(int index) 
{
	glGenTextures(1, g_textures+index);

	static GLubyte pixels[TEXTURE_WIDTH][TEXTURE_HEIGHT][4];

	int span = (int) powl(2,CHECKER_SIZE);
	for(int i = 0;i<TEXTURE_WIDTH;++i)		
		for(int j = 0;j<TEXTURE_HEIGHT;++j) {
			bool isBlack = ((i/span)%2==1) ^ ((j/span)%2==1);
			isBlack = false;
			pixels[i][j][0] = isBlack?0:255;
			pixels[i][j][1] = isBlack?0:255;
			pixels[i][j][2] = isBlack?0:255;
			pixels[i][j][3] = isBlack?0:255;
		}


	glBindTexture(GL_TEXTURE_2D, g_textures[index]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); //SOLVES bug in NVIDIA drivers!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	//gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	glBindTexture(GL_TEXTURE_2D, 0);
}


void init() 
{
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_NORMALIZE);
	glPointSize(1);	

	// Setup sky light		
	float l0_amb[] = {0.5f,0.5f,0.5f,1};
	float l0_diff[] = {1,1,1,1};
	float l0_spec[] = {0.05f,0.05f,0.05f,1};
	
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0_diff);
	glLightfv(GL_LIGHT0, GL_AMBIENT, colorBlack);
	glLightfv(GL_LIGHT0, GL_SPECULAR, colorWhite);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50);

	g_isOpenCL = true;
	g_isAnimate = false;

	//initFBO();
	initParticles();	
	initTexGenerated(0);
	initTexGenerated(1);
	initOpenCL();

	//g_imageTex = loadTexFromFile(TEXTURE_FILENAME_IMAGE);
}

//
// Create camera transformation that captures the player's POV
//
void setupCamera() 
{
	// Convert player's angle to world angle
	double angle = g_game.getAngle()*180.0/M_PI - 90;

	glRotated(angle, 0,1,0);
	glTranslated(-g_game.getPlayerLoc()[0], -2, -g_game.getPlayerLoc()[1]);
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

	// Create camera transformation
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	setupCamera();	

	// Sky light
	//glEnable(GL_LIGHT0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);			
//	glEnable(GL_LIGHTING);
//	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_DEPTH_TEST);			

	// Draw world
	drawWorld();

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

	if(g_isAnimate)
	{
		if(g_isOpenCL)
			animateParticlesOpenCL();
		else
			animateParticles();	

		// Advance time counter
		g_frame++;		
	}
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
		g_isOpenCL = !g_isOpenCL;
		printf("Convolution mode: %s\n", EProcessor[g_isOpenCL]);
		break;
	case GLUT_KEY_F2:
		g_isAnimate = !g_isAnimate;
		printf("Animation: %b\n", g_isAnimate);
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
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE | GLUT_ALPHA);
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(500, 500);

	glutCreateWindow("ex11 - OpenCL interop");

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
	printf("F1 - Switch OpenCL on/off\n");
	printf("F2 - Animation on/off\n");

	if(!GLEW_VERSION_2_0)
		return -1;

	glutMainLoop();

	return 0;  
}

