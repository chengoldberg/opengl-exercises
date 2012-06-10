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
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <math.h>
#include "GameLogic.h"
#include "CTargaImage.h"
#include "CTargaImage.cpp"
#include "Skybox.h"
#include "Skybox.cpp"
#include "common.h"

#pragma comment (lib, "glew32.lib")
using namespace std;

#define SHADER_PARTICLES_VERT "../res/shader/ParticleWave.vert"
#define SHADER_PARTICLES_FRAG "../res/shader/ParticleWave.frag"
#define SHADER_TOON_VERT "../res/shader/Toon.vert"
#define SHADER_TOON_FRAG "../res/shader/Toon.frag"
#define SHADER_CLOUD_VERT "../res/shader/cloud.vert"
#define SHADER_CLOUD_FRAG "../res/shader/cloud.frag"
#define SHADER_CONV_VERT "../res/shader/Conv.vert"
#define SHADER_CONV_FRAG "../res/shader/Conv.frag"
#define SHADER_CUBEMAP_VERT "../res/shader/cubemap.vert"
#define SHADER_CUBEMAP_FRAG "../res/shader/cubemap.frag"
#define SHADER_CONV_FS_EDGE_FRAG "../res/shader/ConvFS_edge.frag"
#define SHADER_CONV_FS_BLUR_FRAG "../res/shader/ConvFS_blur.frag"
#define TEXTURE_FILENAME_IMAGE "../res/tex_2d/tomb.tga"
#define CUBE_MAP_PATH "../res/tex_cube/"

#define MAX_KERNEL_SIZE 25
#define RANDOM ((double)rand() / ((double)(RAND_MAX)+(double)(1)))

char* EConvMode_str[] = {"None", "Frame", "Object"};
enum EConvMode {
	CONV_NONE,
	CONV_FS,
	CONV_OBJECT,

	EConvModeLength
};

char* EConvFilter_str[] = {"Blur","Edge"};
enum EConvFilter {
	CONV_BLUR,
	CONV_EDGE,
	
	EConvFilterLength
};

unsigned int	g_frame;
GameLogic		g_game;
GLuint			g_fboID;
GLuint			g_noise3Dtex, g_imageTex, g_fboColorTex, g_fboDepthTex;
GLuint			g_toonProg, g_cloudProg, g_uniformScale, g_convProg, g_convBlurProg, g_convEdgeProg, g_cubemapProg,  
				g_uniformLightPos, g_uniformSkyColor, g_uniformCloudColor,g_uniformNoise, g_uniformOffset,
				g_particlesProg, g_uniformTime, g_particlesList,
				g_uniformDiffuseColor, g_uniformPhongColor, g_uniformEdge, g_uniformPhong,
				g_unifromConvOffset, g_unifromKernelSize, g_uniformKernelValue,g_uniformBaseImage, g_uniformDepthImage,
				g_attribParam, g_paramsVBO;
float			g_noiseOffset[3] = {0,0,0};
int				g_height, g_width;
EConvMode		g_convMode;
EConvFilter		g_convFilter;
CSkybox*		g_skybox;

float colorDarkGray[] = {0.2f,0.2f,0.2f,0};

void CreateNoise3D();

// ============================== Helper Functions =========================

string readfile(char *filename) {
	ifstream file(filename);
	file.seekg(0, ios::end);GLuint size = (GLuint) file.tellg();file.seekg(0, ios::beg); 
	string text(size + 1, 0);
	file.read((char*)text.data(), size);
	return text;
}

GLuint loadTexFromFile(char* filename) {

	CTargaImage img;
	GLuint texId;

	glGenTextures(1, &texId);	

	if(!img.Load(filename)){
		printf("Unable to load %s\n", filename);
		return 0;
	}

	glBindTexture(GL_TEXTURE_2D, texId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if(img.GetImageFormat() == 1)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.GetWidth(), img.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.GetImage());	
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.GetWidth(), img.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, img.GetImage());	

	img.Release();						

	return texId;
}

int checkProgramInfoLog(GLuint prog) {
	int len = 0, read = 0;
	string log;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
	if (len > 0)
	{
		log.resize(len);
		glGetProgramInfoLog(prog, len, &read, (GLchar*) log.data());
		printf("Program Info Log:\n%s\n", log.c_str());
	}
	int ret;
	glGetProgramiv(prog, GL_LINK_STATUS, &ret);
	
	return ret;
}

