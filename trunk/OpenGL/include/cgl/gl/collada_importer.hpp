#pragma once
#include <stdlib.h>
#include "cgl/gl/common.hpp"
#include <vector>
#include <map>
#include <string>
#include <sstream>

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "asset_library.hpp"

#include "tinyxml2\tinyxml2.h"
#include "tinyxml2\tinyxml2.cpp"

namespace cgl
{
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
				aspect_ratio = 1;// TODO
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
		float* _data;
		std::string _id;
		int _elementSize;
	public:
		COLLADASource()
		{
		}

		COLLADASource(tinyxml2::XMLElement* base)
		{
			_id = base->Attribute("id");
			int count = base->FirstChildElement("float_array")->IntAttribute("count");
			const char* text = base->FirstChildElement("float_array")->GetText();
			_data = new float[count];
			std::stringstream ss(text);
			for(int i=0; i<count; ++i)
			{
				ss >> _data[i];
			}
			_elementSize = 3; //TODO!
		}

		void dereference(std::vector<int> indices, std::vector<float>& result)	
		{
			result.resize(indices.size()*_elementSize);
			int cnt=0;
			for(unsigned int i=0; i<indices.size(); ++i)
			{
				for(int j=0; j<_elementSize; ++j)
				{
					result[cnt++] = _data[indices[i]*_elementSize+j];				
				}
			}
		}

		float* getData() const { return _data; };

		int getElementSize() { return _elementSize; }; 
	};

	struct COLLADAPolylistInput
	{
		COLLADASource* source;
		std::string semantic;
		int offset;
		std::vector<int> indices;
	};

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
		std::vector<COLLADAPolylistInput> inputs;

		for(tinyxml2::XMLElement* geometryNode = base->FirstChildElement("geometry"); 
			geometryNode; 
			geometryNode = geometryNode->NextSiblingElement("geometry"))
		{
			std::string id = geometryNode->Attribute("id");
			tinyxml2::XMLElement* meshNode = geometryNode->LastChildElement("mesh");

			// Obtain the raw source data
			parseMeshSources(assetLibrary, meshNode, sources);
		
			// Extract attribute semantics and faces
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
	public:
		std::map<std::string, int> _names;
		enum 
		{
			LIBRARY_CAMERAS,
		};

		COLLADAXMLVisitor()
		{
			_names["library_cameras"] = LIBRARY_CAMERAS;
		}

		int getElementNameId(const char* name)
		{
			return _names[name];
		}

	   /// Visit a document.
		virtual bool VisitEnter( const tinyxml2::XMLDocument& doc )			{
			return true;
		}
		/// Visit a document.
		virtual bool VisitExit( const tinyxml2::XMLDocument& doc )			{
			return true;
		}

		/// Visit an element.
		virtual bool VisitEnter( const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* firstAttribute )	{
			switch(getElementNameId(element.Name()))
			{
			case LIBRARY_CAMERAS:
				break;
			}
			return true;
		}
		/// Visit an element.
		virtual bool VisitExit( const tinyxml2::XMLElement& element )			{
			return true;
		}

		/// Visit a declaration.
		virtual bool Visit( const tinyxml2::XMLDeclaration& declaration )		{
			return true;
		}
		/// Visit a text node.
		virtual bool Visit( const tinyxml2::XMLText& text )					{
			return true;
		}
		/// Visit a comment node.
		virtual bool Visit( const tinyxml2::XMLComment& comment )				{
			return true;
		}
		/// Visit an unknown node.
		virtual bool Visit( const tinyxml2::XMLUnknown& unknown )				{
			return true;
		}
	};

	void importCollada(std::string filename, AssetLibrary& assetLibrary)
	{
		tinyxml2::XMLDocument doc;
		doc.LoadFile(filename.c_str());
		COLLADAXMLVisitor visitor;
		//doc.Accept(&visitor);

		tinyxml2::XMLElement* root = doc.FirstChildElement("COLLADA");
		parseCameras(assetLibrary, root->FirstChildElement("library_cameras"));
		parseGeometries(assetLibrary, root->FirstChildElement("library_geometries"));
	}
}