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
#define MAG_FILTERS_NUM 2
#define MIN_FILTERS_NUM 6
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

bool		g_isBlending;
bool		g_isMultiTexture;

char		g_textureMagFiltersNames[][30] = {"GL_LINEAR", "GL_NEAREST"};
char		g_textureMinFiltersNames[][30] = {"GL_LINEAR", "GL_NEAREST", "GL_NEAREST_MIPMAP_NEAREST", "GL_NEAREST_MIPMAP_LINEAR", "GL_LINEAR_MIPMAP_NEAREST", "GL_LINEAR_MIPMAP_LINEAR"};
GLuint		g_textureMagFilters[] = {GL_LINEAR, GL_NEAREST};
GLuint		g_textureMinFilters[] = {GL_LINEAR, GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR};
int			g_curTextureMagFilter;
int			g_curTextureMinFilter;

double		g_latitudeD;
double		g_latitude;
double		g_distanceD;
double		g_distance;

float		g_colorWhite[] = {1,1,1,1};
float		g_colorLightGray[] = {0.75,0.75,0.75,1};
float		g_colorDarkGray[] = {0.25,0.25,0.25,1};
float		g_colorBlack[] = {0,0,0,1};


void initTexFromFile() {

	CTargaImage img;

	char *filename = TEXTURE_FILENAME;

	if(!img.Load(filename)){
		printf("Unable to load %s\n", filename);
		return;
	}

	glBindTexture(GL_TEXTURE_2D, g_textures[TEX_FROM_FILE]);

	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.GetWidth(), img.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, img.GetImage());	
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, img.GetWidth(), img.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE, img.GetImage());

	img.Release();						
}