bool myCompileShader(GLuint shader) {
	GLint compiled;

	glCompileShader(shader);

	glGetObjectParameterivARB(shader, GL_COMPILE_STATUS, &compiled);
	GLint blen = 0;	GLsizei slen = 0;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &blen);       

	if (blen > 1)
	{
	 GLchar* compiler_log = (GLchar*)malloc(blen);
	 glGetInfoLogARB(shader, blen, &slen, compiler_log);
	 cout << "compiler_log:\n" << compiler_log << "\n";
	 free (compiler_log);
	}

	return compiled == 1;
}

GLuint createProgramFast(char* vertFn, char* fragFn) {

	printf("\nCreating program: %s and %s\n", vertFn, fragFn);

	string temp = readfile(vertFn);
	char* srcVert = new char[temp.size()+1];;
	strcpy(srcVert, temp.c_str());

	temp = readfile(fragFn);
	char* srcFrag = new char[temp.size()+1];;
	strcpy(srcFrag, temp.c_str());

	bool OK = true;

	//puts("Compiling Vertex Shader\n");
	GLuint vsId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vsId, 1, (const GLchar**) &srcVert, NULL);
	OK = OK && myCompileShader(vsId);
	
	//puts("Compiling Fragment Shader\n");
	GLuint fsId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fsId, 1, (const GLchar**) &srcFrag, NULL);
	OK = OK && myCompileShader(fsId);

	if(!OK) {
		printf("Failed to compile, quitting!\n");
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vsId);
	glAttachShader(program, fsId);
	glLinkProgram(program);

	//glValidateProgram(program);

	if(checkProgramInfoLog(program) == 0) {
		printf("Failed to load program from shaders %s and %s\n", vertFn, fragFn);
		return 0;
	} else {
		printf("Program created!");
	}

	return program;
}

void beginFBO() {

	glBindFramebuffer(GL_FRAMEBUFFER, g_fboID);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			

	// Remember: Different framebuffer - Same context!
}
void endFBO() {

	// Updates smaller mipmaps levels
	//glGenerateMipmap(GL_TEXTURE_2D);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void updateConvUniforms() {

	static float gaussian5[] = {
		1/121.0f, 2/121.0f, 3/121.0f, 2/121.0f, 1/121.0f,					
		2/121.0f, 7/121.0f, 11/121.0f, 7/121.0f, 2/121.0f,
		3/121.0f, 11/121.0f, 17/121.0f, 11/121.0f, 3/121.0f,
		2/121.0f, 7/121.0f, 11/121.0f, 7/121.0f, 2/121.0f,
		1/121.0f, 2/121.0f, 3/121.0f, 2/121.0f, 1/121.0f
	};
	/*
	static float laplacian[3][3] = {
		{-1/8.f,-1/8.f,-1/8.f},
		{-1/8.f, 1    , -1/8.f},
		{-1/8.f,-1/8.f,-1/8.f}
	};
*/
	static float laplacian[9] = {
		-1/8.f,-1/8.f,-1/8.f,
		-1/8.f, 1    , -1/8.f,
		-1/8.f,-1/8.f,-1/8.f
	};
	float offset[MAX_KERNEL_SIZE*2];
	float* kernelPtr;
	int kernelSize;
	int spread;
	GLuint program;

	switch(g_convFilter) {
		case CONV_BLUR:			
			spread = 3;
			kernelSize = 5;			
			kernelPtr = (float*) gaussian5;		
			program = g_convBlurProg;
			break;

		case CONV_EDGE:
			spread = 1;
			kernelSize = 3;			
			kernelPtr = (float*) laplacian;	
			program = g_convEdgeProg;
			break;
		default:
			printf("Undefined convolution filter!");
			return;
	}

	int c = 0;
	int ext = ((kernelSize-1)/2);
	for(int i=0;i<kernelSize;++i)
		for(int j=0;j<kernelSize;++j) {
			offset[c++] = ((i-ext)/(float)g_width)*spread;
			offset[c++] = ((j-ext)/(float)g_height)*spread;
		}

	g_convProg = program;
	glUseProgram(g_convProg);
	glUniform2fv(g_unifromConvOffset, kernelSize*kernelSize, offset);
	glUniform1i(g_unifromKernelSize, kernelSize*kernelSize);
	glUniform1fv(g_uniformKernelValue, kernelSize*kernelSize, kernelPtr);
	glUniform1i(g_uniformBaseImage,0);
	glUniform1i(g_uniformDepthImage,1);
	glUseProgram(0);
}

