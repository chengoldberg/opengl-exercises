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
	

int	g_height, g_width;

cgl::AssetLibrary assetLibrary;

class GLRendererVisitor : public cgl::ssg::IVisitor
{
public:

	GLRendererVisitor(cgl::Camera* camera, glm::mat4 cameraTransformation) : _modelView(1) 
	{
		_modelView = cameraTransformation;
		_projection = camera->getMatrix();
		_currentEffect = NULL;
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
};

class FindCameraVisitor : public cgl::ssg::IVisitor
{
protected:
	cgl::Camera* _foundCamera;
	glm::mat4 _cameraTransformation;
	std::string _queriedId;
	glm::mat4 _modelWorld;

public:
	FindCameraVisitor(std::string queriedId) : _queriedId(queriedId), _foundCamera(NULL) {};

	bool isCameraFound() { return _foundCamera != NULL; };

	cgl::Camera* getCamera() 
	{
		return _foundCamera;
	}

	glm::mat4 getCameraTransformation() const 
	{
		return glm::inverse(_cameraTransformation);
	}

	virtual void visit(cgl::ssg::TransformationNode* transformation)
	{
		glm::mat4 temp = _modelWorld;
		_modelWorld *= transformation->getMatrix();
		transformation->acceptChildren(this);
		_modelWorld = temp;
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
	cgl::ssg::SceneGraphRoot* scene = assetLibrary.getScene("Scene");
	//cgl::ssg::SceneGraphRoot* scene = assetLibrary.getScene("test-scene");
	
	FindCameraVisitor query("Camera");
	//FindCameraVisitor query("test-camera-instance");

	scene->accept(&query);
	if(!query.isCameraFound())
		throw std::exception("Camera not found!");
	GLRendererVisitor renderer(query.getCamera(), query.getCameraTransformation());
	scene->accept(&renderer);
}

void initAssetsFromCOLLADA()
{
	//cgl::importCollada("D:/Chen/Projects/2013/Scenegraph/blender_simple_mat1.dae", assetLibrary);
	cgl::importCollada("D:/Chen/Projects/2013/Scenegraph/blender_test3.dae", assetLibrary);
	
	//cgl::importCollada("D:/Chen/Projects/2013/Scenegraph/blender_simple_tex3.dae", assetLibrary);
	cgl::ssg::SceneGraphRoot* scene = assetLibrary.getScene("Scene");
	scene->accept(&ScenePrettyPrinter());
}

void initAssets()
{	
	// Build library
	cgl::Camera* camera = assetLibrary.storeCamera("test-camera", new cgl::Camera(glm::perspective(60.0f,1.0f,1.0f,100.0f)));

	cgl::CommonEffect* effect = new cgl::CommonEffect();
	effect->init();

	assetLibrary.storeEffect("effect", effect);

	// Build scene graph
	cgl::ssg::SceneGraphRoot* root = new cgl::ssg::SceneGraphRoot();
	cgl::ssg::TransformationNode* cameraTransformNode = new cgl::ssg::TransformationNode(glm::translate(glm::mat4(1), glm::vec3(0,0.5,0)));
	cgl::ssg::CameraInstanceNode* cameraNode = new cgl::ssg::CameraInstanceNode(camera, "test-camera-instance");
	cameraTransformNode->addChild(cameraNode);
	root->addChild(cameraTransformNode);

	cgl::ssg::TransformationNode* meshTransformNode = new cgl::ssg::TransformationNode(glm::translate(glm::mat4(1), glm::vec3(0,0,-5)));
	cgl::ssg::GeomertyInstanceNode* meshNode = new cgl::ssg::GeomertyInstanceNode(assetLibrary.getMesh("Suzanne-mesh"));
	cgl::ssg::EffectInstanceNode* effectNode = new cgl::ssg::EffectInstanceNode(assetLibrary.getEffect("effect"));	
	effectNode->addChild(meshNode);
	meshTransformNode->addChild(effectNode);
	root->addChild(meshTransformNode);	

	assetLibrary.storeScene("test-scene", root);
	root->accept(&ScenePrettyPrinter());
}

void initDebug()
{
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	glDebugMessageCallback(&debugOutput, NULL);
}

void init() 
{
	// Init GL Debug
	initDebug();

	// Init assets
	initAssetsFromCOLLADA();
	//initAssets();	

	// Set background color to gray
	glClearColor(0.5f, 0.5f, 0.5f, 0);
}

void display(void) 
{
	glEnable(GL_DEPTH_TEST);

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		

	// Apply shaders
	drawScene();

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) 
{
	g_width = width;
	g_height = height;
	glViewport(0,0,width,height);
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

int main(int argc, char **argv) 
{
	try
	{
		//glutInitContextVersion(4,3);
		//glutInitContextProfile(GLUT_CORE_PROFILE);
		//glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
		//glutInitContextFlags(GLUT_DEBUG);

		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
		glutInitWindowPosition(0, 0);
		glutInitWindowSize(528, 528);

		glutCreateWindow("ex17 - Asset management");

		glutReshapeFunc(reshape);
		glutDisplayFunc(display);
		glutIdleFunc(display);
		glutKeyboardFunc(keyboardFunc);
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
