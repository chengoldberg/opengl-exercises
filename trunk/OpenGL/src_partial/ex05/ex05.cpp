#define _CRT_SECURE_NO_WARNINGS
#include <GL/glew.h>
#include <stdlib.h>
#include <stdio.h>
#include "Mesh.h"
#include "GameLogic.h"
#include <time.h>

#pragma comment (lib, "glew32.lib")

#define MESH_FILE_LOW "../res/mesh/bomber1.off"

GameLogic g_game;	
int g_width, g_height;		// Window dimensions
Mesh g_mesh;				// The mesh object

// FPS counter
int g_curDiff = 0;
long g_timeDiff[8];
long g_lastTime = 0;

// Display lists
int g_curveListID = -1, g_floorListID = -1;

// ============================== Helper functions ============================

void drawString(char* txt) {
	while(*txt != '\0') {
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *txt);
		txt++;		
	}
}

void updateFPS() {
	long temp = clock();
	g_timeDiff[g_curDiff++] = temp - g_lastTime;
	g_lastTime = temp;
	g_curDiff = g_curDiff % 5;		
}

double calcFPS() {
	return 1000/((double)(g_timeDiff[0]+g_timeDiff[1]+g_timeDiff[2]+g_timeDiff[3]+g_timeDiff[4])/5);
}

// ============================== Drawing functions ============================

void drawHUD() {	
	static char temp[25];

	sprintf(temp+00, "FPS: %.2f", calcFPS());

	glPushMatrix();

	glViewport(0,0,g_width,g_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,g_width,0,g_height,-100,100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	glColor3d(1,0,0);


	glRasterPos2d((g_width-105)/2,(g_height-20)/2);

	drawString(temp);
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void drawFloor() {

	if(g_floorListID == -1) {
		g_floorListID = glGenLists(1);
		glNewList(g_floorListID, GL_COMPILE);

		glDisable(GL_LIGHTING);
		glColor3d(0.75, 0.75, 0.75);
		glBegin(GL_LINES);
		double s = 60;
		for(double i = 0; i <= 1; i+=0.1) {
			glVertex3d(0, 0, i*s-s/2);
			glVertex3d(s, 0, i*s-s/2);

			glVertex3d(i*s, 0, -s/2);
			glVertex3d(i*s, 0, +s/2);				
		}
		glEnd();
		glEnable(GL_LIGHTING);	

		glEndList();
	}		
	glCallList(g_floorListID);
}

void drawCurve() {

	if(g_curveListID == -1) {
		g_curveListID = glGenLists(1);
		glNewList(g_curveListID, GL_COMPILE);

		glDisable(GL_LIGHTING);
		glColor3d(1, 1, 1);
		glBegin(GL_LINE_STRIP);
		for(double i=0;i<=1;i+=0.01) {
			double* point = g_game.calcCurveAt(i);
			glVertex2d(point[0], point[1]);
		}
		glEnd();		
		glEnable(GL_LIGHTING);		
		glEndList();								
	}		
	g_curveListID = glGenLists(1);
}

void drawAirplane(Airplane* airplane) {

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, airplane->pigment.get());

	glPushMatrix();
	{
		// World transformations
		glTranslated(airplane->x, airplane->y, 0);		

		glRotated(airplane->angle, 0, 0, 1);

		// Object transfromations (specific to "bomber1")
		glRotated(+90, 0, 1, 0);
		glRotated(-90, 1, 0, 0);		
		glScaled(0.1, 0.1, 0.1);

		// Draw object
		g_mesh.render();
	}
	glPopMatrix();				
}

void drawAirplanes() {
	for(unsigned int i=0;i<g_game.airplanes.size();++i) {
		Airplane* airplane = g_game.airplanes.at(i);
		drawAirplane(airplane);
	}
}

void drawWorld() {

	// Sky light
	glEnable(GL_LIGHT0);
	float temp[] = {0,1,0,0};
	glLightfv(GL_LIGHT0, GL_POSITION, temp);

	// Draw world
	drawFloor();
	drawCurve();
	drawAirplanes();		
}

// ============================== Setup functions ============================

void loadMesh(char* filename) {
	g_mesh.loadFromFile(filename);
	g_mesh.initAll();
}

void init() {

	// Set background color to black
	glClearColor(0, 0, 0, 0);		

	// Make sure normals remain normalized
	glEnable(GL_NORMALIZE);

	// Setup material
	float temp2[] = {0,0,0,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, temp2);
	float temp3[] = {1,1,1,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, temp3);

	// Setup sky light		
	float temp4[] = {0.8f,0.8f,0.8f,1};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, temp4);
	float temp5[] = {0.15f,0.15f,0.15f,0};
	glLightfv(GL_LIGHT0, GL_AMBIENT, temp5);
	float temp6[] = {0.1f,0.1f,0.1f,0};
	glLightfv(GL_LIGHT0, GL_SPECULAR, temp6);		

	// Initialze mesh
	loadMesh(MESH_FILE_LOW);
}

