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

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

struct Point{
	int x,y;

	Point() {}
	Point(int _x, int _y) : x(_x), y(_y) {}
};

Point g_prevMouse;
double g_rotY;
double g_rotX;

/*
 * Rotate view along x and y axes 
 * @param x Angles to rotate around x
 * @param y Angles to rotate around y
 */
void rotate(double x, double y) {
	g_rotX += x;
	g_rotY += y;
}

/**
 * Draw a unit size RGB cube
 */
void drawRGBCube() {
	double r = 1;

	glBegin(GL_QUADS);
	glColor3d(0,0,1);
	glVertex3d(-r,-r,+r);
	glColor3d(1,0,1);
	glVertex3d(+r,-r,+r);
	glColor3d(1,1,1);
	glVertex3d(+r,+r,+r);
	glColor3d(-1,1,1);
	glVertex3d(-r,+r,+r);

	glColor3d(0,0,0);
	glVertex3d(-r,-r,-r);
	glColor3d(0,0,1);
	glVertex3d(-r,-r,+r);
	glColor3d(0,1,1);
	glVertex3d(-r,+r,+r);
	glColor3d(0,1,0);
	glVertex3d(-r,+r,-r);

	glColor3d(1,0,1);
	glVertex3d(+r,-r,+r);
	glColor3d(1,0,0);
	glVertex3d(+r,-r,-r);
	glColor3d(1,1,0);
	glVertex3d(+r,+r,-r);
	glColor3d(1,1,1);
	glVertex3d(+r,+r,+r);

	glColor3d(1,1,0);
	glVertex3d(+r,+r,-r);
	glColor3d(1,0,0);
	glVertex3d(+r,-r,-r);
	glColor3d(0,0,0);
	glVertex3d(-r,-r,-r);
	glColor3d(0,1,0);
	glVertex3d(-r,+r,-r);			

	glColor3d(0,1,1);
	glVertex3d(-r,+r,+r);
	glColor3d(1,1,1);
	glVertex3d(+r,+r,+r);
	glColor3d(1,1,0);
	glVertex3d(+r,+r,-r);
	glColor3d(0,1,0);
	glVertex3d(-r,+r,-r);

	glColor3d(0,0,0);
	glVertex3d(-r,-r,-r);
	glColor3d(1,0,0);
	glVertex3d(+r,-r,-r);
	glColor3d(1,0,1);
	glVertex3d(+r,-r,+r);
	glColor3d(0,0,1);
	glVertex3d(-r,-r,+r);		

	glEnd();				
}

void init() {
	
	// Set background color to gray
	glClearColor(0.5f, 0.5f, 0.5f, 0);

	// Place camera at (0,0,10)
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();			
	glTranslated(0, 0, -10);	
}

/**
* Create camera transformation such that the model is rotated around the
* world's X-axis and Y-axis. 
*/
void setupCamera() {

	double temp[16];

	glMatrixMode(GL_MODELVIEW);		
	glGetDoublev(GL_MODELVIEW_MATRIX, temp);

	// Rotate along temp in world coordinates
	glRotated(-g_rotX, temp[1], temp[5], temp[9]);
	g_rotX = 0;

	glGetDoublev(GL_MODELVIEW_MATRIX, temp);

	// Rotate along the (1 0 0) in world coordinates
	glRotated(-g_rotY, temp[0], temp[4], temp[8]);			
	g_rotY = 0;				
}

void display(void) {

	// Clear color buffer
	glClear(GL_COLOR_BUFFER_BIT);		

	//glEnable(GL_DEPTH_TEST);

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		

	// Create camera transformation
	setupCamera();		

	// Save camera transformation
	glPushMatrix();

	// Init states		
	glPolygonMode(GL_BACK, GL_POINT);
	glLineWidth(5);
	glPointSize(5);

	// Draw Line RGB cube
	glPolygonMode(GL_FRONT, GL_LINE);		
	glTranslated(-3, 0, 0);
	drawRGBCube();

	// Draw Fill RGB cube
	glPointSize(1);
	glPolygonMode(GL_FRONT, GL_FILL);		
	glTranslated(3, 0, 0);		
	drawRGBCube();
	glPointSize(5);

	// Draw Point RGB cube
	glPolygonMode(GL_FRONT, GL_POINT);
	glTranslated(3, 0, 0);
	drawRGBCube();

	// Load camera transformation
	glPopMatrix();

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {

	// Setup viewport
	glViewport(0,0,width,height);

	// Create projection transformation
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	double w,h;
	w = 10;
	h = 10*((double)height/width);
	glOrtho(-w/2, w/2, -h/2, h/2, -1, 1000);					
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

int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(500, 500);

	glutCreateWindow("ex2 - Drawing RGB Cube");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutMotionFunc(motionFunc);
	glutMouseFunc(mouseFunc);

	//glutFullScreen();

	init();

	glutMainLoop();

	return 0;  
}
