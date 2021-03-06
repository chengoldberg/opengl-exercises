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

#define  _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <math.h>
#include "GameLogic.h"
#include "CTargaImage.h"
#include "CTargaImage.cpp"
#include "common.h"

#pragma comment (lib, "glew32.lib")

#define RANDOM ((double)rand() / ((double)(RAND_MAX)+(double)(1)))

#define TEXTURE_FILENAME_BAR "../res/tex_2d/doombar.tga"
#define TEXTURE_FILENAME_TREE "../res/tex_2d/doomtree.tga"
#define TEXTURE_FILENAME_MARKER "../res/tex_2d/bang.tga"
#define TEXTURE_FILENAME_FLOOR "../res/tex_2d/doomfloor.tga"

#define TEXTURE_MARKER_SIZE 16
#define TEXTURE_BAR_WIDTH 320
#define TEXTURE_BAR_HEIGHT 32

#define FLOOR_WIDTH 10
#define FLOOR_HEIGHT 10
#define POINT_SPRITES_MAX 100


int g_width,	g_height;
unsigned int	g_frame;
GameLogic		g_game;
float			g_snowPos[POINT_SPRITES_MAX][3];
GLuint			g_treeTex, g_markerTex, g_floorTex, g_hudTex, g_lampTex;

float colorDarkGray[] = {0.2f,0.2f,0.2f,0};

// ============================== Helper Functions =========================

void drawString(char* txt) {
	while(*txt != '\0') {
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *txt);
		txt++;		
	}
}

void getStringSize(char* txt, int &w, int &h) {
	w=0;h=0;
	while(*txt != '\0') {
		w += glutBitmapWidth(GLUT_BITMAP_TIMES_ROMAN_24, *txt);
		int temp = glutBitmapWidth(GLUT_BITMAP_TIMES_ROMAN_24, *txt);
		if(temp>h)
			h = temp;
		txt++;		
	}
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

void billboardCheatSphericalBegin() {
	
	float modelview[16];
	int i,j;

	// save the current modelview matrix
	glPushMatrix();

	// get the current modelview matrix
	glGetFloatv(GL_MODELVIEW_MATRIX , modelview);

	// undo all rotations
	// beware all scaling is lost as well 
	for( i=0; i<3; i++ ) 
	    for( j=0; j<3; j++ ) {
		if ( i==j )
		    modelview[i*4+j] = 1.0;
		else
		    modelview[i*4+j] = 0.0;
	    }

	// set the modelview with no rotations
	glLoadMatrixf(modelview);
}

void billboardEnd() {

	// restore the previously 
	// stored modelview matrix
	glPopMatrix();
}

// ============================== Drawing Procedures =========================

void drawHUD() {	

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();	// Push Projection matrix!	
	glLoadIdentity();

	glOrtho(0,g_width,+g_height,0,-1000,1000);	

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	char temp[100];

	sprintf(temp, "Frame: %.2d", g_frame);
	int w,h;
	getStringSize(temp,w,h);

	glColor3d(1,0,0);

	glRasterPos2i(0,h);
	drawString(temp);	
	
	glRasterPos2i(0, g_height);
	drawString(temp);	

	glRasterPos2i(g_width-w, g_height);
	drawString(temp);	

	glRasterPos2i(g_width-w, h);
	drawString(temp);	

	w = TEXTURE_BAR_WIDTH;
	h = TEXTURE_BAR_HEIGHT;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_hudTex);
	glBegin(GL_QUADS);
	glTexCoord2d(0,1);glVertex2d(g_width/2-w/2,g_height-h);
	glTexCoord2d(0,0);glVertex2d(g_width/2-w/2,g_height);
	glTexCoord2d(1,0);glVertex2d(g_width/2+w/2,g_height);
	glTexCoord2d(1,1);glVertex2d(g_width/2+w/2,g_height-h);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();	// Pop modelview matrix!

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();	// Pop Projection matrix!
	
	glMatrixMode(GL_MODELVIEW);
}

void drawObjects() { 

	GLUquadric* q = gluNewQuadric();	

	// Draw Cube
	float gs_ambdiff[] = {1,1,0,0.5};
	float gs_emission[] = {0.5,0.5,0,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorWhite);	
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);		
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);

	glPushMatrix();
	{
		glTranslated(0, 0.75+sin(g_frame*0.075)*0.25, -5);
		glutSolidCube(1);
		// Note RasterPos makes T&L calculations - so set color before
		glDisable(GL_LIGHTING);	glColor3d(0,1,0);
		glRasterPos3d(0,1,0); drawString("Cube");
		glEnable(GL_LIGHTING);
	}
	glPopMatrix();

	// Draw Big sphere		
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorWhite);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		
	glPushMatrix();
	{
		glTranslated(0, 1, 5);
		double scale = 0.75+sin(g_frame*0.075)*0.25;
		glScaled(scale,scale,scale);
		gluSphere(q, 0.75, 32, 32);
		
		glDisable(GL_LIGHTING);	glColor3d(0,1,0);
		glRasterPos3d(0,1,0); drawString("Shpere");
		glEnable(GL_LIGHTING);
	}
	glPopMatrix();		

	// Draw rotating cylinder
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorWhite);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		
	glPushMatrix();
	{
		glTranslated(0, 1, 0);
		
		glDisable(GL_LIGHTING);	glColor3d(0,1,0);
		glRasterPos3d(0,0,0); drawString("Cylinder"); 
		glEnable(GL_LIGHTING);

		glRotated(g_frame, 0, 1, 0);
		glTranslated(0, 0, -1);
		gluCylinder(q, 1, 1, 2, 40, 40);
	}
	glPopMatrix();
}

