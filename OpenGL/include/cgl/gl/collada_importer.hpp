#pragma once
#include <stdlib.h>
#include "cgl/gl/common.hpp"
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <stack>

#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "asset_library.hpp"

#include "tinyxml2\tinyxml2.h"
#include "tinyxml2\tinyxml2.cpp"

void parseFloatArray(const char* line, int count, float* data)
{
	std::stringstream ss(line);
	for(int i=0; i<count; ++i)
	{
		ss >> data[i];
	}		
}

namespace cgl
{
	std::map<std::string, std::string> materialToEffectMap;
	std::string _pathbase;
	std::map<std::string, cgl::ssg::Node*> _idsMap;

	void parseCameras(cgl::AssetLibrary& assetLibrary, tinyxml2::XMLElement* base)
	{
		tinyxml2::XMLElement* cameraNode = base->FirstChildElement();
		do
		{
			std::string id = cameraNode->Attribute("id");
			tinyxml2::XMLElement* commonProjectionNode = tinyxml2::XMLHandle(cameraNode).FirstChildElement("optics").FirstChildElement("technique_common").ToElement();
			tinyxml2::XMLElement* projectionNode;
			if(projectionNode = commonProjectionNode->FirstChildElement("perspective"))
			{
				float xfov, aspect_ratio, znear, zfar;
				projectionNode->FirstChildElement("xfov")->QueryFloatText(&xfov);
				projectionNode->FirstChildElement("aspect_ratio")->QueryFloatText(&aspect_ratio);
				projectionNode->FirstChildElement("znear")->QueryFloatText(&znear);
				projectionNode->FirstChildElement("zfar")->QueryFloatText(&zfar);
				//aspect_ratio = 1;// TODO
				assetLibrary.storeCamera(id, new cgl::Camera(glm::perspective(xfov, aspect_ratio, znear, zfar)));
			}
			else if(projectionNode = commonProjectionNode->FirstChildElement("orthographic"))
			{
				// TODO
			}
		}
		while(cameraNode = cameraNode->NextSiblingElement("camera"));
	}

	class COLLADASource
	{
		void* _data;
		std::string _id;
		int _elementSize;
		int _count;

		enum ArrayNames
		{
			LINEAR,
			BEZIER,
		};

	public:
		COLLADASource()
		{
		}

		COLLADASource(tinyxml2::XMLElement* base)
		{
			_id = base->Attribute("id");
			
			if(tinyxml2::XMLElement* arrayNode = base->FirstChildElement("float_array"))
			{
				int count = arrayNode->IntAttribute("count");
				_data = new float[count];
				parseFloatArray(arrayNode->GetText(), count, (float*) _data);
				_count = count;				
			} 
			else if(tinyxml2::XMLElement* arrayNode = base->FirstChildElement("Name_array"))
			{
				int count = arrayNode->IntAttribute("count");
				std::map<std::string, ArrayNames> arrayNameMap;
				arrayNameMap["LINEAR"] = ArrayNames::LINEAR;
				arrayNameMap["BEZIER"] = ArrayNames::BEZIER;

				ArrayNames* data = new ArrayNames[count];
				std::string temp;
				std::stringstream ss(arrayNode->GetText());
				for(int i=0; i<count; ++i)
				{
					ss >> temp;
					data[i] = arrayNameMap[temp];
				}		
				_data = (ArrayNames*) data;
				_count = count;
			}
			_elementSize = base->FirstChildElement("technique_common")->FirstChildElement("accessor")->IntAttribute("stride");
		}

		void dereference(std::vector<int> indices, std::vector<float>& result)	
		{
			float* data = (float*) _data;
			result.resize(indices.size()*_elementSize);
			int cnt=0;
			for(unsigned int i=0; i<indices.size(); ++i)
			{
				for(int j=0; j<_elementSize; ++j)
				{
					result[cnt++] = data[indices[i]*_elementSize+j];				
				}
			}
		}

		float* getDataFloat() const { return (float*) _data; };
		ArrayNames* getDataName() const { return (ArrayNames*) _data; };

		int getElementCount() { return _count/_elementSize; };
		int getElementSize() { return _elementSize; }; 
	};

	struct COLLADAPolylistInput
	{
		COLLADASource* source;
		std::string semantic;
		int offset;
		std::vector<int> indices;		
	};

