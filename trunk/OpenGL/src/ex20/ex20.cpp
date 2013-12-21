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

//#include "json-parser/json.h"
//#include "json-parser/json.c"

//#include "json/autolink.h"
#include "json/reader.h"

// Not sure why the window we get is different from the size we request...
#define GLUT_WINDOW_PAD 8
#define MOTION_FILENAME "../res/motion/01_01.bvh"

class Joint;

int g_frame;
int	g_height, g_width;
Joint* g_root;
int g_channelsNum;
float* g_motionValues;
int g_framesNum;
cgl::SimpleMesh sm;
cgl::Program g_program;
glm::mat4 g_modelView(1), g_projection;

struct Uniforms
{
	GLuint modelViewMatrix, projectionMatrix, jointColors;
} g_uniforms;

struct Attributes
{
	GLuint position, influenceJoints, influenceWeights;
} g_attributes;

enum E_CHANNELS
{
	POSX, 
	POSY,
	POSZ,
	ROTZ,	
	ROTX,
	ROTY
};

E_CHANNELS channelFromString(std::string str)
{
	std::map<std::string, E_CHANNELS> dict;
	dict["Xposition"] = POSX;
	dict["Yposition"] = POSY;
	dict["Zposition"] = POSZ;
	dict["Xrotation"] = ROTX;
	dict["Yrotation"] = ROTY;
	dict["Zrotation"] = ROTZ;
	return dict[str];
}

class Joint
{
public:

	Joint(std::ifstream& fin)
	{
		std::memset(_validChannels, false, 6);
		parseFromStream(fin);
	}

	void render()
	{
		glTranslatef(_offset.x, _offset.y, _offset.z);
		//if(_name == "lShin")
		//	_CrtDbgBreak();

		if(_validChannels[POSX])
			glTranslatef(_channels[POSX],_channels[POSY],_channels[POSZ]);

		if(_validChannels[ROTX])
		{			
			if(_name == "hip")
			{
				glRotatef(_channels[ROTZ],0,0,1);									
				glRotatef(_channels[ROTX],0,1,0);
				glRotatef(_channels[ROTY],1,0,0);
			}
			else
			{			
				glRotatef(_channels[ROTZ],0,0,1);					
				glRotatef(_channels[ROTX],1,0,0);
				glRotatef(_channels[ROTY],0,1,0);
			}
			
			//glRotatef(_channels[ROTY],1,0,0);
			//glRotatef(_channels[ROTZ],0,1,0);
			//glRotatef(_channels[ROTX],0,0,1);

			//glRotatef(-_channels[ROTX],0,1,0);
			//glRotatef(-_channels[ROTY],1,0,0);
			//glRotatef(-_channels[ROTZ],0,0,1);					
			


		}

		GLUquadric *q = gluNewQuadric();
		gluSphere(q, 2, 5, 5);

		/*
		glBegin(GL_LINES);
		glColor3f(1,0,0);
		glVertex3f(0,0,0);
		glColor3f(1,0,0);
		glVertex3f(10,0,0);

		glColor3f(0,1,0);
		glVertex3f(0,0,0);
		glColor3f(0,1,0);
		glVertex3f(0,10,0);

		glColor3f(0,0,1);
		glVertex3f(0,0,0);
		glColor3f(0,0,1);
		glVertex3f(0,0,10);

		glEnd();
		*/
		for(int i=0; i<_children.size(); ++i)
		{
			glBegin(GL_LINES);
			glVertex3f(0,0,0);
			glVertex3fv(glm::value_ptr(_children[i]->_offset));
			glEnd();
			
			glPushMatrix();
			_children[i]->render();
			glPopMatrix();
		}
	}

