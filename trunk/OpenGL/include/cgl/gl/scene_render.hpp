#pragma once
#include "asset_library.hpp"

namespace cgl
{
	//
	// The Renderer visitor traverses the scene graph and sets states and renders
	// meshes in order. It relies on having the camera and lighting environment
	// provided to it. 
	//
	class GLRendererVisitor : public cgl::ssg::IVisitor
	{
	public:

		GLRendererVisitor(cgl::Camera* camera, glm::mat4 cameraTransformation, cgl::Lighting* lighting, cgl::Effect* defaultEffect) : _modelView(1) 
		{
			_modelView = cameraTransformation;
			_projection = camera->getMatrix();
			_currentEffect = defaultEffect;
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

	void renderScene(cgl::ssg::SceneGraphRoot* scene, std::string cameraName, cgl::Effect* defaultEffect)
	{
		cgl::SpatialVisitor query(cameraName);

		scene->accept(&query);
		if(!query.isCameraFound())
			throw std::exception("Camera not found!");
	
		cgl::Lighting lighting;
		lighting.setActiveLights(query.getLights());
		lighting.transformWorldView(query.getCameraTransformation());

		cgl::ssg::EffectInstanceNode* effectNode = new cgl::ssg::EffectInstanceNode(defaultEffect);
		effectNode->addChild(scene);
		cgl::ssg::GroupNode* root = effectNode;		
		cgl::GLRendererVisitor renderer(query.getCamera(), query.getCameraTransformation(), &lighting, NULL);
		root->accept(&renderer);
	}
}
