#pragma once
#include <stdlib.h>
#include <string>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <time.h>

#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))

namespace cgl
{
	bool loadMeshFromOffFile(std::string fileName, std::vector<glm::vec3>& vertices, std::vector<glm::ivec3>& triangles) ;
		
	class Shader
	{
	protected:
		std::string _source;
		GLuint _id;
		GLuint _type;

	public:
		static std::string readFile(std::string filename)
		{
			std::ifstream file(filename);		
			if(!file.is_open())
				throw std::exception("Failed to load file");
			file.seekg(0, std::ios::end);
			GLuint size = (GLuint) file.tellg();
			file.seekg(0, std::ios::beg); 
			std::string text(size + 1, 0);
			file.read((char*)text.data(), size);
			return text;
		}

		static Shader fromFile(GLuint type, std::string filename)
		{
			return Shader(type, readFile(filename));
		}

		Shader(GLuint type, std::string source)
		{
			this->_source = source;
			this->_type = type;
		}

		Shader& addHeader(std::string headerSource)
		{
			this->_source = headerSource + "\n" + _source;
			return *this;
		}

		bool compile()
		{
			GLint compiled;

			_id = glCreateShader(_type);
			const char* cstr = _source.c_str();
			glShaderSource(_id, 1, &cstr, NULL);

			glCompileShader(_id);

			glGetShaderiv(_id, GL_COMPILE_STATUS, &compiled);
			GLint blen = 0;	GLsizei slen = 0;

			glGetShaderiv(_id, GL_INFO_LOG_LENGTH , &blen);       

			if (blen > 1)
			{
				GLchar* compiler_log = (GLchar*)malloc(blen);
				glGetInfoLogARB(_id, blen, &slen, compiler_log);
				printf("compiler_log:\n");
				printf("%s\n", compiler_log);
				free (compiler_log);
			}
			return compiled == 1;
		}

		GLuint getId() const { return this->_id; } 
	};
	
	class Program
	{
	protected:
		GLuint _id;

		int checkProgramInfoLog(GLuint prog) const
		{
			int len = 0, read = 0;
			std::string log;
			glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
			if (len > 0)
			{
				log.resize(len);
				glGetProgramInfoLog(prog, len, &read, (GLchar*) log.data());
				printf("Program Info Log:\n%s\n", log.c_str());
			}
			int ret;
			glGetProgramiv(prog, GL_LINK_STATUS, &ret);
	
			return ret;
		}

	public:
		Program(){}

		Program(Shader& shader)
		{
			build(shader);
		}

		void use() const
		{
			glUseProgram(this->getId());
		}

		void useDefault() const
		{
			glUseProgram(0);
		}

		void build(Shader& shader)
		{
			std::vector<Shader> shaders;
			shaders.push_back(shader);
			this->build(shaders, true);
		}

		void build(std::vector<Shader>& shaders, bool isSeperable=false)
		{
			_id = glCreateProgram();			
			
			//
			// Compiling
			//
			for(
				std::vector<Shader>::iterator shader = shaders.begin();
				shader != shaders.end();
				++shader)
				{					
					std::cout << "Compiling..." << std::endl;
					if(!shader->compile())
						throw std::exception("Failed to compile shader - quitting build");
					glAttachShader(_id, shader->getId());
				}			

			if(isSeperable)
			{
				glProgramParameteri(_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
			}

			//
			// Linking
			//
			glLinkProgram(_id);

			//glValidateProgram(program);
			if(checkProgramInfoLog(_id) == 0) 
			{
				throw new std::exception("Failed to link programfrom shaders");
			} 
			else 
			{
				std::cout << "Program created!" << std::endl;
			}
		}
		
		GLuint getId() const { return this->_id; } 
	};

	class SpecificProgram : public Program
	{
		virtual void init() = 0;
	};

	struct Attrib 
	{
		void* clientBufferPtr;
		int clientBufferSize;
		std::string name;
		unsigned int componentsNum;		
		GLuint hostBufferId;
		bool isConstant;
		GLuint type;
	};

	class SimpleMesh
	{
	protected:					
		bool _isInitBuffers;
		std::vector<Attrib> _attribs;
		std::vector<GLuint> _attribLocs;

		std::vector<GLuint> _faces;
		GLuint _facesHostBufferId;

		unsigned int _elementsNum;
		unsigned int _elementsPerFace;		
		GLuint _drawMode;
		GLuint _vao;

		static const unsigned int INVALID_ATTRIB_LOC = 9999;
	public:
		SimpleMesh() : _isInitBuffers(false), _elementsPerFace(3), _drawMode(GL_TRIANGLES)
		{
			
		}

		void init(std::vector<GLuint> faces)
		{
			// Copy values
			_faces = faces;
		}

		void init(const std::vector<glm::ivec3>& faces)
		{
			std::vector<GLuint> vec((GLuint*) faces.data(), (GLuint*) faces.data() + faces.size()*3);
			init(vec);
		}