	void parseSources(tinyxml2::XMLElement* meshNode, std::map<std::string, COLLADASource>& sources)
	{	
		for(tinyxml2::XMLElement* sourceNode = meshNode->FirstChildElement("source"); 
			sourceNode; 
			sourceNode = sourceNode->NextSiblingElement("source"))
		{
			std::string id = sourceNode->Attribute("id");
			sources[id] = COLLADASource(sourceNode);
		}	
	}

	void parseMeshSources(cgl::AssetLibrary& assetLibrary, tinyxml2::XMLElement* meshNode, std::map<std::string, COLLADASource>& sources)
	{	
		for(tinyxml2::XMLElement* sourceNode = meshNode->FirstChildElement("source"); 
			sourceNode; 
			sourceNode = sourceNode->NextSiblingElement("source"))
		{
			std::string id = sourceNode->Attribute("id");
			sources[id] = COLLADASource(sourceNode);
		}	

		tinyxml2::XMLElement* verticesNode = meshNode->FirstChildElement("vertices");
		std::string verticesId = verticesNode->Attribute("id");
		std::string sourceId = verticesNode->FirstChildElement("input")->Attribute("source")+1;
		sources[verticesId] = sources[sourceId];
	}

	void parseMeshPolylist(cgl::AssetLibrary& assetLibrary, tinyxml2::XMLElement* polygonsNode, std::map<std::string, COLLADASource>& sources, std::vector<COLLADAPolylistInput>& inputs)
	{
		int count = polygonsNode->IntAttribute("count");
		int* vcount = new int[count];
		{
			std::stringstream ss(polygonsNode->FirstChildElement("vcount")->GetText());			
			for(int i=0; i<count; ++i)
			{
				ss >> vcount[i];
			}
		}

		for(int i=0; i<count; ++i)
		{
			if(vcount[i] != 3)
				throw std::exception("Support only triangles");
		}
				
		for(tinyxml2::XMLElement* inputNode = polygonsNode->FirstChildElement("input"); 
			inputNode; 
			inputNode = inputNode->NextSiblingElement("input"))
		{
			COLLADAPolylistInput input;
			input.source = &sources[inputNode->Attribute("source")+1];
			input.offset = inputNode->IntAttribute("offset");
			input.semantic = inputNode->Attribute("semantic");
			input.indices.resize(count*3);
			inputs.push_back(input);
		}
		int inputsNum = inputs.size();

		{
			std::stringstream ss(polygonsNode->FirstChildElement("p")->GetText());
			for(int i=0; i<count*3*inputsNum; ++i)
			{
				const int inputIndex = i % inputsNum;
				const int index = i / inputsNum;
				ss >> inputs[inputIndex].indices[index];
			}
		}
	}

	void parseGeometries(cgl::AssetLibrary& assetLibrary, tinyxml2::XMLElement* base)
	{
		std::map<std::string, COLLADASource> sources;		

		for(tinyxml2::XMLElement* geometryNode = base->FirstChildElement("geometry"); 
			geometryNode; 
			geometryNode = geometryNode->NextSiblingElement("geometry"))
		{
			std::string id = geometryNode->Attribute("id");
			tinyxml2::XMLElement* meshNode = geometryNode->LastChildElement("mesh");

			// Obtain the raw source data
			parseMeshSources(assetLibrary, meshNode, sources);
		
			// Extract attribute semantics and faces
			std::vector<COLLADAPolylistInput> inputs;
			tinyxml2::XMLElement* polygonsNode = meshNode->FirstChildElement("polylist");
			parseMeshPolylist(assetLibrary, polygonsNode, sources, inputs);

			// Create simplemesh 
			cgl::SimpleMesh* simpleMesh = new cgl::SimpleMesh();
			for(std::vector<COLLADAPolylistInput>::iterator input = inputs.begin(); input != inputs.end(); ++input)
			{
				// For now we use the sequenced version of the mesh, so we dereference it
				std::vector<float>* sequencedInput = new std::vector<float>();
				input->source->dereference(input->indices, *sequencedInput);
				simpleMesh->addAttrib(input->semantic, input->source->getElementSize(), &sequencedInput->at(0), sequencedInput->size()*sizeof(float));
			}
			assetLibrary.storeMesh(id, simpleMesh);
		}
	}