	void parseFromStream(std::ifstream& fin)
	{
		std::string tempStr;
		fin >> _name;
		fin >> tempStr;
		//std::string line;
		//std::getline(fin, line);
		fin >> tempStr;		
		fin >> _offset.x >> _offset.y >> _offset.z;
		
		std::string channelsStrs[6];
		fin >> tempStr;		
		if(tempStr == "CHANNELS")
		{
			// Parse channels
			int channelsNum;
			fin >> channelsNum;
			for(int i=0; i<channelsNum; ++i)
			{
				fin >> tempStr;		
				_validChannels[channelFromString(tempStr)] = true;
			}

			// Parse children
			while(fin >> tempStr)
			{
				if(tempStr == "JOINT" || tempStr == "End")
				{
					_children.push_back(new Joint(fin));
				}
				else if(tempStr == "}")
				{
					return;
				}
				else
				{
					assert(false);
				}
			}

		} 
		else if(tempStr == "}")
		{
			return;
		}
		else
		{
			assert(false);
		}
	}

	int countValids()
	{
		int validsNum = 0;
		for(int i=0; i<6; ++i)
			validsNum += _validChannels[i];

		for(int i=0; i<_children.size(); ++i)
			validsNum += _children[i]->countValids();

		return validsNum;
	}

	void updateChannels(float** valueStream)
	{
		int validsNum = 0;
		for(int i=0; i<6; ++i)
		{
			if(_validChannels[i])
			{
				_channels[i] = *((*valueStream)++);
			}
		}

		for(int i=0; i<_children.size(); ++i)
			_children[i]->updateChannels(valueStream);
	}


protected:
	std::string _name;
	glm::vec3 _offset;
	std::vector<Joint*> _children;	
	float _channels[6];
	bool _validChannels[6];
};

