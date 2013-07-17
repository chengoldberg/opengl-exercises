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

// Not sure why the window we get is different from the size we request...
#define GLUT_WINDOW_PAD 8
#define COLLADA_FILENAME "../res/COLLADA/scene_monkey/monkey_uv.dae"

int	g_height, g_width;
cgl::AssetLibrary g_assetLibrary;

enum EScenes
{
	CODED_SCENE,
	COLLADA_SCENE,	
	TOTAL_SCENES
};
const char* g_scenesNames[] = {"test-scene","Scene"};
const char* g_camerasNames[] = {"test-camera-instance","Camera"};
EScenes g_displayScene = COLLADA_SCENE;

//
// The Renderer visitor traverses the scene graph and sets states and renders
// meshes in order. It relies on having the camera and lighting environment
// provided to it. 
//
class GLRendererVisitor : public cgl::ssg::IVisitor
{
public:

	GLRendererVisitor(cgl::Camera* camera, glm::mat4 cameraTransformation, cgl::Lighting* lighting) : _modelView(1) 
	{
		_modelView = cameraTransformation;
		_projection = camera->getMatrix();
		_currentEffect = NULL;
		_lighting = lighting;
	}

	virtual void visit(cgl::ssg::SceneGraphRoot* scene)
	{				
		scene->acceptChildren(this);		
	}

	virtual void visit(cgl::ssg::TransformationNode* transformation)
	{
		glm::mat4 temp = _modelView;
		_modelView *= transformation->getMatrix();
		transformation->acceptChildren(this);
		_modelView = temp;
	}

	virtual void visit(cgl::ssg::EffectInstanceNode* effectNode)
	{
		cgl::Effect* temp = _currentEffect;
		_currentEffect = effectNode->getEffect();		
		_currentEffect->applyEffect();
		_currentEffect->updateLighting(_lighting);
		effectNode->acceptChildren(this);		

		// Apply previous effect
		_currentEffect = temp;
		if(_currentEffect != NULL)
			_currentEffect->applyEffect();
	}

	virtual void visit(cgl::ssg::GeomertyInstanceNode* mesh)
	{		
		_currentEffect->updateEnvUniforms(_modelView, _projection);
		cgl::SimpleMesh* simpleMesh = mesh->getMesh();
		simpleMesh->setAttribLocs(_currentEffect->getAttribLocs());
		simpleMesh->render();
	}

protected:
	glm::mat4 _modelView, _projection;
	cgl::Effect* _currentEffect;
	cgl::Lighting* _lighting;
};

// 
// Visits the scene-graph and finds the world position of lights and a queried camera
//
class SpatialVisitor : public cgl::ssg::IVisitor
{
protected:
	cgl::Camera* _foundCamera;
	glm::mat4 _cameraTransformation;
	std::string _queriedId;
	glm::mat4 _modelWorld;
	std::vector<cgl::Light*> _lights;

public:
	SpatialVisitor(std::string queriedId) : _queriedId(queriedId), _foundCamera(NULL) {};

	bool isCameraFound() { return _foundCamera != NULL; };

	cgl::Camera* getCamera() 
	{
		return _foundCamera;
	}

	glm::mat4 getCameraTransformation() const 
	{
		return glm::inverse(_cameraTransformation);
	}

	std::vector<cgl::Light*> getLights()
	{
		return _lights;
	}

	virtual void visit(cgl::ssg::TransformationNode* transformation)
	{
		glm::mat4 temp = _modelWorld;
		_modelWorld *= transformation->getMatrix();
		transformation->acceptChildren(this);
		_modelWorld = temp;
	}

	virtual void visit(cgl::ssg::LightInstanceNode* light)
	{
		// Update light world positions
		light->getLight()->getProperties().position = _modelWorld * glm::vec4(0,0,0,1);
		_lights.push_back(light->getLight());
	}

	virtual void visit(cgl::ssg::CameraInstanceNode* camera)
	{
		if(camera->getId() == _queriedId) 
		{
			_foundCamera = camera->getCamera();
			_cameraTransformation = _modelWorld;
		}
	}
};