// ============================== GLUT callbacks ============================

void display(void) {

	// Update FPS counter
	updateFPS();

	// Update game model
	g_game.update();		

	// Draw

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | 	GL_DEPTH_BUFFER_BIT);							

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);				
	glEnable(GL_LIGHTING);		
	glEnable(GL_DEPTH_TEST);

	//TODO: Back face culling!

	// Obtain location and orientation of the first airplane
	double x = g_game.getAirplanes().at(0)->x;
	double y = g_game.getAirplanes().at(0)->y;
	double angle = g_game.getAirplanes().at(0)->angle;		

	//
	// Viewport4 - Side view
	//		
	glViewport(0, 0, g_width/2, g_height/2);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();	
	glOrtho(-1, 31, 0, 15, -100, 100);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();		

	drawWorld();

	//
	// Viewport2 - Isometric
	//		
	glViewport(g_width/2+1, 0, g_width/2, g_height/2);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();	
	gluPerspective(60, (double)g_width/g_height, 0.1, 1000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(x+0.7, y+0.7, +0.7, x, y, 0, 0, 1, 0);

	drawWorld();

	//
	// Viewport3 - Over the head camera
	//		
	glViewport(g_width/2+1, g_height/2+1, g_width/2, g_height/2);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();			
	gluPerspective(60, (double)g_width/g_height, 0.1, 1000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();		
	glTranslated(0, -0.35, -1.4);
	glRotated(90, 0, 1, 0);
	glRotated(-angle, 0, 0, 1);		
	glTranslated(-x, -y, 0);

	drawWorld();

	//
	// Viewport 4 - Wide view 
	//
	glViewport(0, g_height/2, g_width/2, g_height/2);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();			
	gluPerspective(90, (double)g_width/g_height, 0.1, 1000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();				
	glRotated(-60, 0, 1, 0);
	glTranslated(-29, -8, -1);	

	drawWorld();				

	//
	// Fillscreen Viewport
	//	
	drawHUD();

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {
	g_width = width;
	g_height = height;		
}

void keyboardFunc(int key, int x, int y) {
	switch(key) {
	case GLUT_KEY_DOWN: 
		g_game.removeAirplane();
		break;
	case GLUT_KEY_UP:
		g_game.addAirplane();
		break;
	case GLUT_KEY_LEFT:
		g_game.speedDown();
		break;
	case GLUT_KEY_RIGHT:
		g_game.speedUp();
		break;
	case GLUT_KEY_F1:
		g_mesh.setRenderMode(RENDER_IMMEDIATE);
		puts("Immediate");
		break;
	case GLUT_KEY_F2:
		g_mesh.setRenderMode(RENDER_DISPLAY_LIST);
		puts("Display list");
		break;
	case GLUT_KEY_F3:
		g_mesh.setRenderMode(RENDER_VERTEX_ARRAY_HOP);
		puts("Vertex Array hop");
		break;
	case GLUT_KEY_F4:
		g_mesh.setRenderMode(RENDER_VERTEX_ARRAY_SEQ);
		puts("Vertex Array seq");
		break;
	case GLUT_KEY_F5:
		g_mesh.setRenderMode(RENDER_VBO_HOP);
		puts("VBO hop");
		break;
	case GLUT_KEY_F6:
		g_mesh.setRenderMode(RENDER_VBO_SEQ);
		puts("VBO seq");
		break;
	}
}

int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(500, 500);

	glutCreateWindow("ex05 - Acceleration");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutSpecialFunc(keyboardFunc);

	glewInit();
	init();
	
	//glutFullScreen();

	puts("Controls:");
	puts("Up/Down Arrows - Add/Remove object");
	puts("F1 - Immediate mode");
	puts("F2 - Display list");
	puts("F3 - Vertex Array Hop");
	puts("F4 - Vertex Array Seq");
	puts("F5 - VBO hop");
	puts("F6 - VBO Seq");

	glutMainLoop();

	return 0;  
}

