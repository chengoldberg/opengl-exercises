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
	class SplineSegment
	{
	public:
		virtual float calc(float time) = 0;

		virtual float getEndTime() const = 0;
		virtual float getEndValue() const = 0;
	};
	
	class ConstantSplineSegment : public SplineSegment
	{
	public:
		ConstantSplineSegment(float value) : _value(value) {};

		virtual float calc(float time)
		{
			return _value;
		}

		virtual float getEndTime() const
		{
			return std::numeric_limits<float>::infinity();
		}

		virtual float getEndValue() const
		{
			_value;
		}

	protected:
		float _value;
	};

	class LinearSplineSegment : public SplineSegment
	{
	public:
		LinearSplineSegment(float x0, float y0, float x1, float y1) : _x0(x0), _x1(x1), _y0(y0), _y1(y1) 
		{

		};

		virtual float calc(float time)
		{
			const float alpha = (time - _x0)/(_x1 - _x0);
			return (1-alpha)*_y0 + (alpha)*_y1;
		}

		virtual float getEndTime() const
		{
			return _x1;
		}

		virtual float getEndValue() const
		{
			return _y1;
		}

	protected:
		float _x0, _x1, _y0, _y1;
	};

	class SplineSampler
	{
	public:

		SplineSampler() : _amount(0) {};

		void addSample(float time, float value, SplineSegment* segment)
		{
			if(!_amount)
			{
				_leftmostTime = time;
				_leftmostValue = value;
			}
			else
			{
				// Remove the final mark
				_keyframes.pop_back();
			}
			//TODO: insert sorted!!!
			_keyframes.push_back(time);
			_keyframes.push_back(segment->getEndTime());
			_segments.push_back(segment);

			_rightmostTime = segment->getEndTime();
			_rightmostValue = segment->getEndValue();
			_amount++;
		}

		float calc(float time)
		{
			if(time < _leftmostTime)
				return _leftmostValue;
			for(int i=0; i<_keyframes.size()-1; ++i)
			{
				//TODO: binary search
				if(time < _keyframes[i+1])
					return _segments[i]->calc(time);
			}
			return _rightmostValue;
		}

	protected:
		std::vector<float> _keyframes;
		std::vector<SplineSegment*> _segments;
		float _leftmostTime, _leftmostValue;
		float _rightmostTime, _rightmostValue;
		int _amount;
	};

	class Animator
	{
	public:

		Animator(SplineSampler sampler, float* channel) : _sampler(sampler), _channel(channel)
		{

		}

		void update(float time)
		{
			*_channel = _sampler.calc(time);
		}
	protected:
		SplineSampler _sampler;
		float* _channel;
	};

	class AnimationSystem
	{
	protected:
		std::vector<Animator*> _animators;

	public:
		void addAnimation(Animator* animation)
		{
			_animators.push_back(animation);
		}

		void update(float time)
		{
			for(unsigned int i=0; i<_animators.size(); ++i)
			{
				_animators[i]->update(time);
			}
		}
	};

	class CommonProgram : public SpecificProgram
	{
	public:
		struct Attribs
		{
			GLuint position;
			GLuint normal;
			GLuint texcoord;
			GLuint color;
		};

		struct Uniforms
		{
			GLuint modelViewMatrix;
			GLuint normalMatrix;
			GLuint projectionMatrix;
			GLuint textureEnabled;
			GLuint lightingEnabled;
			GLuint texture0;

			struct LightUniforms
			{
				GLuint ambient;              // Aclarri   
				GLuint diffuse;              // Dcli   
				GLuint specular;             // Scli   
				GLuint position;             // Ppli   
				GLuint halfVector;           // Derived: Hi   
				GLuint spotDirection;        // Sdli   
				GLuint spotExponent;        // Srli   
				GLuint spotCutoff;          // Crli                              
											// (range: [0.0,90.0], 180.0)   
				GLuint spotCosCutoff;       // Derived: cos(Crli)                 
											// (range: [1.0,0.0],-1.0)   
				GLuint constantAttenuation; // K0   
				GLuint linearAttenuation;   // K1   
				GLuint quadraticAttenuation;// K2  
				GLuint isEnabled;   
			} lights[8];

			struct MaterialUniforms
			{
				GLuint emission;             
				GLuint ambient;              // Aclarri   
				GLuint diffuse;              // Dcli   
				GLuint specular;             
				GLuint shininess;             // Scli   
			} material;
		};

		CommonProgram() : _isInit(false) {};

		void build()
		{
			std::vector<cgl::Shader> shaders;
			shaders.push_back(cgl::Shader::fromFile(GL_VERTEX_SHADER,"../res/shader/fixed_function_simple.vert"));		
			shaders.push_back(cgl::Shader::fromFile(GL_FRAGMENT_SHADER,"../res/shader/fixed_function_simple.frag"));		
			Program::build(shaders);			
		}

		void initAttributeLocations()
		{
			GLuint programId = getId();
			_attribs.position = glGetAttribLocation(programId, "aPosition");
			_attribs.normal = glGetAttribLocation(programId, "aNormal");
			_attribs.texcoord = glGetAttribLocation(programId, "aTexCoord");
			_attribs.color = glGetAttribLocation(programId, "aColor");
		}

		std::string replaceHelper(const char* txt, int i) 
		{
			char temp[64];
			sprintf_s(temp, txt, i);
			return std::string(temp);
		}

		void initUnifromLocations()
		{
			GLuint programId = getId();
			// Get common environment unifrom locations
			_uniforms.modelViewMatrix = glGetUniformLocation(programId, "uModelViewMatrix");
			_uniforms.normalMatrix = glGetUniformLocation(programId, "uNormalMatrix");
			_uniforms.projectionMatrix = glGetUniformLocation(programId, "uProjectionMatrix");	
			_uniforms.textureEnabled = glGetUniformLocation(programId, "uTextureEnabled");	
			_uniforms.lightingEnabled = glGetUniformLocation(programId, "uLightingEnabled");	
			_uniforms.texture0 = glGetUniformLocation(programId, "uTexture0");	

			// Lighting unifroms			
			for(int i=0;i<8;++i) 
			{	    		
				_uniforms.lights[i].position = glGetUniformLocation(programId, replaceHelper("uLightSource[%d].position",i).c_str());
	    		_uniforms.lights[i].ambient = glGetUniformLocation(programId, replaceHelper("uLightSource[%d].ambient",i).c_str());
	    		_uniforms.lights[i].diffuse = glGetUniformLocation(programId, replaceHelper("uLightSource[%d].diffuse",i).c_str());
	    		_uniforms.lights[i].specular = glGetUniformLocation(programId, replaceHelper("uLightSource[%d].specular",i).c_str());
	    		_uniforms.lights[i].spotDirection = glGetUniformLocation(programId, replaceHelper("uLightSource[%d].spotDirection",i).c_str());
	    		_uniforms.lights[i].spotExponent = glGetUniformLocation(programId, replaceHelper("uLightSource[%d].spotExponent",i).c_str());  			
	    		_uniforms.lights[i].spotCutoff = glGetUniformLocation(programId, replaceHelper("uLightSource[%d].spotCutoff",i).c_str());			
	    		_uniforms.lights[i].constantAttenuation = glGetUniformLocation(programId, replaceHelper("uLightSource[%d].constantAttenuation",i).c_str());
	    		_uniforms.lights[i].linearAttenuation = glGetUniformLocation(programId, replaceHelper("uLightSource[%d].linearAttenuation",i).c_str());			
	    		_uniforms.lights[i].quadraticAttenuation = glGetUniformLocation(programId, replaceHelper("uLightSource[%d].quadraticAttenuation",i).c_str());
	    		_uniforms.lights[i].isEnabled = glGetUniformLocation(programId, replaceHelper("uLightSource[%d].isEnabled",i).c_str());
    		};

			_uniforms.material.ambient = glGetUniformLocation(programId, "uMaterial.ambient");
			_uniforms.material.diffuse = glGetUniformLocation(programId, "uMaterial.diffuse");
			_uniforms.material.specular = glGetUniformLocation(programId, "uMaterial.specular");
			_uniforms.material.shininess = glGetUniformLocation(programId, "uMaterial.shininess");
			_uniforms.material.emission = glGetUniformLocation(programId, "uMaterial.emission");	 
		}
		
		virtual void init()
		{
			if(_isInit)
				return;
			build();
			initAttributeLocations();
			initUnifromLocations();
			_isInit = true;
		}

		CommonProgram::Uniforms getUnifromLocations() const { return _uniforms; };
		CommonProgram::Attribs getAttribLocations() const { return _attribs; };

	protected:
		CommonProgram::Uniforms _uniforms;
		CommonProgram::Attribs _attribs;	
		bool _isInit;
	};

	// Library
	class Light
	{
	public:
		struct LightProperties
		{
			glm::vec4 ambient;              // Aclarri   
			glm::vec4 diffuse;              // Dcli   
			glm::vec4 specular;             // Scli   
			glm::vec4 position;             // Ppli   
			glm::vec4 halfVector;           // Derived: Hi   
			glm::vec3 spotDirection;        // Sdli   
			float spotExponent;        // Srli   
			float spotCutoff;          // Crli                              
										// (range: [0.0,90.0], 180.0)   
			float spotCosCutoff;       // Derived: cos(Crli)                 
										// (range: [1.0,0.0],-1.0)   
			float constantAttenuation; // K0   
			float linearAttenuation;   // K1   
			float quadraticAttenuation;// K2  
			bool isEnabled;   
		} _properties;

	public:
		Light() 
		{
			memset(&_properties, 0, sizeof(_properties));
			_properties.constantAttenuation = 1;
			_properties.spotDirection = glm::vec3(0,0,-1);
			_properties.spotCutoff = 180;
		}

		LightProperties& getProperties() { return _properties; };

	protected:
	};

	class Lighting
	{
	public:
		void setActiveLights(std::vector<cgl::Light*> val) { _activeLights = val; };
		
		void transformWorldView(glm::mat4 worldViewMatrix)
		{
			for(int i=0; i<_activeLights.size(); ++i)
			{
				_activeLights[i]->getProperties().position = worldViewMatrix * _activeLights[i]->getProperties().position;
			}
		}

		std::vector<cgl::Light*>& getActiveLightsGlobally() { return _activeLights; };
	protected:

		std::vector<cgl::Light*> _activeLights;
	};

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
		virtual void updateLighting(cgl::Lighting* lighting) = 0;
		virtual void updateEnvUniforms(glm::mat4& modelView, glm::mat4& projection) = 0;
		virtual std::map<std::string, GLuint> getAttribLocs() = 0;
		virtual void applyEffect() = 0;

	protected:
		
	};

	class CommonEffect : public Effect
	{
	public:
		CommonEffect(CommonProgram* program) : _isInit(false), _isLazyInit(true), _diffuseTexture(NULL), _isLighting(false), _program(program),
			_diffuseColor(glm::vec4(0,0,0,0)), _ambientColor(glm::vec4(0)), _specularColor(glm::vec4(0)), _shininess(50) {}

		void init()
		{
			// Should init only once
			_program->init();

			_attribs = _program->getAttribLocations();
			_attribsByCommonNames["VERTEX"] = _attribs.position;
			_attribsByCommonNames["NORMAL"] = _attribs.normal;
			_attribsByCommonNames["TEXCOORD"] = _attribs.texcoord;
			_attribsByCommonNames["COLOR"] = _attribs.color;
			_uniforms = _program->getUnifromLocations();
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
			_program->use();

			glProgramUniform1i(_program->getId(), _uniforms.lightingEnabled, _isLighting);			
			glProgramUniform1i(_program->getId(), _uniforms.textureEnabled, _diffuseTexture != NULL);			

			if(_diffuseTexture)
			{
				glProgramUniform1i(_program->getId(), _uniforms.texture0, 0);
				glActiveTexture(GL_TEXTURE0+0);
				_diffuseTexture->bind();
			}
		}

		virtual void updateLighting(cgl::Lighting* lighting)
		{
			int cnt = 0;
			std::vector<cgl::Light*> activeLights = lighting->getActiveLightsGlobally();
			for(std::vector<cgl::Light*>::iterator light = activeLights.begin();
				light != activeLights.end();
				light++, cnt++)
			{
				cgl::Light::LightProperties& prop = (*light)->getProperties();
				glUniform1i(_uniforms.lights[cnt].isEnabled, true);
				glUniform4fv(_uniforms.lights[cnt].position, 1, glm::value_ptr(prop.position));
				glUniform4fv(_uniforms.lights[cnt].ambient, 1, glm::value_ptr(prop.ambient));
				glUniform4fv(_uniforms.lights[cnt].diffuse, 1, glm::value_ptr(prop.diffuse));
				glUniform4fv(_uniforms.lights[cnt].specular, 1, glm::value_ptr(prop.specular));
				glUniform1f(_uniforms.lights[cnt].constantAttenuation, prop.constantAttenuation);
				glUniform1f(_uniforms.lights[cnt].linearAttenuation, prop.linearAttenuation);
				glUniform1f(_uniforms.lights[cnt].quadraticAttenuation, prop.quadraticAttenuation);

				glUniform3fv(_uniforms.lights[cnt].spotDirection, 1, glm::value_ptr(prop.spotDirection));
				glUniform1f(_uniforms.lights[cnt].spotExponent, prop.spotExponent);
				glUniform1f(_uniforms.lights[cnt].spotCutoff, prop.spotCutoff);

				glUniform4fv(_uniforms.material.emission, 1, glm::value_ptr(_emissionColor));
				glUniform4fv(_uniforms.material.ambient, 1, glm::value_ptr(_ambientColor));
				glUniform4fv(_uniforms.material.diffuse, 1, glm::value_ptr(_diffuseColor));
				glUniform4fv(_uniforms.material.specular, 1, glm::value_ptr(_specularColor));
				glUniform1f(_uniforms.material.shininess, _shininess/8);
			}
			for(; cnt<8; ++cnt)
				glUniform1i(_uniforms.lights[cnt].isEnabled, false);

		}

		void updateEnvUniforms(glm::mat4& modelView, glm::mat4& projection)
		{
			glProgramUniformMatrix4fv(_program->getId(), _uniforms.modelViewMatrix, 1, false, glm::value_ptr(modelView));
			glProgramUniformMatrix3fv(_program->getId(), _uniforms.normalMatrix, 1, false, glm::value_ptr(glm::mat3(modelView))); //TODO: inverse-transpose glm::inverseTranspose
			glProgramUniformMatrix4fv(_program->getId(), _uniforms.projectionMatrix, 1, false, glm::value_ptr(projection));				
		}

		std::map<std::string, GLuint> getAttribLocs()
		{
			return _attribsByCommonNames;
		}

		void setIsLighting(bool val) { _isLighting = val; };
		void setEmissionColor(glm::vec4 val) { _emissionColor = val; };
		void setAmbientColor(glm::vec4 val) { _ambientColor = val; };
		void setDiffuseColor(glm::vec4 val) { _diffuseColor = val; };
		void setSpecularColor(glm::vec4 val) { _specularColor = val; };
		void setShininess(float val) { _shininess = val; };
		
		void setDiffuseTexture(cgl::Texture* val) { _diffuseTexture = val; };		
	protected:

		cgl::CommonProgram* _program;
		CommonProgram::Uniforms _uniforms;
		CommonProgram::Attribs _attribs;	

		std::map<std::string, GLuint> _attribsByCommonNames;

		cgl::Texture* _diffuseTexture;
		glm::vec4 _emissionColor;
		glm::vec4 _diffuseColor;
		glm::vec4 _ambientColor;
		glm::vec4 _specularColor;
		float _shininess;
		bool _isInit;
		bool _isLazyInit;
		bool _isLighting;
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
			virtual glm::mat4 getMatrix() const { return _matrix; };
			virtual float* getVariable(std::string) { throw std::exception("Variable name unknown"); };
		protected:
			glm::mat4 _matrix;
		};

		class TranslationNode : public TransformationNode
		{
		public:
			TranslationNode(glm::vec3 val) : _translation(val) {};
			virtual glm::mat4 getMatrix() const { return glm::translate(glm::mat4(), _translation); };
			virtual glm::vec3* getTranslationPtr() { return &_translation; };
			virtual float* getVariable(std::string varName) 
			{ 
				if(varName == "X")
					return &_translation.x;
				else if(varName == "Y")
					return &_translation.y;
				else if(varName == "Z")
					return &_translation.z;
				else
					throw std::exception("Variable name unknown");
			};
		protected:
			glm::vec3 _translation;
		};

		class RotationNode : public TransformationNode
		{
		public:
			RotationNode(glm::vec3 axis, float angle) : _axis(axis), _angle(angle) {};
			virtual glm::mat4 getMatrix() const { return glm::rotate(glm::mat4(), _angle, _axis); };
			glm::vec3* getAxisPtr() { return &_axis; };
			float* getAnglePtr() { return &_angle; };
		protected:
			glm::vec3 _axis;
			float _angle;
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

		class LightInstanceNode : public Node
		{
		public:
			LightInstanceNode(cgl::Light* light, std::string id) : _light(light), Node(id) {};
			virtual void accept(IVisitor *visitor);
			cgl::Light* getLight() const { return _light; };
		protected:			
			cgl::Light* _light;
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
			virtual void visit(LightInstanceNode* node){};
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
		std::map<std::string, cgl::Light*> _lights;
		std::map<std::string, cgl::Program*> _programs;
	public:

		void initCommonAssets()
		{
			store("common", new cgl::CommonProgram());
			cgl::CommonEffect* effect = new cgl::CommonEffect((cgl::CommonProgram*) getProgram("common"));
			effect->setDiffuseColor(glm::vec4(1,0,0,1));
			effect->setIsLighting(true);
			store("default", effect);
		}

		AssetLibrary()
		{
			initCommonAssets();
		}

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
		void store(std::string id, cgl::Effect* effect)
		{
			_effects[id] = effect;
		}	

		void store(std::string id, cgl::Image* image)
		{
			_images[id] = image;
		}	

		void store(std::string id, cgl::Light* light)
		{
			_lights[id] = light;
		}	

		void store(std::string id, cgl::Program* program)
		{
			_programs[id] = program;
		}

		ssg::SceneGraphRoot* getScene(std::string id) { return _scenes[id]; };
		cgl::Camera* getCamera(std::string id) { return _cameras[id]; };
		cgl::SimpleMesh* getMesh(std::string id) { return _meshes[id]; };
		cgl::Effect* getEffect(std::string id) { return _effects[id]; };
		cgl::Image* getImage(std::string id) { return _images[id]; };
		cgl::Light* getLight(std::string id) { return _lights[id]; };
		cgl::Program* getProgram(std::string id) { return _programs[id]; };
	};

}

void cgl::ssg::SceneGraphRoot::accept(IVisitor *visitor){ visitor->visit(this); }
void cgl::ssg::CameraInstanceNode::accept(IVisitor *visitor){ visitor->visit(this); }
void cgl::ssg::GeomertyInstanceNode::accept(IVisitor *visitor){ visitor->visit(this); }
void cgl::ssg::TransformationNode::accept(IVisitor *visitor){ visitor->visit(this); }
void cgl::ssg::EffectInstanceNode::accept(IVisitor *visitor){ visitor->visit(this); }
void cgl::ssg::LightInstanceNode::accept(IVisitor *visitor){ visitor->visit(this); }

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

	virtual void visit(cgl::ssg::LightInstanceNode* light)
	{		
		applyIndent();
		std::cout << "Light Instance Node" << std::endl;
	}
};