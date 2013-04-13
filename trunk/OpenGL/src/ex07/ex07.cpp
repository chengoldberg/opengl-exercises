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

#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#include "GameLogic.h"
#include "common.h"

#define MIRROR_WIDTH 3
#define MIRROR_HEIGHT 4

unsigned int frame;
GameLogic game;
float C[] = {30,30,30,0};

float colorMagenta[] = {0.75f,0,0.75f,0};
float colorDarkGray[] = {0.2f,0.2f,0.2f,0};
float colorBg[] = {0, 162/255.0f, 232/255.0f, 0};

// ============================== Drawing methods =========================

void drawObjects() {

	GLUquadric* q = gluNewQuadric();

	// Draw sphere
	float gs_ambdiff[] = {1,0,0,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gs_ambdiff);	
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);		
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);

	glPushMatrix();
	{
		glTranslated(0, 0.75+sin(frame*0.075)*0.25, -5);
		glRotated(frame,0,1,0);
		glRotated(-60,1,0,0);
		glutSolidCone(1,1.5,16,8);
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
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorMagenta);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		
	glPushMatrix();
	{
		glTranslated(6, 1, -7);
		glRotated(frame, 0, 1, 0);
		glTranslated(0, 0, -1);
		gluCylinder(q, 1, 1, 2, 40, 40);
	}
	glPopMatrix();
}

void drawSun() {
	GLUquadric* q = gluNewQuadric();

	// Draw Sun
	float s_ambdiff[] = {1,1,0,1};
	float s_emission[] = {1,1,0,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, s_ambdiff);	
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, s_emission);		
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);

	glPushMatrix();
	{
		glTranslated(C[0],C[1],C[2]);
		gluSphere(q, 1, 16, 16);
	}
	glPopMatrix();
}

void drawGrass() {
	static float ambdiff[] = {0,0.9f,0,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambdiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorBlack);		

	glPushMatrix();
	{
		glTranslated(-game.getBoardWidth()/2.0, 0,game.getBoardHeight()*2.0/3.0);
		//glTranslated(-game.getBoardWidth()/2.0, 0,1);
		glBegin(GL_TRIANGLES);		
		glNormal3d(0, 0, -1);

		for(int i=0;i<game.getBoardWidth();++i) {
			glVertex3d(1+i,		0, 0);
			glVertex3d(0+i,		0, 0);			
			glVertex3d(0.5+i,	2, 0);
		}
		glEnd();	
	}
	glPopMatrix();
}