	class COLLADAXMLVisitor : public tinyxml2::XMLVisitor
	{
	protected:
		cgl::AssetLibrary _assetLibrary;
		cgl::ssg::SceneGraphRoot* _root;
		cgl::ssg::GroupNode* _curNode;
		std::stack<cgl::ssg::GroupNode*> _nodeStack;
		std::stack<std::string> _nodeIdStack;
		std::map<std::string, cgl::ssg::Node*> _idMap;
	public:
		std::map<std::string, int> _names;
		enum 
		{
			INVALID,
			NODE,
			MATRIX,
			TRANSLATE,
			ROTATE,
			INSTANCE_CAMERA,
			INSTANCE_LIGHT,
			INSTANCE_GEOMETRY,
		};

		COLLADAXMLVisitor(cgl::AssetLibrary assetLibrary) : _assetLibrary(assetLibrary)
		{
			_root = new cgl::ssg::SceneGraphRoot();
			_curNode = _root;
			_names["node"] = NODE;
			_names["matrix"] = MATRIX;
			_names["translate"] = TRANSLATE;
			_names["rotate"] = ROTATE;
			_names["instance_camera"] = INSTANCE_CAMERA;
			_names["instance_geometry"] = INSTANCE_GEOMETRY;
			_names["instance_light"] = INSTANCE_LIGHT;
		}

		cgl::ssg::SceneGraphRoot* getSceneGraphRoot() const { return _root; };
		int getElementNameId(const char* name)
		{
			return _names[name];
		}
		cgl::ssg::Node* getNodeByIds(std::string ids) { return _idMap[ids]; } 
		std::map<std::string, cgl::ssg::Node*> getIdsMap() const { return _idMap; };

		/// Visit an element.
		virtual bool VisitEnter( const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* firstAttribute )
		{
			std::string idPath;
			if(const char* sid = element.Attribute("sid"))
			{
				idPath = _nodeIdStack.top() + "/" + std::string(sid);
			}

			switch(getElementNameId(element.Name()))
			{
			case NODE:
				{
					_nodeStack.push(_curNode);
					_nodeIdStack.push(element.Attribute("id"));
				}
				break;

			case MATRIX:
				{					
					float data[16];
					parseFloatArray(element.GetText(), 16, data);
					cgl::ssg::TransformationNode* transformNode = new cgl::ssg::TransformationNode(glm::transpose(glm::make_mat4(data)));
					_curNode->addChild(transformNode);
					_curNode = transformNode;
				}
				break;

			case TRANSLATE:
				{
					float data[3];
					parseFloatArray(element.GetText(), 3, data);
					cgl::ssg::TransformationNode* transformNode = new cgl::ssg::TranslationNode(glm::make_vec3(data));
					_curNode->addChild(transformNode);
					_curNode = transformNode;
					_idMap[idPath] = transformNode;
				}
				break;

			case ROTATE:
				{
					float data[4];
					parseFloatArray(element.GetText(), 4, data);
					cgl::ssg::RotationNode* rotationNode = new cgl::ssg::RotationNode(glm::make_vec3(data), data[3]);
					_curNode->addChild(rotationNode);
					_curNode = rotationNode;
					_idMap[idPath] = rotationNode;
				}
				break;

			case INSTANCE_CAMERA:
				{
					cgl::Camera* camera = _assetLibrary.getCamera(element.Attribute("url")+1);
					cgl::ssg::CameraInstanceNode* cameraNode = new cgl::ssg::CameraInstanceNode(camera, _nodeIdStack.top());
					_curNode->addChild(cameraNode);
				}
				break;

			case INSTANCE_LIGHT:
				{
					cgl::Light* light = _assetLibrary.getLight(element.Attribute("url")+1);
					if(!light)
					{
						std::cerr << "Can't create light instance" << std::endl;
						//throw std::exception("Can't create light instance");
					}
					else
					{
						cgl::ssg::LightInstanceNode* lightNode = new cgl::ssg::LightInstanceNode(light, _nodeIdStack.top());
						_curNode->addChild(lightNode);
					}
				}
				break;

			case INSTANCE_GEOMETRY:
				{
					if(element.FirstChildElement("bind_material"))
					{
						// Note that we currently only support a single material per mesh, so
						// there is no need to "bind material symbols".

						std::string materialId = element.FirstChildElement("bind_material")->FirstChildElement("technique_common")->FirstChildElement("instance_material")->Attribute("target")+1;
						std::string effectId = materialToEffectMap[materialId];
						cgl::Effect* effect = _assetLibrary.getEffect(effectId);
						cgl::ssg::EffectInstanceNode* effectInstance = new cgl::ssg::EffectInstanceNode(effect);
						_curNode->addChild(effectInstance);
						_curNode = effectInstance;
					}

					cgl::SimpleMesh* simpleMesh = _assetLibrary.getMesh(element.Attribute("url")+1);
					cgl::ssg::GeomertyInstanceNode* meshNode = new cgl::ssg::GeomertyInstanceNode(simpleMesh/*, _nodeIdStack.top()*/);
					_curNode->addChild(meshNode);
					return false;
				}
				break;
			}

			return true;
		}

