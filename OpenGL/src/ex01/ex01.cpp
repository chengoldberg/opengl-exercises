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
#define _USE_MATH_DEFINES
#include <math.h>

int g_frame = 0;	// Keeps track of time

// ============================== Drawing methods =========================

void drawCircle(double x, double y, double s) {

	glPushMatrix();

	glTranslated(x, y, 0);
	glScaled(s, s, 0);

	glBegin(GL_POLYGON);
	{
		for(double alpha = 2*M_PI/36; alpha<2*M_PI; alpha+=2*M_PI/36) {
			glVertex2d(cos(alpha), sin(alpha));
		}
	}
	glEnd();		
	glPopMatrix();
}

void drawTriangle(double x, double y, double s) {

	glPushMatrix();

	glTranslated(x, y, 0);
	glScaled(s, s, 0);

	glBegin(GL_POLYGON);
	{
		glVertex2d(-1, 0);
		glVertex2d(1, 0);
		glVertex2d(0, 1);

	}
	glEnd();		
	glPopMatrix();
}

void drawSquare(double x, double y, double s) {

	glPushMatrix();

	glTranslated(x, y, 0);
	glScaled(s, s, 0);

	glBegin(GL_POLYGON);
	{
		glVertex2d(-1, -1);
		glVertex2d(1, -1);
		glVertex2d(1, 1);
		glVertex2d(-1, 1);
	}
	glEnd();		
	glPopMatrix();
}	

void animate() {

	glLoadIdentity();
	double s = (cos(g_frame/100.0));
	glRotated(s*30, 0, 0, 1);
	s = (s+1)/2;
	s = (1-s)*1/5 + s*1;
	glScaled(s, s, s);		
}

// ============================== GLUT methods =========================

void display(void) {

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT);		

	// Apply animation
	animate();

	// Draw scene

	// Sky
	glColor3d(0, 1, 0);
	drawSquare(0, -10, 10);

	// Ground
	glColor3d(0, 1, 1);
	drawSquare(0, +10, 10);		

	// Sun
	glColor3d(1, 0.75, 0);
	drawCircle(-2,2,0.5);

	// House

	glColor3d(0.85, 0.85, 0.85); //TODO: Color
	drawSquare(0, 0, 0.5);

	// Roof
	glColor3d(1, 0.5, 0); //TODO: Color
	drawTriangle(0, 0.5, 0.5);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);	//TODO: Black Border
	glColor3d(0, 0, 0);
	drawTriangle(0, 0.5, 0.5);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Windows

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);	//TODO: color & polygon line mode
	glColor3d(0.3, 0.3, 0.85);					//
	drawSquare(-0.5/2, +0.5/4, 0.5/4);					
	drawSquare(+0.5/2, +0.5/4, 0.5/4);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	//

	glutSwapBuffers();
}

void reshape(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Maintain aspect ratio
	if (w <= h)
		glOrtho(-2.5, 2.5, -2.5 * (GLfloat) h / (GLfloat) w,
		2.5 * (GLfloat) h / (GLfloat) w, -10.0, 10.0);
	else
		glOrtho(-2.5 * (GLfloat) w / (GLfloat) h,
		2.5 * (GLfloat) w / (GLfloat) h, -2.5, 2.5, -10.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
}

void timer(int value) {

	// Advance frame every 16 milliseconds
	g_frame++;

	glutTimerFunc(16,timer,0);
}

/*  
*  Main Loop
*  Open window with initial window size, title bar, 
*  RGBA display mode, and handle input events.
*/
int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(500, 500);
	glutCreateWindow("ex01 - States & Basic Drawing");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);

	glutTimerFunc(16,timer,0);

	//glutFullScreen();

	glutMainLoop();

	return 0;  
}

