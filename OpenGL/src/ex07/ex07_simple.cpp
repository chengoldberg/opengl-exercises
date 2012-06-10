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
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#include "GameLogic.h"

#define MIRROR_WIDTH 3
#define MIRROR_HEIGHT 4

unsigned int frame;
GameLogic game;

float colorBlack[] = {0,0,0,0};
float colorDarkGray[] = {0.2f,0.2f,0.2f,0};
float colorWhite[] = {1,1,1,1};

// Fixes bug where teapot assumed CCW is default
void glutSolidTeapotFIX(double size) {
	int frontFace;
	glGetIntegerv(GL_FRONT_FACE, &frontFace);
	if(frontFace == GL_CCW)
		glFrontFace(GL_CW);
	else
		glFrontFace(GL_CCW);

	glutSolidTeapot(size);
	if(frontFace == GL_CCW)
		glFrontFace(GL_CCW);
	else
		glFrontFace(GL_CW);
}

void drawObjects() {

	GLUquadric* q = gluNewQuadric();

	// Draw sphere
	float gs_ambdiff[] = {1,1,0,1};
	float gs_emission[] = {0.5,0.5,0,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gs_ambdiff);	
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);		
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);

	glPushMatrix();
	{
		glTranslated(0, 0.75+sin(frame*0.075)*0.25, -3);
		gluSphere(q, 0.5, 8, 8);
	}
	glPopMatrix();

	// Draw Big sphere		
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorWhite);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		
	glPushMatrix();
	{
		glTranslated(0, 1, 5);
		gluSphere(q, 0.75, 32, 32);
	}
	glPopMatrix();		

	// Draw rotating cylinder
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorWhite);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		
	glPushMatrix();
	{
		glTranslated(5, 1, -3);
		glRotated(frame, 0, 1, 0);
		glTranslated(0, 0, -1);
		gluCylinder(q, 1, 1, 2, 40, 40);
	}
	glPopMatrix();

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glPushMatrix();
	{
		glScaled(BOARD_WIDTH,BOARD_WIDTH,BOARD_HEIGHT);
		glRotated(45,0,1,0);
		glutWireSphere(1,10,10);
	}
	glPopMatrix();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void drawFloor() {

	float ambdiff[] = {0.65f,0.65f,0.65f,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambdiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		

	glPushMatrix();
	{
		glTranslated(-game.getBoardWidth()/2.0, 0,-game.getBoardHeight()/2.0);
		glBegin(GL_QUADS);		
		glNormal3d(0, 1, 0);
		for(int i=0;i<game.getBoardWidth();++i)
			for(int j=0;j<game.getBoardHeight();++j) {							
				glVertex3d(0+i, 0, 0+j);
				glVertex3d(0+i, 0, 1+j);
				glVertex3d(1+i, 0, 1+j);
				glVertex3d(1+i, 0, 0+j);			
			}
		glEnd();	
	}
	glPopMatrix();
}	

void drawMirror() {
	float ambdiff[] = {0.65f,0.05f,0.05f,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambdiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		

	glBegin(GL_QUADS);		
	glNormal3d(0, 0, 1);
	glVertex3d(-MIRROR_WIDTH/2.0, 0, 0);
	glVertex3d(MIRROR_WIDTH/2.0, 0, 0);
	glVertex3d(MIRROR_WIDTH/2.0, MIRROR_HEIGHT, 0);
	glVertex3d(-MIRROR_WIDTH/2.0, MIRROR_HEIGHT, 0);			
	glEnd();
}

void drawMirrorBorder() {
	float ambdiff[] = {0.0f,0.05f,0.5f,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambdiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		

	GLUquadric *q = gluNewQuadric();
	glPushMatrix();
	{
		glTranslated(-MIRROR_WIDTH/2.0,0,0);
		glRotated(-90,1,0,0);
		gluCylinder(q,0.2,0.2,MIRROR_HEIGHT,10,3);
	}
	glPopMatrix();
	glPushMatrix();
	{
		glTranslated(+MIRROR_WIDTH/2.0,0,0);
		glRotated(-90,1,0,0);
		gluCylinder(q,0.2,0.2,MIRROR_HEIGHT,10,3);
	}
	glPopMatrix();

}

void init() {

	// Set background color to black
	glClearColor(0, 0, 0, 0);		

	// Setup sky light		
	float l0_ambdiff[] = {0.5f,0.5f,0.5f,1};
	float l0_spec[] = {0.05f,0.05f,0.05f,1};
	
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0_ambdiff);
	glLightfv(GL_LIGHT0, GL_AMBIENT, colorBlack);
	glLightfv(GL_LIGHT0, GL_SPECULAR, colorBlack);			
}

void drawActor() {

	float ambdiff[] = {0.5f,0.05f,0.0f,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambdiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		

	double angle = game.getAngle()*180.0/M_PI - 180;
	
	glPushMatrix();
	{	
		glTranslated(game.getPlayerLoc()[0], 1, game.getPlayerLoc()[1]);
		glRotated(-angle, 0,1,0);

		glutSolidTeapotFIX(0.5);
	}
	glPopMatrix();	
}

/**
* Create camera transformation that captures the player's POV
* @param gl OpenGL context
*/
void setupCamera() {

	// Convert player's angle to world angle
	double angle = game.getAngle()*180.0/M_PI - 90;

	glRotated(angle, 0,1,0);
	glTranslated(-game.getPlayerLoc()[0], -1, -game.getPlayerLoc()[1]);
}	

void drawWorld() {
	float l0_pos[] = {-10,10,10,1};
	glLightfv(GL_LIGHT0, GL_POSITION, l0_pos);

	
	drawMirrorBorder();
	drawFloor();
	drawObjects();
}


void display(void) {

	glEnable(GL_NORMALIZE);

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);			

	glEnable(GL_LIGHTING);

	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	glEnable(GL_DEPTH_TEST);	

	// Create camera transformation
	setupCamera();	

	// Sky light
	glEnable(GL_LIGHT0);

	//float l0_pos[] = {0.5,1,1,1};

	// Draw world
	drawWorld();

	glEnable(GL_STENCIL_TEST);

	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
	glStencilFunc(GL_ALWAYS, 0x1, 0x1);	
	glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
	
	//glDepthMask(GL_FALSE);
	//glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
	
	glDisable(GL_LIGHTING);
	glColor3d(0,0,0);
	drawMirror();
	glEnable(GL_LIGHTING);
	
	glClear(GL_DEPTH_BUFFER_BIT);

	glStencilFunc(GL_EQUAL, 0x1, 0x1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glEnable(GL_CLIP_PLANE0);
	double sign = game.loc[1]>0?1:-1;
	double clipPlane[] = {0,0,-sign,0};
	glClipPlane(GL_CLIP_PLANE0, clipPlane);

	glFrontFace(GL_CW);
	glPushMatrix();
	{
		glScaled(1,1,-1);
		
		drawWorld();
		drawActor();		
	}
	glPopMatrix();
	glFrontFace(GL_CCW);	

	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_STENCIL_TEST);	

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

#define UPDATE_INTERVAL_MS 16
void timer(int value) {
	// Advance time counter
	frame++;		

	// Update game model
	game.update();	

	glutTimerFunc(UPDATE_INTERVAL_MS,timer,0);
}

void keyboardFunc(int key, int x, int y) {
	switch(key) {
	case GLUT_KEY_DOWN: 
		game.setMoveBackward(true);
		break;
	case GLUT_KEY_UP:
		game.setMoveForward(true);
		break;
	case GLUT_KEY_LEFT:
		if(glutGetModifiers() == GLUT_ACTIVE_ALT)
			game.setStrafeLeft(true);
		else
			game.setTurnLeft(true);
		break;
	case GLUT_KEY_RIGHT:
		if(glutGetModifiers() == GLUT_ACTIVE_ALT)
			game.setStrafeRight(true);
		else
			game.setTurnRight(true);
		break;
	}
}

void keyboardUpFunc(int key, int x, int y) {
	switch(key) {
	case GLUT_KEY_DOWN: 
		game.setMoveBackward(false);
		break;
	case GLUT_KEY_UP:
		game.setMoveForward(false);
		break;
	case GLUT_KEY_LEFT:
		game.setTurnLeft(false);
		game.setStrafeLeft(false);
		break;
	case GLUT_KEY_RIGHT:
		game.setTurnRight(false);
		game.setStrafeRight(false);
		break;		
	}
}

int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL);
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(500, 500);

	glutCreateWindow("ex07 - Mirror");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutTimerFunc(UPDATE_INTERVAL_MS,timer,0);
	glutSpecialFunc(keyboardFunc);
	glutSpecialUpFunc(keyboardUpFunc);
	
	//glutFullScreen();

	init();

	glutMainLoop();

	return 0;  
}