		/// Visit an element.
		virtual bool VisitExit( const tinyxml2::XMLElement& element )			{
			switch(getElementNameId(element.Name()))
			{
			case NODE:
				{
					_curNode = _nodeStack.top();
					_nodeStack.pop();
					_nodeIdStack.pop();
				}
				break;
			}

			return true;
		}
	};

	void parseVisualScenes(cgl::AssetLibrary& assetLibrary, tinyxml2::XMLElement* base)
	{
		for(tinyxml2::XMLElement* visualSceneNode = base->FirstChildElement("visual_scene"); 
			visualSceneNode; 
			visualSceneNode = visualSceneNode->NextSiblingElement("visual_scene"))
		{
			std::string id = visualSceneNode->Attribute("id");

			COLLADAXMLVisitor visitor(assetLibrary);
			visualSceneNode->Accept(&visitor);
			_idsMap = visitor.getIdsMap();
			assetLibrary.storeScene(id, visitor.getSceneGraphRoot());
		}
	}

	typedef enum
	{
		FOUND_COLOR,
		FOUND_TEXTURE
	} EColorOrTexture;

	EColorOrTexture parseColorOrTexture(tinyxml2::XMLElement* node, glm::vec4* color, std::string* textureName)
	{
		if(tinyxml2::XMLElement* colorNode = node->FirstChildElement("color"))
		{
			float buffer[4];
			parseFloatArray(colorNode->GetText(), 4, buffer);
			*color = glm::make_vec4(buffer);
			return EColorOrTexture::FOUND_COLOR;
		} 
		else if(tinyxml2::XMLElement* textureNode = node->FirstChildElement("texture"))
		{
			*textureName = textureNode->Attribute("texture");
			return EColorOrTexture::FOUND_TEXTURE;
		}
		throw std::exception("Didn't find either color nor texture!");
	}

	cgl::Effect* parseLambertEffect(cgl::AssetLibrary& assetLibrary, tinyxml2::XMLElement* lambert, std::map<std::string, cgl::Texture*>& samplers)
	{		
		cgl::CommonEffect* effect = new cgl::CommonEffect((cgl::CommonProgram*) assetLibrary.getProgram("common"));
		std::string textureName;
		glm::vec4 color;

		if(parseColorOrTexture(lambert->FirstChildElement("emission"), &color, &textureName) == FOUND_COLOR)
			effect->setEmissionColor(color);
		if(parseColorOrTexture(lambert->FirstChildElement("ambient"), &color, &textureName) == FOUND_COLOR)
			effect->setAmbientColor(color);
		if(parseColorOrTexture(lambert->FirstChildElement("diffuse"), &color, &textureName) == FOUND_COLOR)
		{
			effect->setDiffuseColor(color);
		}
		else
		{
			effect->setDiffuseTexture(samplers[textureName]);
			effect->setDiffuseColor(glm::vec4(0.8,0.8,0.8,1));
		}
		effect->setIsLighting(true);
		return effect;
	}

	cgl::Effect* parsePhongEffect(cgl::AssetLibrary& assetLibrary, tinyxml2::XMLElement* phong, std::map<std::string, cgl::Texture*>& samplers)
	{		
		cgl::CommonEffect* effect = (cgl::CommonEffect*) parseLambertEffect(assetLibrary, phong, samplers);
		std::string textureName;
		glm::vec4 color;

		if(parseColorOrTexture(phong->FirstChildElement("specular"), &color, &textureName) == FOUND_COLOR)
			effect->setSpecularColor(color);
		effect->setShininess((float)std::atof(phong->FirstChildElement("shininess")->FirstChildElement("float")->GetText()));

		return effect;
	}