void drawScene()
{	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glOrtho(-100,100,-100,100,-1000,1000);
	gluPerspective(90,1,1,1000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(g_motionValues[0],g_motionValues[1],500, g_motionValues[0],g_motionValues[1],g_motionValues[2], 0,1,0);

	//glRotatef(90,0,1,0);	

	g_program.use();
	g_projection = glm::perspective(glm::pi<double>()/2,1.0,1.0,1000.0);
	g_modelView = glm::lookAt(
		glm::vec3(0,0,1000), 
		glm::vec3(0,0,0),
		glm::vec3(0,1,0));

	g_modelView = glm::rotate(g_modelView, -90.0f, glm::vec3(0,1,0));
	g_modelView = glm::rotate(g_modelView, -90.0f, glm::vec3(1,0,0));
	g_modelView = glm::translate(g_modelView, glm::vec3(-11.37770081,  +6.67086077,   -5.01906919));
	
	//g_modelView = glm::rotate(g_modelView, glm::pi<float>()/2.0f, glm::vec3(0,1,0));
	//g_modelView = glm::scale(g_modelView, glm::vec3(2,2,2));

	glProgramUniformMatrix4fv(g_program.getId(), g_uniforms.modelViewMatrix, 1, false, glm::value_ptr(g_modelView));
	glProgramUniformMatrix4fv(g_program.getId(), g_uniforms.projectionMatrix, 1, false, glm::value_ptr(g_projection));
	sm.render();
	g_program.useDefault();
	g_root->render();	

}

void initShaders()
{	
	std::vector<cgl::Shader> shaders;
	shaders.push_back(cgl::Shader::fromFile(GL_VERTEX_SHADER,"../res/shader/ex20.vert"));		
	shaders.push_back(cgl::Shader::fromFile(GL_FRAGMENT_SHADER,"../res/shader/ex20.frag"));		
	g_program.build(shaders);	

	g_attributes.position = glGetAttribLocation(g_program.getId(), "aPosition");
	g_uniforms.modelViewMatrix = glGetUniformLocation(g_program.getId(), "uModelViewMatrix");
	g_uniforms.projectionMatrix = glGetUniformLocation(g_program.getId(), "uProjectionMatrix");
}

void init() 
{
	// Init GL Debug
	cgl::initDebugAll();
	initShaders();

	//std::string json = cgl::Shader::readFile("C:\\Users\\ChenLucy\\Cache\\opengl-exercises\\OpenGL\\res\\motion\\skin.cgtf");

	std::ifstream file("C:\\Users\\ChenLucy\\Cache\\opengl-exercises\\OpenGL\\res\\motion\\skin.bin", std::ios::binary);
	if(!file.is_open())
		throw std::exception("Failed to load file");
	file.seekg(0, std::ios::end);
	GLuint size = (GLuint) file.tellg();
	file.seekg(0, std::ios::beg); 
	char* bindata = new char[size];
	file.read(bindata, size);

	std::ifstream ifs("C:\\Users\\ChenLucy\\Cache\\opengl-exercises\\OpenGL\\res\\motion\\skin.cgtf");
	Json::Value val;
	Json::Reader rdr;
	rdr.parse(ifs, val);		
	
	std::vector<GLuint> indices;
	unsigned short* ptr = (unsigned short*)(bindata + val["triangles"]["pos"].asInt());
	const int cnt = val["triangles"]["bytecount"].asInt()/2;
	for(int i=0; i<cnt; ++i)
	{
		indices.push_back(*(ptr+i));
	}
	sm.init(indices);
	sm.addAttrib("position", 3, bindata + val["vertex_positions"]["pos"].asUInt(), val["vertex_positions"]["bytecount"].asUInt());
	sm.addAttrib("weight", 4, bindata + val["vertex_weights"]["pos"].asUInt(), val["vertex_weights"]["bytecount"].asUInt());
	//sm.setDrawMode(GL_POINTS);

	std::vector<GLuint> attribLocs;
	attribLocs.push_back(g_attributes.position);
	sm.setAttribLocs(attribLocs);

	sm.initBuffers();

	/*
	json_value* jv = json_parse(json.c_str(), json.length());
	*/

	// Init assets
	
	std::ifstream fin(MOTION_FILENAME, std::ifstream::in);
	assert(fin.is_open());
	std::string tempStr;
	fin >> tempStr;
	assert(tempStr == "HIERARCHY");
	fin >> tempStr;
	assert(tempStr == "ROOT");

	g_root = new Joint(fin);
	g_channelsNum = g_root->countValids();

	fin >> tempStr;
	assert(tempStr == "MOTION");
	fin >> tempStr;
	assert(tempStr == "Frames:");
	fin >> g_framesNum;
	fin >> tempStr >> tempStr;
	assert(tempStr == "Time:");
	float frameTime;
	fin >> frameTime;

	const int totalValues = g_channelsNum*g_framesNum;
	g_motionValues = new float[totalValues];

	for(int i=0; i<totalValues; ++i)
	{
		fin >> g_motionValues[i];
	}

	fin.close();

	// Set background color to gray
	glClearColor(0.7f, 0.7f, 0.4f, 0);

	//g_frame = 1020;
	g_frame = 6;
}

void display(void) 
{
	glEnable(GL_DEPTH_TEST);

	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		
	
	float* ptr = g_motionValues+g_channelsNum*g_frame;
	g_root->updateChannels(&ptr);

	// Apply shaders
	drawScene();

	//cgl::dumpColorBuffer("out.raw", g_width, g_height);
	
	// Swap double buffer
	glutSwapBuffers();

	g_frame++;
	//if(g_frame > 1300)
	//	g_frame = 1020;
	//::Sleep(10);
	g_frame = g_frame % g_framesNum;
}

void reshape(int width, int height) 
{
	g_width = width-GLUT_WINDOW_PAD;
	g_height = height-GLUT_WINDOW_PAD;
	glViewport(0,0,g_width,g_height);
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

void keyboardSpecialFunc(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_F1:
		break;

	case GLUT_KEY_F2:
		break;

	default:
		break;
	}
}

int main(int argc, char **argv) 
{
	try
	{
		//glutInitContextVersion(4,3);
		//glutInitContextProfile(GLUT_CORE_PROFILE);
		//glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
		//glutInitContextFlags(GLUT_DEBUG);

		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
		glutInitWindowPosition(0, 0);
		glutInitWindowSize(512+GLUT_WINDOW_PAD*2, 520+GLUT_WINDOW_PAD*2);

		glutCreateWindow("ex17 - Asset management");

		glutReshapeFunc(reshape);
		glutDisplayFunc(display);
		glutIdleFunc(display);
		glutKeyboardFunc(keyboardFunc);
		glutSpecialFunc(keyboardSpecialFunc);
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