		void init(std::string _meshFilepath, GLuint program)
		{
			std::vector<glm::vec3> vertices;
			std::vector<glm::ivec3> triangles;
			loadMeshFromOffFile(_meshFilepath, vertices, triangles);	
			init(triangles);
			addAttrib("aPosition", 3, vertices.data(), vertices.size()*sizeof(glm::vec3));
			initAttribLocs(program);
			initBuffers();
		}

		SimpleMesh* addAttrib(std::string name, unsigned int componentsNum, void* clientBufferPtr, int bufferSize, GLuint type = GL_FLOAT)
		{
			if(_attribs.empty())
			{
				// Obtain number of elements from first attribute, assuming
				// it's the most important.
				_elementsNum = (bufferSize/sizeof(float))/componentsNum;
			}

			Attrib attrib;
			attrib.clientBufferPtr = clientBufferPtr;
			attrib.clientBufferSize = bufferSize;
			attrib.name = name;
			attrib.componentsNum = componentsNum;
			attrib.type = type;

			_attribs.push_back(attrib);

			return this;
		}

		// TODO: when changing attribLocs, need to re-init VAO
		void setAttribLocs(std::map<std::string, GLuint> dict)
		{
			_attribLocs.resize(_attribs.size());
			for(unsigned int i=0; i<_attribs.size(); ++i)
			{
				if(dict.count(_attribs[i].name))
					_attribLocs[i] = dict[_attribs[i].name];
				else
					_attribLocs[i] = INVALID_ATTRIB_LOC;
			}		
		}

		void initAttribLocs(GLuint program) 
		{
			_attribLocs.resize(_attribs.size());
			for(unsigned int i=0; i<_attribs.size(); ++i)
			{				
				_attribLocs[i] = glGetAttribLocation(program, _attribs[i].name.c_str());
			}
		}

		void setAttribLocs(std::vector<GLuint> val) { _attribLocs = val;}
		void setDrawMode(GLuint val) { _drawMode = val;}

		void render()
		{
			if(!_isInitBuffers)
			{
				this->initBuffers();
			}
			if(_isInitBuffers)
			{
				this->renderActually();
			}
		}

		void initEBO()
		{
			if(!_faces.empty())
			{
				glGenBuffers(1, &_facesHostBufferId);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _facesHostBufferId);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, _faces.size() * sizeof(GLuint), (const char*) &_faces[0], GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
		}

		void initVBO()
		{
			for(unsigned int i=0; i<_attribs.size(); ++i)
			{
				Attrib& attrib = _attribs[i];
				glGenBuffers(1, &attrib.hostBufferId);
				glBindBuffer(GL_ARRAY_BUFFER, attrib.hostBufferId);
				glBufferData(GL_ARRAY_BUFFER, attrib.clientBufferSize, attrib.clientBufferPtr, GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
		}

		void initVAO()
		{
			// Keep all vertex attribute states in VAO
			glGenVertexArrays(1, &_vao);
			glBindVertexArray(_vao);

			for(unsigned int i=0; i<_attribs.size(); ++i)
			{
				if(_attribLocs[i] == INVALID_ATTRIB_LOC)
					continue;
				Attrib& attrib = _attribs[i];
				glBindBuffer(GL_ARRAY_BUFFER, attrib.hostBufferId);
				glEnableVertexAttribArray(_attribLocs[i]);
				if(attrib.type == GL_UNSIGNED_INT)
					glVertexAttribIPointer(_attribLocs[i], attrib.componentsNum, attrib.type, 0, BUFFER_OFFSET(0));					
				else
					glVertexAttribPointer(_attribLocs[i], attrib.componentsNum, attrib.type, false, 0, BUFFER_OFFSET(0));					
				
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}

			glBindVertexArray(0);
		}

		void initBuffers() 
		{
			initVBO();
			initVAO();
			initEBO();
			_isInitBuffers = true;
		}

		void renderActually()
		{
			glBindVertexArray(_vao);
			if(_faces.empty())
			{
				glDrawArrays(_drawMode, 0, _elementsNum);
			}
			else
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _facesHostBufferId);
				glDrawElements(_drawMode, _faces.size(), GL_UNSIGNED_INT, BUFFER_OFFSET(0)); 
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			glBindVertexArray(0);
		}
	};


	class Timer
	{
	protected:
		// FPS counter
		const int trailLength;
		int _curDiff;
		long _timeDiff[16];
		long _lastTime;

	public:
		Timer() : _curDiff(0), _lastTime(0), trailLength(sizeof(_timeDiff)/sizeof(long))
		{
		}

		void start()
		{
			long temp = clock();
			_lastTime = temp;
		}

		void stop()
		{
			_timeDiff[_curDiff++] = clock() - _lastTime;			
			_curDiff = _curDiff % trailLength;		
		}

		double averageTail() {
			//return ((double)(_timeDiff[0]+_timeDiff[1]+_timeDiff[2]+_timeDiff[3]+_timeDiff[4])/5);
			double sum = 0;
			for(int i=0;i<trailLength;++i)
				sum += _timeDiff[i];
			return sum/trailLength;
		}
	};