	void parseEffects(cgl::AssetLibrary& assetLibrary, tinyxml2::XMLElement* base)
	{
		if(!base)
			return;
		for(tinyxml2::XMLElement* effectNode = base->FirstChildElement("effect"); 
			effectNode; 
			effectNode = effectNode->NextSiblingElement("effect"))
		{
			std::string id = effectNode->Attribute("id");

			tinyxml2::XMLElement* commonProfileNode = effectNode->FirstChildElement("profile_COMMON");
			// In our case, the Texture object contains both surface and sampler
			std::map<std::string, cgl::Texture*> surfaces;
			std::map<std::string, cgl::Texture*> samplers;
			for(tinyxml2::XMLElement* newparamNode = commonProfileNode->FirstChildElement("newparam"); 
				newparamNode; 
				newparamNode = newparamNode->NextSiblingElement("newparam"))
			{
				std::string newparamId = newparamNode->Attribute("sid");

				if(tinyxml2::XMLElement* surfaceNode = newparamNode->FirstChildElement("surface"))
				{
					Texture2D* tex2d = new Texture2D();
					std::string imageId = surfaceNode->FirstChildElement("init_from")->GetText();
					tex2d->initFrom(assetLibrary.getImage(imageId));
					surfaces[newparamId] = tex2d;
				}
				else if(tinyxml2::XMLElement* samplerNode = newparamNode->FirstChildElement("sampler2D"))
				{
					samplers[newparamId] = surfaces[samplerNode->FirstChildElement("source")->GetText()];
				}
			}

			cgl::Effect* effect;
			if(tinyxml2::XMLElement* phong = commonProfileNode->FirstChildElement("technique")->FirstChildElement("phong"))
				effect = parsePhongEffect(assetLibrary, phong, samplers);
			else if(tinyxml2::XMLElement* lambert = commonProfileNode->FirstChildElement("technique")->FirstChildElement("lambert"))
				effect = parseLambertEffect(assetLibrary, lambert, samplers);
			else
				throw std::exception("Unidentified effect!");
			assetLibrary.storeEffect(id, effect);
		}
	}

	cgl::Light* parsePointLight(cgl::AssetLibrary& assetLibrary, tinyxml2::XMLElement* pointNode)
	{
		cgl::Light* light = new cgl::Light();
		cgl::Light::LightProperties& prop = light->getProperties();

		float buffer[3];
		parseFloatArray(pointNode->FirstChildElement("color")->GetText(), 3, buffer);
		glm::vec4 color = glm::make_vec4(buffer);

		prop.diffuse = color;
		prop.ambient = color;
		prop.specular = color;
		prop.constantAttenuation = (float) std::atof(pointNode->FirstChildElement("constant_attenuation")->GetText());
		prop.linearAttenuation = (float) std::atof(pointNode->FirstChildElement("linear_attenuation")->GetText());
		prop.quadraticAttenuation = (float) std::atof(pointNode->FirstChildElement("quadratic_attenuation")->GetText());

		return light;
	}

	void parseLights(cgl::AssetLibrary& assetLibrary, tinyxml2::XMLElement* base)
	{
		if(!base)
			return;
		for(tinyxml2::XMLElement* lightNode = base->FirstChildElement("light"); 
			lightNode; 
			lightNode = lightNode->NextSiblingElement("light"))
		{
			std::string id = lightNode->Attribute("id");

			tinyxml2::XMLElement* technique = lightNode->FirstChildElement("technique_common");
			// In our case, the Texture object contains both surface and sampler
			
			cgl::Light* light;
			if(tinyxml2::XMLElement* pointNode = technique->FirstChildElement("point"))
				light = parsePointLight(assetLibrary, pointNode);			
			else 
			{
				std::cerr << "Unidentified light detected" << std::endl;
				continue;
			}
			assetLibrary.store(id, light);
		}
	}

	void parseMaterials(tinyxml2::XMLElement* base)
	{
		if(!base)
			return;
		for(tinyxml2::XMLElement* materialNode = base->FirstChildElement("material"); 
			materialNode; 
			materialNode = materialNode->NextSiblingElement("material"))
		{
			std::string id = materialNode->Attribute("id");
			materialToEffectMap[id] = materialNode->FirstChildElement("instance_effect")->Attribute("url")+1;
		}
	}

	struct AnimationSampler
	{
		COLLADASource* input;
		COLLADASource* output;
		COLLADASource* interpolation;
	};

	AnimationSampler parseSampler(tinyxml2::XMLElement* base, std::map<std::string, COLLADASource>& sources)
	{
		AnimationSampler sampler;
		for(tinyxml2::XMLElement* inputNode = base->FirstChildElement("input"); 
			inputNode; 
			inputNode = inputNode->NextSiblingElement("input"))
		{
			COLLADASource* source = &sources[inputNode->Attribute("source")+1];
			std::string semantic = inputNode->Attribute("semantic");
			if(semantic == "INPUT")
				sampler.input = source;
			else if(semantic == "OUTPUT")
				sampler.output = source;
			else if(semantic == "INTERPOLATION")
				sampler.interpolation = source;
			else
				std::cerr << "Unknown semantic of animation sampler" << std::endl;
		}	
		return sampler;
	}

