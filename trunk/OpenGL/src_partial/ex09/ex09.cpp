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
#include "CTargaImage.cpp"
#include "common.h"

#pragma comment (lib, "glew32.lib")
using namespace std;

#define SHADER_PARTICLES_VERT "../res/shader/ParticleWave.vert"
#define SHADER_PARTICLES_FRAG "../res/shader/ParticleWave.frag"
#define SHADER_TOON_VERT "../res/shader/Toon.vert"
#define SHADER_TOON_FRAG "../res/shader/Toon.frag"
#define SHADER_CLOUD_VERT "../res/shader/cloud.vert"
#define SHADER_CLOUD_FRAG "../res/shader/cloud.frag"

#define RANDOM ((double)rand() / ((double)(RAND_MAX)+(double)(1)))

unsigned int	g_frame;
GameLogic		g_game;
GLuint			g_noise3Dtex;
float			g_noiseOffset[3] = {0,0,0};

float colorDarkGray[] = {0.2f,0.2f,0.2f,0};

// Generates a 3D texture that contains 4 octaves of perlin noise
void CreateNoise3D();

// ============================== Helper Functions =========================

string readfile(char *filename) {
	ifstream file(filename);
	file.seekg(0, ios::end);GLuint size = (GLuint) file.tellg();file.seekg(0, ios::beg); 
	string text(size + 1, 0);
	file.read((char*)text.data(), size);
	return text;
}

GLuint loadTexFromFile(char* filename) {

	CTargaImage img;
	GLuint texId;

	glGenTextures(1, &texId);	

	if(!img.Load(filename)){
		printf("Unable to load %s\n", filename);
		return 0;
	}

	glBindTexture(GL_TEXTURE_2D, texId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if(img.GetImageFormat() == 1)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.GetWidth(), img.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.GetImage());	
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.GetWidth(), img.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, img.GetImage());	

	img.Release();						

	return texId;
}

/*
 * Compiles shader prints errors and returns status
 */
bool myCompileShader(GLuint shader) {
	GLint compiled;

	glCompileShader(shader);

	glGetObjectParameterivARB(shader, GL_COMPILE_STATUS, &compiled);
	GLint blen = 0;	GLsizei slen = 0;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &blen);       

	if (blen > 1)
	{
	 GLchar* compiler_log = (GLchar*)malloc(blen);
	 glGetInfoLogARB(shader, blen, &slen, compiler_log);
	 cout << "compiler_log:\n" << compiler_log << "\n";
	 free (compiler_log);
	}

	return compiled == 1;
}

/*
 * Prints program's errors and returns status
 */
int checkProgramInfoLog(GLuint prog) {
	int len = 0, read = 0;
	string log;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
	if (len > 1)
	{
		log.resize(len);
		glGetProgramInfoLog(prog, len, &read, (GLchar*) log.data());
		printf("Program Info Log:\n%s\n", log.c_str());
	}
	int ret;
	glGetProgramiv(prog, GL_LINK_STATUS, &ret);
	return ret;
}

// ============================== Drawing Procedures =========================

void drawObjects() {

	GLUquadric* q = gluNewQuadric();

	// Draw sphere
	float gs_ambdiff[] = {1,1,0,0.5};
	float gs_emission[] = {0.5,0.5,0,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gs_ambdiff);	
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);		
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);

	glPushMatrix();
	{
		glTranslated(0, 0.75+sin(g_frame*0.075)*0.25, -3);
		glutSolidCube(1);
	}
	glPopMatrix();

	// Draw teapot
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorWhite);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		
	glPushMatrix();
	{
		glTranslated(0, 1, 5);
		glutSolidTeapotFIX(2);
	}
	glPopMatrix();		

	// Draw rotating cylinder
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorWhite);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		
	glPushMatrix();
	{
		glTranslated(5, 1, -3);
		glRotated(g_frame, 0, 1, 0);
		glTranslated(0, 0, -1);
		gluCylinder(q, 1, 1, 2, 40, 40);
	}
	glPopMatrix();
}