// ============================== Drawing Procedures =========================

void drawFlag() {
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_imageTex);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glBegin(GL_QUADS);
	glTexCoord2d(0,0);
	glVertex3d(-1,0,0);	
	glTexCoord2d(1,0);
	glVertex3d(+1,0,0);	
	glTexCoord2d(1,1);
	glVertex3d(+1,+2,0);	
	glTexCoord2d(0,1);
	glVertex3d(-1,+2,0);	
	glEnd();

	glDisable(GL_TEXTURE_2D);
}


void drawObjects() {

	GLUquadric* q = gluNewQuadric();

	// Draw cube
	float gs_ambdiff[] = {1,1,0,1};
	float gs_emission[] = {0.5,0.5,0,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gs_ambdiff);	
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);		
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);

	glPushMatrix();
	{
		glTranslated(8, 0.75+sin(g_frame*0.075)*0.25, 8);
		glutSolidCube(1);
	}
	glPopMatrix();

	glUseProgram(g_cubemapProg);
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, g_skybox->m_tex);

	glPushMatrix();
	{
		glTranslated(8, 2.5+sin(g_frame*0.075)*0.5, -8);
		glutSolidSphere(2,24,24);
	}
	glPopMatrix();
	glUseProgram(0);

	glPushMatrix();
	{
		glTranslated(-8,0,8);
		glRotated(-45,0,1,0);
		drawFlag();
	}
	glPopMatrix();

	if(g_convMode == CONV_OBJECT)
		beginFBO();
	// Draw teapot
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gs_ambdiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		

	glPushMatrix();
	{
		glTranslated(-5, 1, 0);
		glutSolidTeapotFIX(2);
	}
	glPopMatrix();		


	if(g_convMode == CONV_OBJECT)
		endFBO();

	glUseProgram(g_toonProg);
	glPushMatrix();
	{
		glTranslated(5, 1, 0);
		glRotated(-180,0,1,0);
		glutSolidTeapotFIX(2);
	}
	glPopMatrix();		
	glUseProgram(0);

	// Draw rotating cylinder
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorWhite);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		
	glPushMatrix();
	{
		glTranslated(-10, 1, -10);
		glRotated(g_frame, 0, 1, 0);
		glTranslated(0, 0, -1);
		gluCylinder(q, 1, 1, 2, 40, 40);
	}
	glPopMatrix();
}

void drawFloor() {

	static float ambdiff[] = {0.65f,0.65f,0.65f,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambdiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, colorBlack);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, colorWhite);		

	glPushMatrix();
	{
		glTranslated(-g_game.getBoardWidth()/2.0, 0,-g_game.getBoardHeight()/2.0);
		glBegin(GL_QUADS);		
		glNormal3d(0, 1, 0);
		for(int i=0;i<g_game.getBoardWidth();++i)
			for(int j=0;j<g_game.getBoardHeight();++j) {							
				glTexCoord2d(0,0);
				glVertex3d(0+i, 0, 0+j);
				glTexCoord2d(0,1);
				glVertex3d(0+i, 0, 1+j);
				glTexCoord2d(1,1);
				glVertex3d(1+i, 0, 1+j);
				glTexCoord2d(1,0);
				glVertex3d(1+i, 0, 0+j);			
			}
		glEnd();	
	}
	glPopMatrix();
}	

	GLuint g_rboID;