	std::map<std::string, AnimationSampler> parseSamplers(tinyxml2::XMLElement* base, std::map<std::string, COLLADASource>& sources)
	{
		std::map<std::string, AnimationSampler> samplers;
		for(tinyxml2::XMLElement* samplerNode = base->FirstChildElement("sampler"); 
			samplerNode; 
			samplerNode = samplerNode->NextSiblingElement("sampler"))
		{
			std::string id = samplerNode->Attribute("id");
			samplers[id] = parseSampler(samplerNode, sources);
		}	
		return samplers;
	}
	
	void parseAnimations(AnimationSystem& animationSystem, tinyxml2::XMLElement* base)
	{
		for(tinyxml2::XMLElement* animationNode = base->FirstChildElement("animation"); 
			animationNode; 
			animationNode = animationNode->NextSiblingElement("animation"))
		{
			std::string id = animationNode->Attribute("id");

			// We assume a single output per input

			// Obtain the raw source data
			std::map<std::string, COLLADASource> sources;		
			parseSources(animationNode, sources);
			std::map<std::string, AnimationSampler> samplers = parseSamplers(animationNode, sources);

			// Parse channels			
			for(tinyxml2::XMLElement* channelNode = animationNode->FirstChildElement("channel"); 
				channelNode; 
				channelNode = channelNode->NextSiblingElement("channel"))
			{
				AnimationSampler sampler = samplers[channelNode->Attribute("source")+1];

				cgl::SplineSampler splineSampler;
				int samplesNum = sampler.input->getElementCount();
				float* X = sampler.input->getDataFloat();
				float* Y = sampler.output->getDataFloat();
				for(int i=0; i<samplesNum-1; ++i)
				{					
					splineSampler.addSample(X[i], Y[i],
						new cgl::LinearSplineSegment(X[i],Y[i],X[i+1],Y[i+1]));
				}

				// Get target
				std::string target = channelNode->Attribute("target");
				int tokenIndex = target.find_first_of(".");
				std::string idFull = target.substr(0, tokenIndex);
				std::string varName = target.substr(tokenIndex+1,target.size());
				float* channelTarget = ((cgl::ssg::TransformationNode*) _idsMap[idFull])->getVariable(varName);
				cgl::Animator* animator = new cgl::Animator(splineSampler, channelTarget);
				animationSystem.addAnimation(animator);
			}				
		}
	}

	void parseImages(cgl::AssetLibrary& assetLibrary, tinyxml2::XMLElement* base)
	{
		if(!base)
			return;
		for(tinyxml2::XMLElement* imageNode = base->FirstChildElement("image"); 
			imageNode; 
			imageNode = imageNode->NextSiblingElement("visual_scene"))
		{
			std::string id = imageNode->Attribute("id");
			std::string imagePath;
			try
			{
				// Expecting relative path
				std::string imageFilename = imageNode->FirstChildElement("init_from")->GetText();
				imagePath = _pathbase + imageFilename;
			}
			catch(...)
			{
				throw std::exception("failed to parse image asset");
			}

			cgl::Image* image = new cgl::Image(imagePath);
			assetLibrary.store(id, image);
		}
	}
	
	void importCollada(std::string filename, AssetLibrary& assetLibrary, AnimationSystem* animationSystem = NULL)
	{		
		tinyxml2::XMLDocument doc;
		doc.LoadFile(filename.c_str());		

		_pathbase = filename.substr(0,1+filename.find_last_of("/"));
		tinyxml2::XMLElement* root = doc.FirstChildElement("COLLADA");

		parseCameras(assetLibrary, root->FirstChildElement("library_cameras"));
		parseGeometries(assetLibrary, root->FirstChildElement("library_geometries"));
		parseImages(assetLibrary, root->FirstChildElement("library_images"));
		parseEffects(assetLibrary, root->FirstChildElement("library_effects"));
		parseLights(assetLibrary, root->FirstChildElement("library_lights"));
		parseMaterials(root->FirstChildElement("library_materials"));		
		parseVisualScenes(assetLibrary, root->FirstChildElement("library_visual_scenes"));		
		if(animationSystem)
			parseAnimations(*animationSystem, root->FirstChildElement("library_animations"));		
	}
};