void drawFloor() {

	glEnable(GL_TEXTURE_2D);	
	glBindTexture(GL_TEXTURE_2D, g_floorTex);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	static float ambdiff[] = {0.9f,0.9f,0.9f,1};
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

	glDisable(GL_TEXTURE_2D);
}	

void drawPointSprites() {

	glBindTexture(GL_TEXTURE_2D, g_markerTex);

	glDisable(GL_LIGHTING);
	glColor3f(1,0,0);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_POINT_SPRITE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

	glBegin(GL_POINTS);
	for(int i=0;i<POINT_SPRITES_MAX;++i) {
		glVertex3fv(g_snowPos[i]);
	}
	glEnd();
	
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
}

void drawTree() {

	glBindTexture(GL_TEXTURE_2D, g_treeTex);

	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
	glPushMatrix();
	{
		glBegin(GL_QUADS);
		glTexCoord2d(0,0);
		glVertex3d(-1,0,0);
		glTexCoord2d(1,0);
		glVertex3d(1,0,0);
		glTexCoord2d(1,1);
		glVertex3d(1,2,0);
		glTexCoord2d(0,1);
		glVertex3d(-1,2,0);
		glEnd();
	}
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}

void drawWorld() {
	
	drawFloor();
	drawObjects();
	
	// Draw Sprites
	glAlphaFunc(GL_GREATER,0);
	glEnable(GL_ALPHA_TEST);
	
	glPushMatrix();
	glTranslated(3,0,3);
	billboardCheatSphericalBegin();
    drawTree();
	billboardEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslated(3,0,-3);
	billboardCheatSphericalBegin();
    drawTree();
	billboardEnd();
	glPopMatrix();
		
	// Draw snow	
	glPointSize(TEXTURE_MARKER_SIZE);
	drawPointSprites();

	glDisable(GL_ALPHA_TEST);
}

void init() {

	//float temp[] = {1,0.1,0};
	//glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, temp);
	glPointParameterf(GL_POINT_SPRITE_COORD_ORIGIN,GL_LOWER_LEFT);

	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_NORMALIZE);

	// Set background color to black
	glClearColor(44.0f/255, 72.0f/255, 114.0f/255, 0);		

	// Setup sky light		
	float l0_ambdiff[] = {0.5f,0.5f,0.5f,1};
	float l0_spec[] = {0.05f,0.05f,0.05f,1};
	
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0_ambdiff);
	glLightfv(GL_LIGHT0, GL_AMBIENT, colorBlack);
	glLightfv(GL_LIGHT0, GL_SPECULAR, colorBlack);			
	glEnable(GL_LIGHT0);

	for(int i=0;i<POINT_SPRITES_MAX;++i) {
		g_snowPos[i][0] = (float) RANDOM*g_game.getBoardWidth()-g_game.getBoardWidth()/2;
		g_snowPos[i][1] = (float) RANDOM*5;
		g_snowPos[i][2] = (float) RANDOM*g_game.getBoardHeight()-g_game.getBoardHeight()/2;
	}

	g_floorTex = loadTexFromFile(TEXTURE_FILENAME_FLOOR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	g_markerTex = loadTexFromFile(TEXTURE_FILENAME_MARKER);
	g_treeTex = loadTexFromFile(TEXTURE_FILENAME_TREE);

	g_hudTex = loadTexFromFile(TEXTURE_FILENAME_BAR);
}

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

	static float l0_pos[] = {-10,10,10,1};
	glLightfv(GL_LIGHT0, GL_POSITION, l0_pos);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);			
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);	

	// Draw world
	drawWorld();
	
	// Draw HUD
	drawHUD();

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {
	// Create perspective projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(60, (double)width/height, 0.1, 1000);
	glViewport(0,0,width,height);

	g_width = width;
	g_height = height;
}

void timer(int value) {
	
	glutTimerFunc(16,timer,0);

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

int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE | GLUT_ALPHA);
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(320, 200);

	glutCreateWindow("ex08 - Sprites");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutTimerFunc(16,timer,0);
	glutSpecialFunc(keyboardFunc);
	glutSpecialUpFunc(keyboardUpFunc);
	
	//glutFullScreen();
	glewInit();

	init();
	
	GLfloat fSizes[2];
	glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, fSizes);

	printf("\n");
	printf("Implementation Details\n");
	printf("======================\n");
	printf("Point sprite extension supported: %d\n", GLEW_ARB_point_sprite);
	printf("Point aliasing size, min: %f, max: %f\n", fSizes[0], fSizes[1]);

	glutMainLoop();

	return 0;  
}