void drawScene()
{	
	cgl::ssg::SceneGraphRoot* scene = g_assetLibrary.getScene(g_scenesNames[g_displayScene]);	
	SpatialVisitor query(g_camerasNames[g_displayScene]);

	scene->accept(&query);
	if(!query.isCameraFound())
		throw std::exception("Camera not found!");
	
	cgl::Lighting lighting;
	lighting.setActiveLights(query.getLights());
	lighting.transformWorldView(query.getCameraTransformation());

	GLRendererVisitor renderer(query.getCamera(), query.getCameraTransformation(), &lighting);
	scene->accept(&renderer);
}

void initAssetsFromCOLLADA()
{	
	cgl::importCollada(COLLADA_FILENAME, g_assetLibrary);		
	cgl::ssg::SceneGraphRoot* scene = g_assetLibrary.getScene("Scene");
	scene->accept(&ScenePrettyPrinter());
}

void initAssetsFromCode()
{	
	// Build library
	cgl::Camera* camera = g_assetLibrary.storeCamera("test-camera", new cgl::Camera(glm::perspective(60.0f,1.0f,1.0f,100.0f)));
	cgl::Light* light = new cgl::Light();
	light->getProperties().diffuse = glm::vec4(1,1,1,1);
	light->getProperties().isEnabled = true;

	cgl::CommonEffect* effect = new cgl::CommonEffect((cgl::CommonProgram*) g_assetLibrary.getProgram("common"));
	effect->init();
	effect->setDiffuseColor(glm::vec4(1,1,0,1));
	effect->setIsLighting(true);

	g_assetLibrary.storeEffect("effect", effect);

	// Build scene graph
	cgl::ssg::SceneGraphRoot* root = new cgl::ssg::SceneGraphRoot();
	cgl::ssg::TransformationNode* cameraTransformNode = new cgl::ssg::TransformationNode(glm::translate(glm::mat4(1), glm::vec3(0,0,0)));
	cgl::ssg::CameraInstanceNode* cameraNode = new cgl::ssg::CameraInstanceNode(camera, "test-camera-instance");
	cameraTransformNode->addChild(cameraNode);
	root->addChild(cameraTransformNode);

	cgl::ssg::TransformationNode* meshTransformNode = new cgl::ssg::TransformationNode(glm::translate(glm::mat4(1), glm::vec3(0,0,-3)));
	cgl::ssg::GeomertyInstanceNode* meshNode = new cgl::ssg::GeomertyInstanceNode(g_assetLibrary.getMesh("Suzanne-mesh"));
	cgl::ssg::EffectInstanceNode* effectNode = new cgl::ssg::EffectInstanceNode(g_assetLibrary.getEffect("effect"));	
	effectNode->addChild(meshNode);
	meshTransformNode->addChild(effectNode);
	root->addChild(meshTransformNode);	

	cgl::ssg::TransformationNode* lightTransformNode = new cgl::ssg::TransformationNode(glm::translate(glm::mat4(1), glm::vec3(0,5,0)));
	cgl::ssg::LightInstanceNode* lightNode = new cgl::ssg::LightInstanceNode(light, "light");
	lightTransformNode->addChild(lightNode);
	root->addChild(lightTransformNode);

	g_assetLibrary.storeScene("test-scene", root);
	root->accept(&ScenePrettyPrinter());
}

void init() 
{
	// Init GL Debug
	cgl::initDebugAll();

	// Init assets
	initAssetsFromCOLLADA();
	initAssetsFromCode();	

	// Set background color to gray
	glClearColor(0.7f, 0.7f, 0.4f, 0);
}

void display(void) 
{
	glEnable(GL_DEPTH_TEST);

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		

	// Apply shaders
	drawScene();

	cgl::dumpColorBuffer("out.raw", g_width, g_height);
	
	// Swap double buffer
	glutSwapBuffers();
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