void initFBO() {

	int texWidth = g_width; 
	int texHeight = g_height;

	glGenFramebuffers(1, &g_fboID);
	glBindFramebuffer(GL_FRAMEBUFFER, g_fboID);

	// Now we need to attach a color buffer
	glGenTextures(1, &g_fboColorTex);
	glBindTexture(GL_TEXTURE_2D, g_fboColorTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_fboColorTex, 0);

	// Now a depth buffer!
	glGenTextures(1, &g_fboDepthTex);
	glBindTexture(GL_TEXTURE_2D, g_fboDepthTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, texWidth, texHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_fboDepthTex, 0);

	/*
	glGenRenderbuffers(1, &g_rboID);
	glBindRenderbuffer(GL_RENDERBUFFER, g_rboID);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, texWidth, texHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_rboID);
	*/

	GLuint status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		printf("Bad framebuffer init! (code: %d)\n", status);
	else
		printf("framebuffer init OK!\n");

	// Get back to defualt framebuffer!
	glBindFramebuffer(GL_FRAMEBUFFER, 0);	
}

void deleteFBO() {
	glDeleteFramebuffers(1, &g_fboID);
	glDeleteRenderbuffers(1, &g_rboID);
	glDeleteTextures(1, &g_fboColorTex);
}

void drawScreenAligned() {

	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_ALPHA_TEST);
	//glAlphaFunc(GL_GREATER,0);

	glUseProgram(g_convProg);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_fboDepthTex);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_fboColorTex);

	glDisable(GL_LIGHTING);

	if(g_convMode == CONV_OBJECT)
		glEnable(GL_BLEND);
	glBlendFunc(GL_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();	// Push Projection matrix!	
	glLoadIdentity();

	glOrtho(0,g_width,+g_height,0,-1000,1000);	

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glBegin(GL_QUADS);
	glTexCoord2d(0,1);
	glVertex2d(0,0);
	glTexCoord2d(1,1);
	glVertex2d(g_width,0);
	glTexCoord2d(1,0);
	glVertex2d(g_width,g_height);	
	glTexCoord2d(0,0);
	glVertex2d(0,g_height);
	glEnd();

	glPopMatrix();	// Pop modelview matrix!

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();	// Pop Projection matrix!
	
	glMatrixMode(GL_MODELVIEW);

	glUseProgram(0);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
}

void drawParticles() {
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDepthMask(false);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(g_particlesProg);
	glUniform1f(g_uniformTime,g_frame/25.0f);

	glBindBuffer(GL_ARRAY_BUFFER, g_paramsVBO);
	glVertexAttribPointer(g_attribParam, 2, GL_FLOAT, false, 0, 0);
	glVertexPointer(2, GL_FLOAT, 0, 0);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableVertexAttribArray(g_attribParam);
	glDrawArrays(GL_POINTS, 0, 8000);
	glDisableVertexAttribArray(g_attribParam);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glCallList(g_particlesList);
	
	glDepthMask(true);
	glDisable(GL_BLEND);
	glUseProgram(0);
}

void drawWorld() {
	static float l0_pos[] = {-10,10,10,1};
	glLightfv(GL_LIGHT0, GL_POSITION, l0_pos);

	glUseProgram(g_cloudProg);
	{
		glUniform3fv(g_uniformOffset, 1, g_noiseOffset);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, g_noise3Dtex);
		glUniform1i(g_uniformNoise, 0);
		glEnable(GL_BLEND);
		drawFloor();
		glDisable(GL_BLEND);
	}
	glUseProgram(0);

	
	//glUseProgram(g_toonProg);
	{
		drawObjects();	
	}
	//glUseProgram(0);	

	glPushMatrix();
	{
		glTranslated(-2,3,0);	
		drawParticles();
	}
	glPopMatrix();

}

// ============================== Init Procedures =========================


