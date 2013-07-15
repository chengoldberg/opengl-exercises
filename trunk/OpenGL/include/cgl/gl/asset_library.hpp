#pragma once
#include <stdlib.h>
#include "cgl/gl/common.hpp"
#include <vector>
#include <map>
#include <string>
#include "glm/glm.hpp"
#include "CTargaImage.h"
#include "CTargaImage.cpp"

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
	public:
		Image(std::string filename) : _filename(filename), _data(NULL)
		{
		}

		void loadImage()
		{
			//TODO: release
			CTargaImage* img = new CTargaImage();
			if(!img->Load(_filename.c_str()))
			{				
				throw std::exception("Unable to load image");
			}		
			unsigned char format = img->GetImageFormat();
			//assert(format==1);
			_data = img->GetImage();
			_width = img->GetWidth();
			_height = img->GetHeight();
		}

		unsigned char* getImage()
		{
			// Lazy load
			if(!_data)
				loadImage();
			return _data;
		}

		int getWidth() const { return _width; };
		int getHeight() const { return _height; };

	protected:
		int _width, _height;
		unsigned char* _data;
		std::string _filename;
	};

	class Texture
	{
	public:
		virtual void bind() = 0;
	};

	class Texture2D : public Texture
	{
	public:

		void initFrom(Image* image)
		{
			_image = image;
			image->loadImage();
			glGenTextures(1, &_texId);
			bind();
			//unsigned char* data = new unsigned char[_image->getWidth()*_image->getHeight()*4];
			//std::memcpy(data, _image->getImage(), _image->getWidth()*_image->getHeight()*4);
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _image->getWidth(), _image->getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, _image->getImage());
			//glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);

			glPixelStorei(GL_UNPACK_ALIGNMENT,1);
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _image->getWidth(), _image->getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, _image->getImage());
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, _image->getWidth(), _image->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, _image->getImage());
			/*
			static GLubyte data[] = {
				255,255,255,
				128,128,128,
				64,64,64,
				0,0,0};
			glPixelStorei(GL_UNPACK_ALIGNMENT,1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			*/
		}
	
		virtual void bind()
		{
			glBindTexture(GL_TEXTURE_2D, _texId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}

	protected:
		cgl::Image* _image;
		GLuint _texId;
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
		CommonEffect() : _isInit(false), _isLazyInit(true), _diffuseTexture(NULL)
		{
		}

		void init()
		{
			std::vector<cgl::Shader> shaders;
			shaders.push_back(cgl::Shader::fromFile(GL_VERTEX_SHADER,"../res/shader/fixed_function_simple.vert"));		
			shaders.push_back(cgl::Shader::fromFile(GL_FRAGMENT_SHADER,"../res/shader/fixed_function_simple.frag"));		
			_program.build(shaders);	

			// Get common environment unifrom locations
			_unifroms.modelViewMatrix = glGetUniformLocation(_program.getId(), "uModelViewMatrix");
			_unifroms.projectionMatrix = glGetUniformLocation(_program.getId(), "uProjectionMatrix");	
			_unifroms.textureEnabled = glGetUniformLocation(_program.getId(), "uTextureEnabled");	
			_unifroms.texture0 = glGetUniformLocation(_program.getId(), "uTexture0");	
			_attribs["VERTEX"] = glGetAttribLocation(_program.getId(), "aPosition");
			_attribs["NORMAL"] = glGetAttribLocation(_program.getId(), "aColor");
			_attribs["TEXCOORD"] = glGetAttribLocation(_program.getId(), "aTexCoord");
			_isInit = true;
		}

		void applyEffect()
		{
			if(!_isInit)
			{
				if(_isLazyInit)
					init();
				else
					throw std::exception("Can't apply effect before initializing it");
			}
			_program.use();

			//glProgramUniform1i(_program.getId(), _unifroms.textureEnabled, _diffuseTexture != NULL);			
			glProgramUniform1i(_program.getId(), _unifroms.textureEnabled, 1);			

			if(_diffuseTexture)
			{
				glProgramUniform1i(_program.getId(), _unifroms.texture0, 0);
				glActiveTexture(GL_TEXTURE0+0);
				_diffuseTexture->bind();
			}

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

		void setEmissionColor(glm::vec4 val) { _emissionColor = val; };
		void setAmbientColor(glm::vec4 val) { _ambientColor = val; };
		void setDiffuseColor(glm::vec4 val) { _diffuseColor = val; };
		void setSpecularColor(glm::vec4 val) { _specularColor = val; };
		void setShininess(float val) { _shininess = val; };
		
		void setDiffuseTexture(cgl::Texture* val) { _diffuseTexture = val; };		
	protected:

		struct Uniforms
		{
			GLuint modelViewMatrix;
			GLuint projectionMatrix;
			GLuint textureEnabled;
			GLuint texture0;
		} _unifroms;

		std::map<std::string, GLuint> _attribs;
		cgl::Program _program;

		cgl::Texture* _diffuseTexture;
		glm::vec4 _emissionColor;
		glm::vec4 _diffuseColor;
		glm::vec4 _ambientColor;
		glm::vec4 _specularColor;
		float _shininess;
		bool _isInit;
		bool _isLazyInit;
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
		std::map<std::string, cgl::Image*> _images;
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

		// Storing takes ownership on the object
		void store(std::string id, cgl::Image* image)
		{
			_images[id] = image;
		}	

		ssg::SceneGraphRoot* getScene(std::string id) { return _scenes[id]; };
		cgl::Camera* getCamera(std::string id) { return _cameras[id]; };
		cgl::SimpleMesh* getMesh(std::string id) { return _meshes[id]; };
		cgl::Effect* getEffect(std::string id) { return _effects[id]; };
		cgl::Image* getImage(std::string id) { return _images[id]; };
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