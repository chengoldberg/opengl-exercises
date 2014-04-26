#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#define _USE_MATH_DEFINES
#include "GameLogic.h"

#define UPDATE_INTERVAL_MS 16

GameLogic g_game;	
int g_frame;

// Common colors
float colorBlack[] = {0,0,0,1};
float colorDarkGray[] = {0.2f,0.2f,0.2f,1};
float colorLightGray[] = {0.8f,0.8f,0.8f,1};
float colorWhite[] = {1,1,1,1};

// ============================== Drawing functions =========================

void drawObjects() {

	GLUquadric* q = gluNewQuadric();

	// Draw glowing sphere
	float gs_ambdiff[] = {1,1,0,1};
	float gs_emission[] = {0.5f,0.5f,0,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gs_ambdiff);	
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, gs_emission);		
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);	

	glPushMatrix();
	{
		glTranslated(3, 0.75+sin(g_frame*0.075)*0.25, 3);
		gluSphere(q, 0.5, 8, 8);
	}
	glPopMatrix();

	// Draw Big sphere		
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorWhite);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		
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
	glLightfv(GL_LIGHT0, GL_SPECULAR, colorBlack);		

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

	//TODO: Polygon fill		
	//TODO: Enable Lighting				
	//TODO: Depth Test
	//TODO: Flashlight (GL_LIGHT1)
	//TODO: Sky light (GL_LIGHT0)
	//TODO: Sphere light (GL_LIGHT2)
	//TODO: Sphere emission
	//TODO: Sphere light contain
	//TODO: Lighting model
void display(void) {

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);			

	// Create camera transformation
	setupCamera();	

	// Draw world
	drawFloor();
	drawWalls();		
	drawObjects();

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

void timer(int value) {

	glutTimerFunc(UPDATE_INTERVAL_MS,timer,0);

	// Advance frame every 16 milliseconds
	g_frame++;

	// Update g_game model
	g_game.update();		

	glutPostRedisplay();
}

void keyboardFunc(unsigned char key, int x, int y) {
	switch(key) {
	case 27:	// Quit on 'Escape' key
		exit(0);
	}
}

int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH)-512)/2, (glutGet(GLUT_SCREEN_HEIGHT)-512)/2);
	glutInitWindowSize(512, 512);

	glutCreateWindow("ex04 - Lights");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	//glutIdleFunc(display);
	glutSpecialFunc(keyboardFunc);
	glutSpecialUpFunc(keyboardUpFunc);
	glutKeyboardFunc(keyboardFunc);
	glutTimerFunc(UPDATE_INTERVAL_MS,timer,0);

	//glutFullScreen();

	init();

	glutMainLoop();

	return 0;  
}