void initParticles() {

	g_particlesProg = createProgramFast(SHADER_PARTICLES_VERT, SHADER_PARTICLES_FRAG);

	g_uniformTime = glGetUniformLocation(g_particlesProg, "time");
	g_attribParam = glGetAttribLocation(g_particlesProg, "Param");

	float params[8000][2];
	for(int i=0;i<8000;i++) {		
		params[i][0] = (float)RANDOM;
		params[i][1] = (float)RANDOM;		
	}

	glGenBuffers(1, &g_paramsVBO);	
	glBindBuffer(GL_ARRAY_BUFFER, g_paramsVBO);
	glBufferData(GL_ARRAY_BUFFER, 8000*2*4, params, GL_STATIC_DRAW);		

	glVertexAttribPointer(g_attribParam, 2, GL_FLOAT, false, 0, 0);
	glVertexPointer(2, GL_FLOAT, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

/*	
	g_particlesList = glGenLists(1);
	glNewList(g_particlesList, GL_COMPILE);
	glBegin(GL_POINTS);
	for(int i=0;i<8000;i++) {		
		//glColor4d(RANDOM,RANDOM,RANDOM,RANDOM);
		glVertexAttrib2d(g_attribParam, RANDOM, RANDOM);
		glVertex3d(0,0,0);				
	}
	glEnd();
	glEndList();
*/	

	
}

void initCubemap() {
	g_cubemapProg = createProgramFast(SHADER_CUBEMAP_VERT, SHADER_CUBEMAP_FRAG);	

	GLuint cubemap = glGetUniformLocation(g_toonProg, "CubeMap");
	glUseProgram(g_cubemapProg);
	glUniform1i(cubemap, 0);
	glUseProgram(0);
}

void initToon() {
	g_toonProg = createProgramFast(SHADER_TOON_VERT, SHADER_TOON_FRAG);	

	g_uniformDiffuseColor = glGetUniformLocation(g_toonProg, "DiffuseColor");
	g_uniformPhongColor = glGetUniformLocation(g_toonProg, "PhongColor");
	g_uniformEdge = glGetUniformLocation(g_toonProg, "Edge");
	g_uniformPhong = glGetUniformLocation(g_toonProg, "Phong");

	glUseProgram(g_toonProg);
	glUniform1f(g_uniformEdge, 0.3f);
	glUniform1f(g_uniformPhong, 0.95f);
	glUniform3f(g_uniformPhongColor, 0.8f,0.8f,0.8f);
	glUniform3f(g_uniformDiffuseColor, 0.2f,0.2f,1);
	glUseProgram(0);
}

void initCloud() {
	g_cloudProg = createProgramFast(SHADER_CLOUD_VERT, SHADER_CLOUD_FRAG);

	g_uniformScale = glGetUniformLocation(g_cloudProg, "Scale");
	g_uniformLightPos = glGetUniformLocation(g_cloudProg, "LightPos");
	g_uniformSkyColor = glGetUniformLocation(g_cloudProg, "SkyColor");
	g_uniformCloudColor = glGetUniformLocation(g_cloudProg, "CloudColor");
	g_uniformNoise = glGetUniformLocation(g_cloudProg, "Noise");
	g_uniformOffset = glGetUniformLocation(g_cloudProg, "Offset");

	glUseProgram(g_cloudProg);
	glUniform1f(g_uniformScale, 0.15f);
	//glUniform3f(g_uniformLightPos, 10,10,10);
	glUniform4f(g_uniformSkyColor, 1,1,1,0);
	glUniform4f(g_uniformCloudColor, 1,1,1,1);
	glUseProgram(0);
}

void initConv() {
	
	g_convBlurProg = createProgramFast(SHADER_CONV_VERT, SHADER_CONV_FS_BLUR_FRAG);
	g_convEdgeProg = createProgramFast(SHADER_CONV_VERT, SHADER_CONV_FS_EDGE_FRAG);
	
	// They share the same uniforms so it doesn't matter

	g_unifromConvOffset = glGetUniformLocation(g_convBlurProg, "Offset");
	g_unifromKernelSize = glGetUniformLocation(g_convBlurProg, "KernelSize");
	g_uniformKernelValue = glGetUniformLocation(g_convBlurProg, "KernelValue");
	g_uniformBaseImage = glGetUniformLocation(g_convBlurProg, "BaseImage");
	g_uniformDepthImage = glGetUniformLocation(g_convBlurProg, "DepthImage");
}

void initNoise3DTexture() {
    glEnable(GL_TEXTURE_3D);

	glGenTextures(1, &g_noise3Dtex);
	glBindTexture(GL_TEXTURE_3D, g_noise3Dtex);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	CreateNoise3D();

	glDisable(GL_TEXTURE_3D);
}

void init() {

	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_NORMALIZE);
	glPointSize(3);	

	// Setup sky light		
	float l0_amb[] = {0.5f,0.5f,0.5f,1};
	float l0_diff[] = {1,1,1,1};
	float l0_spec[] = {0.05f,0.05f,0.05f,1};
	
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0_diff);
	glLightfv(GL_LIGHT0, GL_AMBIENT, colorBlack);
	glLightfv(GL_LIGHT0, GL_SPECULAR, colorWhite);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50);

	//initFBO();
	initNoise3DTexture();
	initParticles();
	initToon();
	initCloud();
	initConv();
	initCubemap();

	g_imageTex = loadTexFromFile(TEXTURE_FILENAME_IMAGE);

	g_skybox = new CSkybox();
	g_skybox->init(30, CUBE_MAP_PATH);
}

