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
	
struct Uniforms
{
	GLuint modelViewMatrix;
	GLuint projectionMatrix;
} g_unifroms;

struct Attribs
{
	GLuint position;
	GLuint color;
} g_attribs;

cgl::Program g_program;
int	g_height, g_width;

cgl::AssetLibrary assetLibrary;

class GLRendererVisitor : public cgl::ssg::IVisitor
{
public:

	GLRendererVisitor(cgl::Camera* camera, glm::mat4 cameraTransformation) : _modelView(1) 
	{
		_modelView = cameraTransformation;
		_projection = camera->getMatrix();
	}

	virtual void visit(cgl::ssg::SceneGraphRoot* scene)
	{
		g_program.use();
		scene->acceptChildren(this);
		g_program.useDefault();
	}

	virtual void visit(cgl::ssg::TransformationNode* transformation)
	{
		glm::mat4 temp = _modelView;

		_modelView *= transformation->getMatrix();
		glProgramUniformMatrix4fv(g_program.getId(), g_unifroms.modelViewMatrix, 1, false, glm::value_ptr(_modelView));
		glProgramUniformMatrix4fv(g_program.getId(), g_unifroms.projectionMatrix, 1, false, glm::value_ptr(_projection));	
		transformation->acceptChildren(this);

		_modelView = temp;
	}

	virtual void visit(cgl::ssg::GeomertyInstanceNode* mesh)
	{		
		cgl::SimpleMesh* simpleMesh = mesh->getMesh();
		std::vector<GLuint> attribLocs;
		attribLocs.push_back(g_attribs.position);
		attribLocs.push_back(g_attribs.color);
		simpleMesh->setAttribLocs(attribLocs);
		simpleMesh->render();
	}

protected:
	glm::mat4 _modelView, _projection;
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

cgl::SimpleMesh* loadRGBCube()
{
	cgl::SimpleMesh* RGBCube = new cgl::SimpleMesh();

	float vertices[][3] = {
		{-1,-1,-1}, //0
		{-1,-1,+1}, //1
		{-1,+1,-1}, //2
		{-1,+1,+1}, //3
		{+1,-1,-1}, //4
		{+1,-1,+1}, //5
		{+1,+1,-1}, //6
		{+1,+1,+1}};//7

	float colors[][3] = {
		{0,0,0}, //0
		{0,0,1}, //1
		{0,1,0}, //2
		{0,1,1}, //3
		{1,0,0}, //4
		{1,0,1}, //5
		{1,1,0}, //6
		{1,1,1}};//7
	
	GLuint faces[] = {
		0,4,5,1,
		0,1,3,2,
		0,2,6,4,
		7,6,2,3,
		7,5,4,6,
		7,3,1,5};

	std::vector<GLuint> facesVec;
	facesVec.assign(faces, faces+sizeof(faces)/sizeof(GLuint));
	RGBCube->init(facesVec);
	RGBCube->
		addAttrib("position", 3, vertices, sizeof(vertices))->
		addAttrib("color", 3, colors, sizeof(colors));
	
	std::vector<GLuint> attribLocs;
	attribLocs.push_back(g_attribs.position);
	attribLocs.push_back(g_attribs.color);
	RGBCube->setAttribLocs(attribLocs);

	// Must do it here because using local buffers 
	RGBCube->initBuffers();

	return RGBCube;
}

void drawScene()
{
	cgl::ssg::SceneGraphRoot* scene = assetLibrary.getScene("test-scene");

	FindCameraVisitor query("test-camera-instance");
	scene->accept(&query);

	GLRendererVisitor renderer(query.getCamera(), query.getCameraTransformation());
	scene->accept(&renderer);
}

void initShaders() 
{
	std::vector<cgl::Shader> shaders;
	shaders.push_back(cgl::Shader::fromFile(GL_VERTEX_SHADER,"../res/shader/fixed_function_simple.vert"));		
	shaders.push_back(cgl::Shader::fromFile(GL_FRAGMENT_SHADER,"../res/shader/fixed_function_simple.frag"));		
	g_program.build(shaders);	

	g_unifroms.modelViewMatrix = glGetUniformLocation(g_program.getId(), "uModelViewMatrix");
	g_unifroms.projectionMatrix = glGetUniformLocation(g_program.getId(), "uProjectionMatrix");	

	g_attribs.position = glGetAttribLocation(g_program.getId(), "aPosition");
	g_attribs.color = glGetAttribLocation(g_program.getId(), "aColor");
}

void initAssetsFromCOLLADA()
{
	cgl::importCollada("D:/Chen/Projects/2013/Scenegraph/blender_simple.dae", assetLibrary);
	{
		// Build scene graph
		cgl::ssg::SceneGraphRoot* root = new cgl::ssg::SceneGraphRoot();
		cgl::ssg::TransformationNode* cameraTransformNode = new cgl::ssg::TransformationNode(glm::translate(glm::mat4(1), glm::vec3(0,0.5,0)));
		cgl::ssg::CameraInstanceNode* cameraNode = new cgl::ssg::CameraInstanceNode(assetLibrary.getCamera("Camera-camera"),"test-camera-instance");
		cameraTransformNode->addChild(cameraNode);
		root->addChild(cameraTransformNode);

		cgl::ssg::TransformationNode* meshTransformNode = new cgl::ssg::TransformationNode(glm::translate(glm::mat4(1), glm::vec3(0,0,-5)));
		cgl::ssg::GeomertyInstanceNode* meshNode = new cgl::ssg::GeomertyInstanceNode(assetLibrary.getMesh("Cube-mesh"));
		meshTransformNode->addChild(meshNode);
		root->addChild(meshTransformNode);

		assetLibrary.storeScene("test-scene", root);
	}
}

void initAssets()
{	
	// Build library
	cgl::Camera* camera = assetLibrary.storeCamera("test-camera", new cgl::Camera(glm::perspective(60.0f,1.0f,1.0f,100.0f)));

	// Build scene graph
	cgl::ssg::SceneGraphRoot* root = new cgl::ssg::SceneGraphRoot();
	cgl::ssg::TransformationNode* cameraTransformNode = new cgl::ssg::TransformationNode(glm::translate(glm::mat4(1), glm::vec3(0,0.5,0)));
	cgl::ssg::CameraInstanceNode* cameraNode = new cgl::ssg::CameraInstanceNode(camera, "test-camera-instance");
	cameraTransformNode->addChild(cameraNode);
	root->addChild(cameraTransformNode);

	cgl::ssg::TransformationNode* meshTransformNode = new cgl::ssg::TransformationNode(glm::translate(glm::mat4(1), glm::vec3(0,0,-5)));
	cgl::ssg::GeomertyInstanceNode* meshNode = new cgl::ssg::GeomertyInstanceNode(loadRGBCube());
	meshTransformNode->addChild(meshNode);
	root->addChild(meshTransformNode);

	assetLibrary.storeScene("test-scene", root);
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

	// Init shaders
	initShaders();

	// Init assets
	//initAssets();
	initAssetsFromCOLLADA();

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
		glutInitContextVersion(4,3);
		glutInitContextProfile(GLUT_CORE_PROFILE);
		//glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
		glutInitContextFlags(GLUT_DEBUG);

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