void drawFloor() {

	static float ambdiff[] = {0.65f,0.65f,0.65f,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambdiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		

	glPushMatrix();
	{
		glTranslated(-g_game.getBoardWidth()/2.0, 0,-g_game.getBoardHeight()/2.0);
		glBegin(GL_QUADS);		
		glNormal3d(0, 1, 0);
		for(int i=0;i<g_game.getBoardWidth();++i)
			for(int j=0;j<g_game.getBoardHeight();++j) {							
				glTexCoord2d(0,0);
				glVertex3d(0+i, 0, 0+j);
				glTexCoord2d(0,1);
				glVertex3d(0+i, 0, 1+j);
				glTexCoord2d(1,1);
				glVertex3d(1+i, 0, 1+j);
				glTexCoord2d(1,0);
				glVertex3d(1+i, 0, 0+j);			
			}
		glEnd();	
	}
	glPopMatrix();
}	

void drawParticles() {
	//TODO:
}

void drawWorld() {
	static float l0_pos[] = {-10,10,10,1};
	glLightfv(GL_LIGHT0, GL_POSITION, l0_pos);

	drawFloor();
	drawObjects();	
	glPushMatrix();
	{
		glTranslated(0,1.5,0);	
		drawParticles();
	}
	glPopMatrix();
}

// ============================== Init Procedures =========================

void initNoise3DTexture() {
    glEnable(GL_TEXTURE_3D);

	glGenTextures(1, &g_noise3Dtex);
	glBindTexture(GL_TEXTURE_3D, g_noise3Dtex);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	CreateNoise3D();

	glDisable(GL_TEXTURE_3D);
}

void init() {

	glEnable(GL_NORMALIZE);

	// Set background color 
	glClearColor(0.5,0.5,0.5,0.5);		

	// Setup sky light		
	float l0_ambdiff[] = {0.5f,0.5f,0.5f,1};
	float l0_spec[] = {0.05f,0.05f,0.05f,1};
	
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0_ambdiff);
	glLightfv(GL_LIGHT0, GL_AMBIENT, colorBlack);
	glLightfv(GL_LIGHT0, GL_SPECULAR, colorBlack);

	initNoise3DTexture();
}

//
// Create camera transformation that captures the player's POV
//
void setupCamera() {

	// Convert player's angle to world angle
	double angle = g_game.getAngle()*180.0/M_PI - 90;

	glRotated(angle, 0,1,0);
	glTranslated(-g_game.getPlayerLoc()[0], -1, -g_game.getPlayerLoc()[1]);
}	

// ============================== GLUT Callbacks =========================

void display(void) {

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			

	// Create camera transformation
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	setupCamera();	

	// Sky light
	glEnable(GL_LIGHT0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);			
	glEnable(GL_LIGHTING);
	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_DEPTH_TEST);		

	// Draw world
	drawWorld();
	
	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {
	// Create perspective projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(60, (double)width/height, 0.1, 1000);
	glViewport(0,0,width,height);
}

void timer(int value) {

	glutTimerFunc(16,timer,0);

	g_noiseOffset[1]+=0.002f;
	g_noiseOffset[2]+=0.0002f;
	g_noiseOffset[0]+=0.0002f;
	
	// Update game model
	g_game.update();		

	// Advance time counter
	g_frame++;		
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
	case GLUT_KEY_F2:
		static bool b = false;
		if(b == false) 
			glEnable(GL_MULTISAMPLE);
		else 
			glDisable(GL_MULTISAMPLE);
		b ^= true;
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

void keyboardFunc(unsigned char key, int x, int y) {
	switch(key) {
	case 27:	// Quit on 'Escape' key
		exit(0);
	}
}

int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE | GLUT_ALPHA);
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH)-512)/2, (glutGet(GLUT_SCREEN_HEIGHT)-512)/2);
	glutInitWindowSize(512, 512);

	glutCreateWindow("ex09 - Using GLSL Shaders");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutTimerFunc(16,timer,0);
	glutSpecialFunc(keyboardFunc);
	glutSpecialUpFunc(keyboardUpFunc);
	glutKeyboardFunc(keyboardFunc);

	//glutFullScreen();

	glewInit();

	init();
	
	printf("\n");
	printf("Implementation Details\n");
	printf("======================\n");
	printf("OpenGL 2.0 Supported: %d\n", GLEW_VERSION_2_0);

	if(!GLEW_VERSION_2_0)
		return -1;

	glutMainLoop();

	return 0;  
}

