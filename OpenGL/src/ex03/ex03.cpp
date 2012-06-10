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
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

struct Point{
	int x,y;

	Point() {}
	Point(int _x, int _y) : x(_x), y(_y) {}
};

int g_width, g_height;
Point g_prevMouse;
double g_rotY;
double g_rotX;

// Fractal tree variables
double g_angle;
double g_maxDepth;
double g_factor;	
double g_splits;

// Menu overhead
int g_curMenu = 0;

// ============================== GUI methods ========================

void drawString(char* txt) {
	while(*txt != '\0') {
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *txt);
		txt++;		
	}
}

void changeVal(int delta) {

	static double* A[] = {&g_angle,&g_maxDepth,&g_factor,&g_splits};	
	static double steps[] = {1,1,0.01,1};
	static double max[] = {180,8,1,8};
	static double min[] = {0,0,0.1,0};

	*(A[g_curMenu])+=delta*steps[g_curMenu];
	if(*(A[g_curMenu]) > max[g_curMenu]) *(A[g_curMenu]) = max[g_curMenu];
	if(*(A[g_curMenu]) < min[g_curMenu]) *(A[g_curMenu]) = min[g_curMenu];
}

void drawHUD() {	

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();	// Push Projection matrix!
	
	glLoadIdentity();
	glOrtho(0,g_width,-g_height,0,-1000,1000);	

	char temp[100];

	sprintf(temp+00, "Angle: %.2f", g_angle);
	sprintf(temp+25, "Depth: %.0f", g_maxDepth);
	sprintf(temp+50, "factor: %.2f", g_factor);
	sprintf(temp+75, "Splits: %.0f", g_splits);

	for(int i=0;i<4;++i) {		
		if(g_curMenu==i) 
			glColor3d(1,0,0);
		else
			glColor3d(1,0.5,0);		
		glRasterPos2d(0.0, -20.0*(i+1.0));
		drawString(temp+25*i);	
	}	
	glPopMatrix();	// Pop Projection matrix!
	glMatrixMode(GL_MODELVIEW);
}

// ============================== Drawing methods =========================

void drawTreeRec(int level) {

	GLUquadric* quad = gluNewQuadric();

	// Draw cylinder
	glColor3d(0.5, 0.5, 0.5);		
	gluCylinder(quad, 0.1, 0.1*g_factor, 1, 8, 1);

	glTranslated(0, 0, 1);

	// Recursion stopping condition
	if(level == g_maxDepth) {

		// Draw red sphere
		glColor3d(1, 0, 0);
		gluSphere(quad, 0.25, 8, 8);
		return;
	} 

	// Setup local coordinate system for each child and draw

	glScaled(g_factor, g_factor, g_factor);

	for(int i = 0; i < g_splits; ++i) {			

		glPushMatrix();

		glRotated(g_angle, 0, 1, 0);
		drawTreeRec(level+1);
		glPopMatrix();

		glRotated(360/g_splits, 0, 0, 1);
	}
}

/**
* Rotate view along x and y axes 
* @param x Angles to rotate around x
* @param y Angles to rotate around y
*/
void rotate(double x, double y) {
	g_rotX += x;
	g_rotY += y;
}

void init() {

	g_factor = 0.6;
	g_maxDepth = 4;
	g_angle = 45;
	g_splits = 2;

	glClearColor(1, 1, 1, 0);
}

/**
* Create camera transformation such that the model is rotated around the
* world's X-axis and Y-axis. 
*/
void setupCamera() {

	double temp[16];

	glMatrixMode(GL_MODELVIEW);		
	glGetDoublev(GL_MODELVIEW_MATRIX, temp);

	// Rotate along the (0 1 0) in world coordinates
	glRotated(-g_rotX, temp[1], temp[5], temp[9]);
	g_rotX = 0;

	glGetDoublev(GL_MODELVIEW_MATRIX, temp);

	// Rotate along the (1 0 0) in world coordinates
	glRotated(-g_rotY, temp[0], temp[4], temp[8]);			
	g_rotY = 0;				
}

// ============================== GLUT methods =========================

void display(void) {

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT);		

	// Create camera transformation
	setupCamera();		

	glPushMatrix();
	glLoadIdentity();
	drawHUD();
	glPopMatrix();

	// Save camera transformation
	glPushMatrix();

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// Draw tree a bit lower 
	glTranslated(0, -1, 0);		
	glRotated(-90, 1, 0, 0);

	// Draw tree		
	drawTreeRec(0);

	// Load camera transformation
	glPopMatrix();

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {

	g_width = width;
	g_height = height;

	// Create projection transformation
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glViewport(0,0,width,height);

	double w,h;
	w = 5;
	h = 5*((double)height/width);
	glOrtho(-w/2, w/2, -h/2, h/2, -1000, 1000);				
}

void motionFunc(int x, int y) {

	// Calc difference from previous mouse location
	Point prev = g_prevMouse;
	int dx = prev.x - x;
	int dy = prev.y - y;

	// Rotate model
	rotate(dx, dy);

	// Remember mouse location 
	g_prevMouse = Point(x,y);	
}

void mouseFunc(int button, int state, int x, int y) {
	if(button == GLUT_LEFT_BUTTON){
		g_prevMouse = Point(x,y);	
	}
}

void keyboardFunc(int key, int x, int y) {
	switch(key) {
	case GLUT_KEY_DOWN: 
		g_curMenu++;
		break;
	case GLUT_KEY_UP:
		g_curMenu--;
		break;
	case GLUT_KEY_LEFT:
		changeVal(-1);
		break;
	case GLUT_KEY_RIGHT:
		changeVal(+1);
		break;
	}

	g_curMenu = g_curMenu<0?0:g_curMenu;
	g_curMenu = g_curMenu>3?3:g_curMenu;
}

int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(500, 500);

	glutCreateWindow("ex3 - Transformations (Fractal Tree)");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutMotionFunc(motionFunc);
	glutMouseFunc(mouseFunc);
	glutSpecialFunc(keyboardFunc);

	//glutFullScreen();

	init();

	glutMainLoop();

	return 0;  
}

