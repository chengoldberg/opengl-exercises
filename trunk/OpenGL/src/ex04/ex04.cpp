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
#include "GameLogic.h"

GameLogic g_game;	
int g_frame;

// Common colors
float colorBlack[] = {0,0,0,1.0f};
float colorDarkGray[] = {0.2f,0.2f,0.2f,1.0f};
float colorLightGray[] = {0.8f,0.8f,0.8f,1.0f};
float colorWhite[] = {1,1,1,1};

// ============================== Drawing methods =========================

void drawObjects() {

	GLUquadric* q = gluNewQuadric();

	// Draw glowing sphere
	float gs_ambdiff[] = {1,1,0,1};
	float gs_emission[] = {0.5,0.5,0,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gs_ambdiff);	
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, gs_emission);		
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);


	glPushMatrix();
	{
		glTranslated(3, 0.75+sin(g_frame*0.075)*0.25, 3);
		gluSphere(q, 0.5, 8, 8);
	}
	glPopMatrix();

	// Contain sphere light		
	glDisable(GL_LIGHT2);

	// Draw Big sphere		
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorWhite);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100); // Note range! [0,127]
	glPushMatrix();
	{
		glTranslated(7, 1, 3);
		gluSphere(q, 0.75, 32, 32);
	}
	glPopMatrix();		

	// Draw rotating cylinder
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorWhite);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		
	glPushMatrix();
	{
		glTranslated(11, 1, 3);
		glRotated(g_frame, 0, 1, 0);
		glTranslated(0, 0, -1);
		gluCylinder(q, 1, 1, 2, 40, 40);
	}
	glPopMatrix();
}

void drawWalls() {

	float ambdiff[] = {0.75f,0.75f,0.75f,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambdiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);				

	glBegin(GL_QUADS);

	glNormal3d(0, 0, -1);
	for(int i=0;i<HOR_TOTAL;++i) {
		int* wall = GameLogic::horWalls[i];
		glVertex3d(wall[0], 0, wall[1]);
		glVertex3d(wall[0], 2, wall[1]);
		glVertex3d(wall[0]+1, 2, wall[1]);
		glVertex3d(wall[0]+1, 0, wall[1]);			

	}

	glNormal3d(1, 0, 0);
	for(int i=0;i<VER_TOTAL;++i) {
		int* wall = GameLogic::verWalls[i];
		glVertex3d(wall[0], 0, wall[1]);
		glVertex3d(wall[0], 2, wall[1]);
		glVertex3d(wall[0], 2, wall[1]+1);
		glVertex3d(wall[0], 0, wall[1]+1);			

	}		
	glEnd();		
}

void drawFloor() {

	float ambdiff[] = {0.65f,0.65f,0.65f,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambdiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		

	glBegin(GL_QUADS);		
	glNormal3d(0, 1, 0);
	for(int i=0;i<g_game.getBoardWidth();++i)
		for(int j=0;j<g_game.getBoardHeight();++j) {							
			glVertex3d(0+i, 0, 0+j);
			glVertex3d(0+i, 0, 1+j);
			glVertex3d(1+i, 0, 1+j);
			glVertex3d(1+i, 0, 0+j);			
		}
		glEnd();		
}	

// ============================== Setup methods =========================

void init() {

	glEnable(GL_NORMALIZE);

	// Set background color to black
	glClearColor(0, 0, 0, 0);		

	// Set default color to white 
	glColor3d(1, 1, 1);

	// Configure depth buffer
	glClearDepth(1);			// Clear to 1

	// Configure depth test
	glDepthFunc(GL_LESS);

	/*
	* Setup constant lighting attributes 
	*/

	// Setup sky light		
	float l0_ambdiff[] = {0.8f,0.8f,0.8f,1};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0_ambdiff);
	glLightfv(GL_LIGHT0, GL_AMBIENT, colorBlack);
	glLightfv(GL_LIGHT0, GL_SPECULAR, colorWhite);		

	// Setup flashlight
	float l1_ambdiff[] = {1,0,0,1};
	glLightfv(GL_LIGHT1, GL_DIFFUSE, l1_ambdiff);
	glLightfv(GL_LIGHT1, GL_AMBIENT, colorBlack);				
	glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 10);
	glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 100);

	// Setup sphere light
	float l2_pos[] = {0,0,0,1};
	glLightfv(GL_LIGHT2, GL_POSITION, l2_pos);
	glLightfv(GL_LIGHT2, GL_AMBIENT, colorBlack);		
	float l2_ambdiffspec[] = {1,1,0,1};
	glLightfv(GL_LIGHT2, GL_DIFFUSE, l2_ambdiffspec);		
	glLightfv(GL_LIGHT2, GL_SPECULAR, l2_ambdiffspec);
	glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.5);				
}

/**
* Create camera transformation that captures the player's POV
*/
void setupCamera() {

	// Convert player's angle to world angle
	double angle = g_game.getAngle()*180.0/M_PI - 90;

	glRotated(angle, 0,1,0);
	glTranslated(-g_game.getPlayerLoc()[0], -1, -g_game.getPlayerLoc()[1]);
}	

// ============================== GLUT methods =========================

void display(void) {

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

	// Flashlight
	glEnable(GL_LIGHT1);
	float temp[] = {0,0,0,1};
	glLightfv(GL_LIGHT1, GL_POSITION, temp);

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | 	GL_DEPTH_BUFFER_BIT);			

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);			

	glEnable(GL_LIGHTING);

	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	glEnable(GL_DEPTH_TEST);	

	// Create camera transformation
	setupCamera();	

	// Sky light
	glEnable(GL_LIGHT0);
	float l0_pos[] = {0.5f,1,1,0};
	glLightfv(GL_LIGHT0, GL_POSITION, l0_pos);

	// Sphere light
	glEnable(GL_LIGHT2);
	glPushMatrix();
	glTranslated(3, 0.75+sin(g_frame*0.075)*0.25, 3);
	glLightfv(GL_LIGHT2, GL_POSITION, colorBlack);
	glPopMatrix();			

	// Draw world
	drawFloor();
	drawWalls();		
	drawObjects();

	// Advance time counter
	//g_frame++;

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

void keyboardFunc(int key, int x, int y) {
	switch(key) {
	case GLUT_KEY_DOWN: 
		g_game.setMoveBackward(true);
		break;
	case GLUT_KEY_UP:
		g_game.setMoveForward(true);
		break;
	case GLUT_KEY_LEFT:
		g_game.setTurnLeft(true);
		break;
	case GLUT_KEY_RIGHT:
		g_game.setTurnRight(true);
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
		break;
	case GLUT_KEY_RIGHT:
		g_game.setTurnRight(false);
		break;
	}
}

#define UPDATE_INTERVAL_MS 16
void timer(int value) {

	// Advance frame every 16 milliseconds
	g_frame++;

	// Update g_game model
	g_game.update();		

	glutTimerFunc(UPDATE_INTERVAL_MS,timer,0);
}

int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(500, 500);

	glutCreateWindow("ex04 - Lights");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutSpecialFunc(keyboardFunc);
	glutSpecialUpFunc(keyboardUpFunc);

	glutTimerFunc(UPDATE_INTERVAL_MS,timer,0);

	//glutFullScreen();

	init();

	glutMainLoop();

	return 0;  
}

