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

	class Image
	{
	};

	class Texture
	{

	};

	class Effect
	{
	public:
		virtual void updateEnvUniforms(glm::mat4& modelView, glm::mat4& projection) = 0;
		virtual std::map<std::string, GLuint> getAttribLocs() = 0;
		virtual void applyEffect() = 0;
	protected:
		
	};

	class CommonEffect : public Effect
	{
	public:
		void init()
		{
			std::vector<cgl::Shader> shaders;
			shaders.push_back(cgl::Shader::fromFile(GL_VERTEX_SHADER,"../res/shader/fixed_function_simple.vert"));		
			shaders.push_back(cgl::Shader::fromFile(GL_FRAGMENT_SHADER,"../res/shader/fixed_function_simple.frag"));		
			_program.build(shaders);	

			// Get common environment unifrom locations
			_unifroms.modelViewMatrix = glGetUniformLocation(_program.getId(), "uModelViewMatrix");
			_unifroms.projectionMatrix = glGetUniformLocation(_program.getId(), "uProjectionMatrix");	
			_attribs["VERTEX"] = glGetAttribLocation(_program.getId(), "aPosition");
			_attribs["NORMAL"] = glGetAttribLocation(_program.getId(), "aColor");
		}

		void applyEffect()
		{
			_program.use();
		}

		void updateEnvUniforms(glm::mat4& modelView, glm::mat4& projection)
		{
			glProgramUniformMatrix4fv(_program.getId(), _unifroms.modelViewMatrix, 1, false, glm::value_ptr(modelView));
			glProgramUniformMatrix4fv(_program.getId(), _unifroms.projectionMatrix, 1, false, glm::value_ptr(projection));	
		}

		std::map<std::string, GLuint> getAttribLocs()
		{
			return _attribs;
		}

	protected:

		struct Uniforms
		{
			GLuint modelViewMatrix;
			GLuint projectionMatrix;
		} _unifroms;

		std::map<std::string, GLuint> _attribs;
		cgl::Program _program;
		cgl::Texture _diffuseTexture;
		glm::vec4 _diffuseColor;
		glm::vec4 _ambientColor;
		glm::vec4 _specularColor;
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

		class EffectInstanceNode : public GroupNode
		{
		public:
			EffectInstanceNode(cgl::Effect* effect) : _effect(effect) {};
			cgl::Effect* getEffect() const { return _effect; };
			virtual void accept(IVisitor *visitor);
		protected:
			cgl::Effect* _effect;
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
			virtual void visit(EffectInstanceNode* node){node->acceptChildren(this);};
		};
	}

	class AssetLibrary
	{
	protected:
		std::map<std::string, cgl::Camera*> _cameras;
		std::map<std::string, cgl::SimpleMesh*> _meshes;
		std::map<std::string, ssg::SceneGraphRoot*> _scenes;
		std::map<std::string, cgl::Effect*> _effects;
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

		cgl::Effect* storeEffect(std::string id, cgl::Effect* effect)
		{
			_effects[id] = effect;
			return _effects[id];
		}	

		ssg::SceneGraphRoot* getScene(std::string id) { return _scenes[id]; };
		cgl::Camera* getCamera(std::string id) { return _cameras[id]; };
		cgl::SimpleMesh* getMesh(std::string id) { return _meshes[id]; };
		cgl::Effect* getEffect(std::string id) { return _effects[id]; };
	};

}

void cgl::ssg::SceneGraphRoot::accept(IVisitor *visitor){ visitor->visit(this); }
void cgl::ssg::CameraInstanceNode::accept(IVisitor *visitor){ visitor->visit(this); }
void cgl::ssg::GeomertyInstanceNode::accept(IVisitor *visitor){ visitor->visit(this); }
void cgl::ssg::TransformationNode::accept(IVisitor *visitor){ visitor->visit(this); }
void cgl::ssg::EffectInstanceNode::accept(IVisitor *visitor){ visitor->visit(this); }

void cgl::ssg::GroupNode::acceptChildren(IVisitor *visitor)
{
	std::vector<Node*>::iterator it = _children.begin();
	for(;it != _children.end(); ++it)
	{
		(*it)->accept(visitor);
	}
}

class ScenePrettyPrinter : public cgl::ssg::IVisitor
{
protected:
	int _indent;
public:

	ScenePrettyPrinter() : _indent(0)
	{
	}

	void applyIndent()
	{
		for(int i=0; i<_indent*4;++i)
		{
			std::cout << " ";
		}
	}

	virtual void visit(cgl::ssg::SceneGraphRoot* scene)
	{
		applyIndent();
		std::cout << "Scene root" << std::endl;
		_indent++;
		scene->acceptChildren(this);
		_indent--;
	}

	virtual void visit(cgl::ssg::TransformationNode* transformation)
	{
		applyIndent();
		std::cout << "Transformation node " << std::endl;
		_indent++;
		transformation->acceptChildren(this);
		_indent--;
	}

	virtual void visit(cgl::ssg::GeomertyInstanceNode* mesh)
	{		
		applyIndent();
		std::cout << "Geomerty Instance Node" << std::endl;
	}

	virtual void visit(cgl::ssg::EffectInstanceNode* effect)
	{		
		applyIndent();
		std::cout << "Effect Instance Node" << std::endl;
		_indent++;
		effect->acceptChildren(this);
		_indent--;
	}

	virtual void visit(cgl::ssg::CameraInstanceNode* camera)
	{
		applyIndent();
		std::cout << "Camera Instance Node" << std::endl;
	}
};