#include <stdlib.h>
#include <string>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>
#include <iostream>

#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))

namespace cgl
{
	class Shader
	{
	protected:
		std::string _source;
		GLuint _id;
		GLuint _type;

	public:
		Shader(GLuint type, std::string source)
		{
			this->_source = source;
			this->_type = type;
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
		void build(std::vector<Shader>& shaders)
		{
			_id = glCreateProgram();
			glUseProgram(_id);
			
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

	struct Attrib 
	{
		void* clientBufferPtr;
		int clientBufferSize;
		std::string name;
		unsigned int componentsNum;		
		GLuint hostBufferId;
		bool isConstant;
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

	public:
		SimpleMesh() : _isInitBuffers(false), _elementsPerFace(3), _drawMode(GL_TRIANGLES)
		{
			
		}

		void init(std::vector<GLuint> faces)
		{
			// Copy values
			_faces = faces;
		}

		SimpleMesh* addAttrib(std::string name, unsigned int componentsNum, void* clientBufferPtr, int bufferSize)
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
		
			_attribs.push_back(attrib);

			return this;
		}

		void setAttribLocs(std::vector<GLuint> val) { _attribLocs = val;}

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
				Attrib& attrib = _attribs[i];
				glBindBuffer(GL_ARRAY_BUFFER, attrib.hostBufferId);
				glEnableVertexAttribArray(_attribLocs[i]);
				glVertexAttribPointer(_attribLocs[i], attrib.componentsNum, GL_FLOAT, false, 0, BUFFER_OFFSET(0));					
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
}