//
// Create camera transformation that captures the player's POV
//
void setupCamera() {

	// Convert player's angle to world angle
	double angle = g_game.getAngle()*180.0/M_PI - 90;

	glRotated(angle, 0,1,0);
	glTranslated(-g_game.getPlayerLoc()[0], -1, -g_game.getPlayerLoc()[1]);
}	

// ============================== GLUT Callbacks =========================

void display(void) {

	glDepthFunc(GL_LEQUAL);
	glDisable(GL_BLEND);

	// Set background color 
	//glClearColor(0.5,0.5,0.5,1);		
	glClearColor(1,0,0,1);		

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			

	// Create camera transformation
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if(g_convMode == CONV_FS)
		beginFBO();

	// Draw Skybox
	glPushMatrix();
	{
		double angle = g_game.getAngle()*180.0/M_PI - 90;
		glRotated(angle,0,1,0);
		g_skybox->Render(0,0,0);
	}
	glPopMatrix();

	setupCamera();	

	// Sky light
	glEnable(GL_LIGHT0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);			
	glEnable(GL_LIGHTING);
	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_DEPTH_TEST);			

	// Draw world
	drawWorld();
	
	if(g_convMode == CONV_FS) 
		endFBO();	

	if(g_convMode != CONV_NONE)
		drawScreenAligned();

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {

	g_width = width; g_height = height;

	// Create perspective projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(60, (double)width/height, 0.1, 1000);
	glViewport(0,0,width,height);

	deleteFBO();
	initFBO();

	updateConvUniforms();
}

void timer(int value) {

	glutTimerFunc(16,timer,0);

	g_noiseOffset[1]+=0.003f;
	g_noiseOffset[2]+=0.000f;
	g_noiseOffset[0]+=0.001f;
	
	// Update game model
	g_game.update();		

	// Advance time counter
	g_frame++;		
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
		if(glutGetModifiers() == GLUT_ACTIVE_ALT)
			g_game.setStrafeLeft(true);
		else
			g_game.setTurnLeft(true);
		break;
	case GLUT_KEY_RIGHT:
		if(glutGetModifiers() == GLUT_ACTIVE_ALT)
			g_game.setStrafeRight(true);
		else
			g_game.setTurnRight(true);
		break;
	case GLUT_KEY_F1:
		g_convMode = (EConvMode)((g_convMode+1) % EConvModeLength);
		printf("Convolution mode: %s\n", EConvMode_str[g_convMode]);
		break;
	case GLUT_KEY_F2:
		g_convFilter = (EConvFilter)((g_convFilter+1) % EConvFilterLength);
		printf("Convolution filter: %s\n", EConvFilter_str[g_convFilter]);
		updateConvUniforms();
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
		g_game.setStrafeLeft(false);
		break;
	case GLUT_KEY_RIGHT:
		g_game.setTurnRight(false);
		g_game.setStrafeRight(false);
		break;		
	}
}

int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE | GLUT_ALPHA);
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(500, 500);

	glutCreateWindow("ex09 - Using GLSL Shaders");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutTimerFunc(16,timer,0);
	glutSpecialFunc(keyboardFunc);
	glutSpecialUpFunc(keyboardUpFunc);
	
	//glutFullScreen();

	glewInit();

	init();
	
	printf("\n");
	printf("Implementation Details\n");
	printf("======================\n");
	printf("OpenGL 2.0 Supported: %d\n", GLEW_VERSION_2_0);
	printf("\nUsage\n");
	printf("======================\n");
	printf("F1 - Change Convolution target {Frame, Object, None}\n");
	printf("F2 - Change Convolution Filter {Gaussian, Laplacian}\n");

	if(!GLEW_VERSION_2_0)
		return -1;

	glutMainLoop();

	return 0;  
}

