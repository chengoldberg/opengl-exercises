#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <math.h>
#include "CTargaImage.h"
#include "CTargaImage.cpp"
#include "Skybox.h"
#include "Skybox.cpp"

#pragma comment (lib, "glew32.lib")

#define LATITUDE_VEL 2.5
#define DISTANCE_VEL 0.05

#define CUBE_MAP_PATH "../res/tex_cube/"
#define TEXTURE_FILENAME "../res/tex_2d/tomb.tga"
#define TEXTURE_WIDTH 256
#define TEXTURE_HEIGHT 64
#define CHECKER_SIZE 4

enum {
	TEX_GENERATED,
	TEX_FROM_FILE,
	TEX_FROM_FBO
};

// Object IDs
GLuint		g_fboID;		// Framebuffer Object ID
GLuint		g_textures[3];	// Texture Object IDs

CSkybox*	g_skybox;
int			g_winWidth, 
			g_winHeight;

int			g_curTexture;
int			g_frame;

// Camera 
double		g_latitudeD;
double		g_latitude;
double		g_distanceD;
double		g_distance;

// Constants
float		g_colorWhite[] = {1,1,1,1};
float		g_colorLightGray[] = {0.75,0.75,0.75,1};
float		g_colorDarkGray[] = {0.25,0.25,0.25,1};
float		g_colorBlack[] = {0,0,0,1};

// ============================== Texture init functions============================


/*
 * Puts an image from a file in texture object g_textures[TEX_FROM_FILE] 
 */
void initTexFromFile() {

	CTargaImage img;

	char *filename = TEXTURE_FILENAME;

	if(!img.Load(filename)){
		printf("Unable to load %s\n", filename);
		return;
	}

	//TODO: Transfer img.GetImage() to texture object g_textures[TEX_FROM_FILE]

	img.Release();						
}


/*
 * Puts a CPU generated iamge in texture object g_textures[TEX_GENERATED] 
 */
void initTexGenerated() {

	static GLubyte pixels[TEXTURE_WIDTH][TEXTURE_HEIGHT][4];

	//TODO: Create an image in "pixels" and transfer it to texture object g_textures[TEX_GENERATED] 
}

/*
 * Links the FBO g_fboID to the texture object g_textures[TEX_FROM_FBO] 
 */
void initTexFromFBO() {

	//TODO: Create FBO g_fboID and link its render output to texture g_textures[TEX_FROM_FBO] 
}

// ============================== Draw Functions ============================

//
// Draws a scene into the framebuffer
//
void DrawToFBO() {

	//TODO: Draw scene into FBO g_fboID
}

void drawFlag() {

	double flagWidth = 2;
	double flagHeight = 1.5;
	double d = 0.05;						// Space between vertices
	double maxOffset = 0.15;				// Max inflation for flag
	double norm[3] = {0,0,0};	

	double prevOffset = 0;					
	double prevNorm[] = {0,0,0};

	//TODO: Enable 2D textures

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, g_colorWhite);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, g_colorLightGray);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, g_colorWhite);

	glBegin(GL_QUADS);		

	for(double x=0;x<flagWidth;x+=d) {

		// Calculate inflation offset per column
		double offset = sin(x*7-(g_frame/8.0))*maxOffset*(x/flagWidth);

		// Calculate derivative per column
		double der = 7*cos(x*7-(g_frame/8.0))*maxOffset*(x/flagWidth) + sin(x*7-(g_frame/8.0))*maxOffset/flagWidth;

		// Convert derivative ratio to angle
		double alpha = atan(der);

		// Calculate normal
		norm[0] = cos(alpha-M_PI/2);
		norm[2] = sin(alpha-M_PI/2);			

		glNormal3dv(prevNorm);

		//TODO: set texture coordinates!
		glVertex3d(x, 0, prevOffset);
		//TODO: set texture coordinates!
		glVertex3d(x, flagHeight, prevOffset);

		// Set vertices of current column
		glNormal3dv(norm);

		//TODO: set texture coordinates!
		glVertex3d(x+d, flagHeight, offset);
		//TODO: set texture coordinates!
		glVertex3d(x+d, 0, offset);			

		// Keep current column normal and offset
		prevOffset = offset;
		prevNorm[0] = norm[0];
		prevNorm[2] = norm[2];
	}
	glEnd();
}

void drawPoleAndFlag() {
	glPushMatrix();
	{					
		GLUquadric *q = gluNewQuadric();

		// Draw flag pole
		glPushMatrix();
		{			
			glRotated(-90, 1, 0, 0);
			gluCylinder(q, 0.04, 0.04, 3, 10, 3);		
		}
		glPopMatrix();

		// Draw waving flag
		glPushMatrix();
		{			
			glTranslated(0, 1.5,0);
			drawFlag();
		}
		glPopMatrix();
	}
	glPopMatrix();		
}

