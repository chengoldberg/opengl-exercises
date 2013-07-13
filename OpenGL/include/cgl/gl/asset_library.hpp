#pragma once
#include <stdlib.h>
#include "cgl/gl/common.hpp"
#include <vector>
#include <map>
#include <string>
#include "glm/glm.hpp"

namespace cgl
{
	// Library

	class Camera
	{
	public:
		Camera(glm::mat4 matrix) : _matrix(matrix)
		{

		}

		glm::mat4 getMatrix() const { return _matrix; };
	protected:
		glm::mat4 _matrix;		
	};

	// Simple-Scene-Graph
	namespace ssg
	{
		class IVisitor;

		class Node
		{
		public:
			Node(){};
			Node(std::string id) : _id(id) {};
			virtual void accept(IVisitor *visitor) = 0;

			std::string getId() const { return _id; };
			void setParent(Node* parent) { _parent = parent; };
		protected:
			Node* _parent;
			std::string _id;
		};

		class GroupNode : public Node
		{
		public:			
			virtual void accept(IVisitor *visitor) = 0; 
			virtual void acceptChildren(IVisitor *visitor); 
			void addChild(Node* child)
			{
				child->setParent(this);
				_children.push_back(child);
			};
		protected:
			Node* _parent;
			std::vector<Node*> _children;
		};

		class TransformationNode : public GroupNode 
		{
		public:
			TransformationNode() : _matrix(1) {};
			TransformationNode(glm::mat4 matrix) : _matrix(matrix) {};
			virtual void accept(IVisitor *visitor);
			glm::mat4 getMatrix() const { return _matrix; };
		protected:
			glm::mat4 _matrix;
		};

		class GeomertyInstanceNode : public Node 
		{
		public:
			GeomertyInstanceNode(cgl::SimpleMesh* mesh) : _mesh(mesh) {};
			cgl::SimpleMesh* getMesh() const { return _mesh; };
			virtual void accept(IVisitor *visitor);
		protected:
			cgl::SimpleMesh* _mesh;
		};

		class CameraInstanceNode : public Node
		{
		public:
			CameraInstanceNode(cgl::Camera* camera, std::string id) : _camera(camera), Node(id) {};
			virtual void accept(IVisitor *visitor);
			cgl::Camera* getCamera() const { return _camera; };
		protected:			
			cgl::Camera* _camera;
		};

		class SceneGraphRoot : public GroupNode
		{		
		public:
			virtual void accept(IVisitor *visitor);
		};

		class IVisitor
		{
		public:
			virtual void visit(Node*){};
			virtual void visit(GroupNode* node){node->acceptChildren(this);};
			
			virtual void visit(TransformationNode* node){node->acceptChildren(this);};
			virtual void visit(SceneGraphRoot* node){node->acceptChildren(this);};
			virtual void visit(GeomertyInstanceNode*){};
			virtual void visit(CameraInstanceNode*){};
		};
	}

	class AssetLibrary
	{
	protected:
		std::map<std::string, cgl::Camera*> _cameras;
		std::map<std::string, cgl::SimpleMesh*> _meshes;
		std::map<std::string, ssg::SceneGraphRoot*> _scenes;
	public:

		Camera* storeCamera(std::string id, cgl::Camera* camera) 
		{ 
			_cameras[id] = camera;
			return _cameras[id];
		};

		ssg::SceneGraphRoot* storeScene(std::string id, ssg::SceneGraphRoot* scene)
		{
			_scenes[id] = scene;
			return _scenes[id];
		}

		cgl::SimpleMesh* storeMesh(std::string id, cgl::SimpleMesh* mesh)
		{
			_meshes[id] = mesh;
			return _meshes[id];
		}

		ssg::SceneGraphRoot* getScene(std::string id) { return _scenes[id]; };
		cgl::Camera* getCamera(std::string id) { return _cameras[id]; };
		cgl::SimpleMesh* getMesh(std::string id) { return _meshes[id]; };
	};

}

void cgl::ssg::SceneGraphRoot::accept(IVisitor *visitor){ visitor->visit(this); }
void cgl::ssg::CameraInstanceNode::accept(IVisitor *visitor){ visitor->visit(this); }
void cgl::ssg::GeomertyInstanceNode::accept(IVisitor *visitor){ visitor->visit(this); }
void cgl::ssg::TransformationNode::accept(IVisitor *visitor){ visitor->visit(this); }

void cgl::ssg::GroupNode::acceptChildren(IVisitor *visitor)
{
	std::vector<Node*>::iterator it = _children.begin();
	for(;it != _children.end(); ++it)
	{
		(*it)->accept(visitor);
	}
}