void drawFloor() {

	static float ambdiff[] = {0.65f,0.65f,0.65f,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambdiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorBlack);		

	glPushMatrix();
	{
		glTranslated(-game.getBoardWidth()/2.0, 0,-game.getBoardHeight()/3.0);
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
	static float ambdiff[] = {0.65f,0.05f,0.05f,1};
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
	static float ambdiff[] = {0.0f,0.05f,0.75f,1};
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
	glPushMatrix();
	{
		glTranslated(+MIRROR_WIDTH/2.0-0.1,0,0);
		glTranslated(0,MIRROR_HEIGHT,0);
		glRotated(-90,0,1,0);
		gluCylinder(q,0.2,0.2,MIRROR_WIDTH,10,3);
	}
	glPopMatrix();
	glPushMatrix();
	{
		glTranslated(+MIRROR_WIDTH/2.0+0.1,0,0);
		glRotated(-90,0,1,0);
		gluCylinder(q,0.2,0.2,MIRROR_WIDTH,10,3);
	}
	glPopMatrix();
}

void drawActor() {

	float ambdiff[] = {0,0.75,0.75,1};
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

void drawWorld() {	
	drawMirrorBorder();
	drawFloor();
	drawObjects();
}

void drawObjectShadows() {
	glColorMask(false,false,false,false);
	glDisable(GL_LIGHTING);
	glColor3d(1,1,1);
	glPushMatrix();	
/*
	glTranslatef(-C[0],-C[1],-C[2]);
	float mat[] = {
		C[1],0,0,0,
		0,C[1],0,1,
		0,0,+C[1],0,
		0,0,0,0
	};
	glMultMatrixf(mat);
	glTranslatef(C[0],C[1],C[2]);
*/

	float mat[] = {
		1,0,0,0,
		-C[0]/C[1],0,-C[2]/C[1],0,
		0,0,1,0,
		0,0,0,1
	};	
	glMultMatrixf(mat);

	drawMirrorBorder();
	drawMirror();
	drawObjects();
	drawGrass();
	drawActor();
	
	glPopMatrix();
	glEnable(GL_LIGHTING);
	glColorMask(true,true,true,true);
}

void drawWorldWithShadows(GLuint flag) {

	glEnable(GL_STENCIL_TEST);

	// Draw only in given Stencil values
	glStencilFunc(GL_EQUAL, flag, 0xff);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	if(flag) // If Mirror
		drawActor();		
	drawMirrorBorder();
	drawObjects();
	drawGrass();
	drawSun();
	

	// Mark intersection of floor and existing stencil
	glStencilFunc(GL_EQUAL, 3, flag);	
	glStencilOp(GL_ZERO,GL_ZERO,GL_REPLACE);
	
	drawFloor();
		
	glDisable(GL_DEPTH_TEST);
	glStencilFunc(GL_LEQUAL, 1, 0xff);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	drawObjectShadows();

	// Draw ambient on floor in stencil
	glStencilFunc(GL_EQUAL, 1, 0xff);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glDisable(GL_LIGHT0);
	drawFloor();
	glEnable(GL_LIGHT0);

	glDisable(GL_STENCIL_TEST);
}

// ============================== Init methods =========================

void init() {

	glEnable(GL_NORMALIZE);

	// Set background color to black
	glClearColor(colorBg[0],colorBg[1],colorBg[2],colorBg[3]);		

	// Setup sky light		
	float l0_ambdiff[] = {0.75f,0.75f,0.75f,1};
	float l0_spec[] = {0.05f,0.05f,0.05f,1};
	
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0_ambdiff);
	glLightfv(GL_LIGHT0, GL_AMBIENT, colorBlack);
	glLightfv(GL_LIGHT0, GL_SPECULAR, l0_ambdiff);			
	
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 22); // Note range! [0,127]
}

//
// Create camera transformation that captures the player's POV
//
void setupCamera() {

	// Convert player's angle to world angle
	double angle = game.getAngle()*180.0/M_PI - 90;

	glRotated(angle, 0,1,0);
	glTranslated(-game.getPlayerLoc()[0], -1, -game.getPlayerLoc()[1]);

	//gluLookAt(10,10,10,0,0,0,0,1,0);
}	

// ============================== GLUT Callbacks =========================

void display(void) {	

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);			

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Create camera transformation
	setupCamera();	

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, C);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);			
	glEnable(GL_LIGHTING);
	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glEnable(GL_DEPTH_TEST);	

	// Draw world	
	drawWorldWithShadows(0);
		
	// Start stenciling

	glEnable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	glClearStencil(0x0);
	glClear(GL_STENCIL_BUFFER_BIT);
	
	// Mark Mirror
	glStencilFunc(GL_ALWAYS, 0x1, 0xff);	
	glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
		
	glDisable(GL_LIGHTING);
	glColor4f(colorBg[0],colorBg[1],colorBg[2],colorBg[3]);
	drawMirror();
	glEnable(GL_LIGHTING);		

	glClear(GL_DEPTH_BUFFER_BIT);
	
	// Restrict drawing to Mirror
	glStencilFunc(GL_EQUAL, 0x1, 0xff);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	
	glEnable(GL_CLIP_PLANE0);
	double sign = game.loc[1]>0?1:-1;
	double clipPlane[] = {0,0,-sign,0};
	glClipPlane(GL_CLIP_PLANE0, clipPlane);

	glFrontFace(GL_CW);
	glPushMatrix();
	{
		glScaled(1,1,-1);			
		glLightfv(GL_LIGHT0, GL_POSITION, C);
		drawWorldWithShadows(1);
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

void timer(int value) {

	glutTimerFunc(16,timer,0);

	// Advance time counter
	frame++;		

	// Update game model
	game.update();		

	// Update light position
	
	C[0] = (float) 30*cos(frame*0.002f);
	C[2] = (float) 30*sin(frame*0.002f);
	C[1] = 25;
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
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(500, 500);

	glutCreateWindow("ex07 - Stenciling");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutTimerFunc(16,timer,0);
	glutSpecialFunc(keyboardFunc);
	glutSpecialUpFunc(keyboardUpFunc);
	
	//glutFullScreen();

	init();

	glutMainLoop();

	return 0;  
}