//
// Puts a CPU generated iamge in texture object g_textures[TEX_GENERATED] 
//
void initTexGenerated() {

	static GLubyte pixels[TEXTURE_WIDTH][TEXTURE_HEIGHT][4];

	int span = (int) powl(2,CHECKER_SIZE);
	for(int i = 0;i<TEXTURE_WIDTH;++i)		
		for(int j = 0;j<TEXTURE_HEIGHT;++j) {
			bool isBlack = ((i/span)%2==1) ^ ((j/span)%2==1);
			pixels[i][j][0] = isBlack?0:255;
			pixels[i][j][1] = isBlack?0:255;
			pixels[i][j][2] = isBlack?0:255;
			pixels[i][j][3] = isBlack?0:255;
		}

		glBindTexture(GL_TEXTURE_2D, g_textures[TEX_GENERATED]);

		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

//
// Links the FBO g_fboID to the texture object g_textures[TEX_FROM_FBO] 
//
void initTexFromFBO() {

	GLuint rbID;

	glGenFramebuffers(1, &g_fboID);
	glBindFramebuffer(GL_FRAMEBUFFER, g_fboID);

	// Now we need to attach a color buffer
	glBindTexture(GL_TEXTURE_2D, g_textures[TEX_FROM_FBO]);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_textures[TEX_FROM_FBO], 0);

	// Now a depth buffer!	
	glGenRenderbuffers(1, &rbID);
	glBindRenderbuffer(GL_RENDERBUFFER, rbID);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, TEXTURE_WIDTH, TEXTURE_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbID);

	GLuint status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		printf("Bad framebuffer init!\n");

	// Get back to defualt framebuffer!
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//
// Draws a scene into the framebuffer
//
void DrawToFBO() {

	glBindFramebuffer(GL_FRAMEBUFFER, g_fboID);

	// Remember: Different framebuffer - Same context!

	glClearColor(1,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
	glViewport(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1,1,-1,1,100,-100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glScaled(0.75,0.75,0.75);
	glRotated(g_frame/3.0,0,1,0);
	glutSolidTeapot(1);

	glPopMatrix();

	// Updates smaller mipmaps levels
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setupFlagTexture() {
	glActiveTexture(GL_TEXTURE0);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_textures[g_curTexture]);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); //SOLVES bug in NVIDIA drivers!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, g_textureMagFilters[g_curTextureMagFilter]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_textureMinFilters[g_curTextureMinFilter]);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glActiveTexture(GL_TEXTURE1);

	if(g_isMultiTexture) {
		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, g_textures[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); //SOLVES bug in NVIDIA drivers!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, g_textureMagFilters[g_curTextureMagFilter]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_textureMinFilters[g_curTextureMinFilter]);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	} else {
		glDisable(GL_TEXTURE_2D);
	}
}

void drawFlag() {

	double flagWidth = 2;
	double flagHeight = 1.5;
	double d = 0.05;						// Space between vertices
	double maxOffset = 0.15;				// Max inflation for flag
	double norm[3] = {0,0,0};	

	double prevOffset = 0;					
	double prevNorm[] = {0,0,0};

	setupFlagTexture();

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

		glTexCoord2d(1-x/flagWidth, 0);
		glMultiTexCoord2d(GL_TEXTURE1, 1-x/flagWidth, 0);
		glVertex3d(x, 0, prevOffset);

		glTexCoord2d(1-x/flagWidth, 1);
		glMultiTexCoord2d(GL_TEXTURE1, 1-x/flagWidth, 1);
		glVertex3d(x, flagHeight, prevOffset);

		// Set vertices of current column
		glNormal3dv(norm);

		glTexCoord2d(1-(x+d)/flagWidth, 1);
		glMultiTexCoord2d(GL_TEXTURE1, 1-(x+d)/flagWidth, 1);
		glVertex3d(x+d, flagHeight, offset);

		glTexCoord2d(1-(x+d)/flagWidth, 0);
		glMultiTexCoord2d(GL_TEXTURE1, 1-(x+d)/flagWidth, 0);
		glVertex3d(x+d, 0, offset);			

		// Keep current column normal and offset
		prevOffset = offset;
		prevNorm[0] = norm[0];
		prevNorm[2] = norm[2];
	}
	glEnd();

	glDisable(GL_TEXTURE_2D);
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
	g_curTextureMagFilter = 0;
	g_curTextureMinFilter = 0;

	glEnable(GL_NORMALIZE);

	// The data we use doesn't have any special alignment 
	// must specify if we deal with non-4 sizes, e.g. RGB (default state value is 4!)
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
	if(g_isBlending) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else
		glDisable(GL_BLEND);
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
	glTranslatef(0,0,(GLfloat) -pow(g_distance,2));
	glRotated(g_latitude,0,1,0);

	// Wolrd coordinates

	float l0Pos[] = {10,10,10,1};
	glLightfv(GL_LIGHT0, GL_POSITION, l0Pos);

	glTranslated(-2/2,-3.0/2.0-1.5/2.0,0);	
	drawPoleAndFlag();

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);

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
	case GLUT_KEY_F4:		
		g_curTextureMagFilter++;
		g_curTextureMagFilter %= MAG_FILTERS_NUM;
		printf("Texture mag filter: %s\n", g_textureMagFiltersNames[g_curTextureMagFilter]);
		break;
	case GLUT_KEY_F5:		
		g_curTextureMinFilter++;
		g_curTextureMinFilter %= MIN_FILTERS_NUM;
		printf("Texture min filter: %s\n", g_textureMinFiltersNames[g_curTextureMinFilter]);
		break;
	case GLUT_KEY_F6:
		g_isBlending ^= true;
		printf("Blending is %s\n", g_isBlending?"ON":"OFF");
		break;
	case GLUT_KEY_F7:
		g_isMultiTexture ^= true;
		printf("Multi-Texture is %s\n", g_isMultiTexture?"ON":"OFF");
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
	puts("F4 - Change mag filter");
	puts("F5 - Change min filter");
	puts("F6 - Toggle blending");
	puts("F7 - Toggle multi-texture");

	glutMainLoop();

	return 0;  
}