// ============================== Setup functions ===========================

void setupLighting() {

	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	// Setup sky light		
	glLightfv(GL_LIGHT0, GL_DIFFUSE, g_colorLightGray);
	glLightfv(GL_LIGHT0, GL_AMBIENT, g_colorDarkGray);
	glLightfv(GL_LIGHT0, GL_SPECULAR, g_colorBlack);		

	glEnable(GL_LIGHT0);
}

void setupViewport() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60,(double)g_winWidth/g_winHeight,0.1,100);
	glViewport(0,0,g_winWidth,g_winHeight);
}

void init() {

	g_distance = 2.5;	

	glEnable(GL_NORMALIZE);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);

	// Generate 3 texture object IDs
	glGenTextures(3, g_textures);

	initTexGenerated();
	initTexFromFile();
	initTexFromFBO();

	g_skybox = new CSkybox();
	g_skybox->init(30, CUBE_MAP_PATH);

	setupLighting();
}

// ============================== GLUT callbacks ============================

void display(void) {

	// Draw a scene into the texture
	DrawToFBO();

	//
	// Draw main scene
	//

	// Setup states
	glClearColor(0,0,0,1);	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	setupViewport();

	// Clear color buffer	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Draw Skybox
	glPushMatrix();
	{
		glRotated(g_latitude,0,1,0);
		g_skybox->Render(0,0,0);
	}
	glPopMatrix();

	// Camera transformation
	glTranslatef(0,0,(float) -pow(g_distance,2));
	glRotated(g_latitude,0,1,0);

	// Wolrd coordinates

	float l0Pos[] = {10,10,10,1};
	glLightfv(GL_LIGHT0, GL_POSITION, l0Pos);

	glTranslated(-2/2,-3.0/2.0-1.5/2.0,0);	
	drawPoleAndFlag();

	//g_frame++;

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {
	g_winWidth = width;
	g_winHeight = height;
	glutPostRedisplay();
}

void keyboardFunc(int key, int x, int y) {
	switch(key) {
	case GLUT_KEY_DOWN: 		
		g_distanceD = DISTANCE_VEL;
		break;
	case GLUT_KEY_UP:
		g_distanceD = -DISTANCE_VEL;
		break;
	case GLUT_KEY_LEFT:
		g_latitudeD = LATITUDE_VEL;
		break;
	case GLUT_KEY_RIGHT:
		g_latitudeD = -LATITUDE_VEL;
		break;
	case GLUT_KEY_F1:
		puts("Texture generated");
		g_curTexture = TEX_GENERATED;
		break;
	case GLUT_KEY_F2:
		puts("Texture from file");
		g_curTexture = TEX_FROM_FILE;
		break;
	case GLUT_KEY_F3:
		puts("Texture from FBO");		
		g_curTexture = TEX_FROM_FBO;
		break;
	}
}

void keyboardUpFunc(int key, int x, int y) {
	switch(key) {
	case GLUT_KEY_DOWN: 		
		g_distanceD = 0;
		break;
	case GLUT_KEY_UP:
		g_distanceD = 0;
		break;
	case GLUT_KEY_LEFT:
		g_latitudeD = 0;
		break;
	case GLUT_KEY_RIGHT:
		g_latitudeD = 0;
		break;
	}
}

#define UPDATE_INTERVAL_MS 16
void timer(int value) {

	// Handle Controls
	g_latitude += g_latitudeD;
	g_distance += g_distanceD;

	// Advance frame every 16 milliseconds
	g_frame++;

	glutTimerFunc(UPDATE_INTERVAL_MS,timer,0);
}

int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(500, 500);

	glutCreateWindow("ex06 - Textures");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutSpecialFunc(keyboardFunc);
	glutSpecialUpFunc(keyboardUpFunc);
	glutTimerFunc(UPDATE_INTERVAL_MS,timer,0);

	glewInit();
	if (glewIsSupported("GL_VERSION_2_0"))
		printf("Ready for OpenGL 2.0\n");
	else {
		printf("OpenGL 2.0 not supported\n");
		exit(1);
	}

	init();

	//glutFullScreen();

	puts("Controls");
	puts("Arrows to Rotate/zoom");
	puts("F1 - Place generated texture");
	puts("F2 - Place texture from file");
	puts("F3 - Place texture from FBO");

	glutMainLoop();

	return 0;  
}

