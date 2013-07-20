/*
 * Copyright (C) 2013  Chen Goldberg
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

//#define GLM_PRECISION_HIG

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <string>
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cgl/gl/common.hpp"
#include "cgl/gl/asset_library.hpp"
#include "cgl/gl/collada_importer.hpp"
#include "cgl/gl/scene_render.hpp"

// Not sure why the window we get is different from the size we request...
#define GLUT_WINDOW_PAD 8
#define COLLADA_FILENAME "../res/COLLADA/animation_tests/animation_test_linear.dae"

int g_frame;
int	g_height, g_width;
cgl::AssetLibrary g_assetLibrary;
cgl::AnimationSystem g_animationSystem;

enum EScenes
{
	CODED_SCENE,
	COLLADA_SCENE,	
	TOTAL_SCENES
};
const char* g_scenesNames[] = {"test-scene","Scene"};
const char* g_camerasNames[] = {"test-camera-instance","Camera"};
EScenes g_displayScene = COLLADA_SCENE;

void drawScene()
{	
	cgl::ssg::SceneGraphRoot* scene = g_assetLibrary.getScene(g_scenesNames[g_displayScene]);	
	cgl::renderScene(scene, g_camerasNames[g_displayScene], g_assetLibrary.getEffect("default"));
}

void initAssetsFromCOLLADA()
{	
	cgl::importCollada(COLLADA_FILENAME, g_assetLibrary, &g_animationSystem);		
	cgl::ssg::SceneGraphRoot* scene = g_assetLibrary.getScene("Scene");
	scene->accept(&ScenePrettyPrinter());
}

void init() 
{
	// Init GL Debug
	cgl::initDebugAll();

	// Init assets
	initAssetsFromCOLLADA();

	// Set background color to gray
	glClearColor(0.7f, 0.7f, 0.4f, 0);

	g_frame = 0;
}

void display(void) 
{
	glEnable(GL_DEPTH_TEST);

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		

	g_animationSystem.update(g_frame/100.0f);

	// Apply shaders
	drawScene();

	//cgl::dumpColorBuffer("out.raw", g_width, g_height);
	
	// Swap double buffer
	glutSwapBuffers();

	g_frame++;
}

void reshape(int width, int height) 
{
	g_width = width-GLUT_WINDOW_PAD;
	g_height = height-GLUT_WINDOW_PAD;
	glViewport(0,0,g_width,g_height);
}

void keyboardFunc(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:	// Escape key
		exit(0);
		break;

	default:
		break;
	}
}

void keyboardSpecialFunc(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_F1:
		g_displayScene = EScenes::COLLADA_SCENE;
		break;

	case GLUT_KEY_F2:
		g_displayScene = EScenes::CODED_SCENE;
		break;

	default:
		break;
	}
}

int main(int argc, char **argv) 
{
	try
	{
		glutInitContextVersion(4,3);
		glutInitContextProfile(GLUT_CORE_PROFILE);
		//glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
		glutInitContextFlags(GLUT_DEBUG);

		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
		glutInitWindowPosition(0, 0);
		glutInitWindowSize(512+GLUT_WINDOW_PAD*2, 520+GLUT_WINDOW_PAD*2);

		glutCreateWindow("ex17 - Asset management");

		glutReshapeFunc(reshape);
		glutDisplayFunc(display);
		glutIdleFunc(display);
		glutKeyboardFunc(keyboardFunc);
		glutSpecialFunc(keyboardSpecialFunc);
		//glutFullScreen();

		// Glew limitation 
		// Ref: http://openglbook.com/glgenvertexarrays-access-violationsegfault-with-glew/
		glewExperimental = GL_TRUE; 
		glewInit();

		init();

		glutMainLoop();
	}
	catch(std::exception ex)
	{
		std::cerr << ex.what() << std::endl;
		exit(-1);
	}
	catch(...)
	{
		exit(-1);
	}

	return 0;  
}

// Note - when instantiating non-mutalbe assets - must do proper clone (e.g. a scene's transformation - anything that can be changed by an animator)