	void dumpColorBuffer(std::string filename, int width, int height)
	{	
		static char buffer[512*512*3];
		memset(buffer, 0, width*height*3);
		glPixelStorei(GL_PACK_ALIGNMENT,1);
		glReadBuffer(GL_BACK_LEFT);
		glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,buffer);
		std::ofstream f(filename, std::ios_base::binary);
		f.write(buffer,width*height*3);
		f.close();	
	}


	bool loadMeshFromOffFile(std::string fileName, std::vector<glm::vec3>& vertices, std::vector<glm::ivec3>& triangles) 
	{
		int verticesNum, trianglesNum;

		int i;
		FILE *fp;
		errno_t err;
		if((err = fopen_s(&fp, fileName.c_str(), "rb"))!=0)
		{
			std::cout << "File not found " << fileName << std::endl;
			return false;
		}
		std::cout << "Loaded successfully " << fileName << std::endl;

		fscanf_s(fp, "OFF %d %d 0", &verticesNum, &trianglesNum);

		vertices.resize(verticesNum);
		triangles.resize(trianglesNum);

		for (i=0; i < verticesNum; ++i)
		{
			fscanf_s(fp, "%f %f %f", 
				&(vertices[i].x), 
				&(vertices[i].y),
				&(vertices[i].z));
		}
		for (i=0; i < trianglesNum; ++i)
		{
			fscanf_s(fp, " 3 %d %d %d", 
				&(triangles[i][0]), 
				&(triangles[i][1]), 
				&(triangles[i][2]));	
		}
		fclose(fp);

		return true;
	}

}

//
// Copied completely from GLF (http://sourceforge.net/projects/glf/) - MIT License
//
static void APIENTRY debugOutput
(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	GLvoid* userParam
)
{
	//FILE* f;
	//f = fopen("debug_output.txt","a");
	//if(f)
	{
		char debSource[32], debType[32], debSev[32];
		bool Error(false);

		if(source == GL_DEBUG_SOURCE_API_ARB)
			strcpy_s(debSource, "OpenGL");
		else if(source == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)
			strcpy_s(debSource, "Windows");
		else if(source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)
			strcpy_s(debSource, "Shader Compiler");
		else if(source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB)
			strcpy_s(debSource, "Third Party");
		else if(source == GL_DEBUG_SOURCE_APPLICATION_ARB)
			strcpy_s(debSource, "Application");
		else if(source == GL_DEBUG_SOURCE_OTHER_ARB)
			strcpy_s(debSource, "Other");
 
		if(type == GL_DEBUG_TYPE_ERROR_ARB)
			strcpy_s(debType, "error");
		else if(type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)
			strcpy_s(debType, "deprecated behavior");
		else if(type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)
			strcpy_s(debType, "undefined behavior");
		else if(type == GL_DEBUG_TYPE_PORTABILITY_ARB)
			strcpy_s(debType, "portability");
		else if(type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
			strcpy_s(debType, "performance");
		else if(type == GL_DEBUG_TYPE_OTHER_ARB)
			strcpy_s(debType, "message");
 
		if(severity == GL_DEBUG_SEVERITY_HIGH_ARB)
		{
			strcpy_s(debSev, "high");
			Error = true;
		}
		else if(severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
			strcpy_s(debSev, "medium");
		else if(severity == GL_DEBUG_SEVERITY_LOW_ARB)
			strcpy_s(debSev, "low");

			fprintf(stderr,"%s: %s(%s) %d: %s\n", debSource, debType, debSev, id, message);
			assert(!Error);
			//fclose(f);
	}
}

namespace cgl
{
	void initDebugAll()
	{
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageCallback(&debugOutput, NULL);
	}

	int checkProgramInfoLog(GLuint prog) 
	{
		int len = 0, read = 0;
		std::string log;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
		if (len > 0)
		{
			log.resize(len);
			glGetProgramInfoLog(prog, len, &read, (GLchar*) log.data());
			printf("Program Info Log:\n%s\n", log.c_str());
		}
		int ret;
		glGetProgramiv(prog, GL_LINK_STATUS, &ret);
	
		return ret;
	}

	bool checkShaderCompilationStatus(GLuint shader)
	{
		// Check if compilation successful
		GLint isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
		std::cout << "Compilation " << (isCompiled?"successful":"failed") << std::endl;

		// Check compilation log for errors and warnings
		GLint logLength = 0;	
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &logLength);       
		if (logLength > 1)
		{
			GLsizei actualLogLength = 0;
			std::vector<GLchar> logString(logLength);
			glGetShaderInfoLog(shader, logLength, &actualLogLength, logString.data());
			std::cout 
				<< "Compiler log:" 
				<< std::string(logString.begin(), logString.begin()+actualLogLength) 
				<< std::endl;
		}
		return isCompiled == 1;
	}

}




class SimpleProgram
{
public:
	void buildProgram(	const char* vertexShaderFilename, 
						const char* fragmentShaderFilename);
	void use();

protected:

	GLuint m_uiProgramId;			
};