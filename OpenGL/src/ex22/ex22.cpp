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
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <string>
#include <iostream>
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/color_space.hpp>
#include <glm/gtc/random.hpp>
#include <vector>
#include <cgl/gl/common.hpp>

#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))
#define MAX_VERTICES_AMOUNT_EXPONENT 20
#define VERTICES_AMOUNT (1<<MAX_VERTICES_AMOUNT_EXPONENT)

#define MESH_FILENAME "../res/mesh/player.off"
#define GEOMETRY_SHADER_FILENAME "../res/shader/ex22.geom"

enum EVertexArrayObject
{
	VAO_VERTEX,
	VAO_TOTAL
};

enum EBufferObject
{
	VBO_POSITION,
	VBO_NORMAL,
	EBO_TRIANGLES,
	BUFFER_OBJECTS_TOTAL
};

enum EQueryObject
{
	QUERY_PRIMITIVES,
	QUERY_UPLOAD_TIME,
	QUERY_DOWNLOAD_TIME,
	QUERY_CALC_TIME,
	QUERY_OBJECTS_TOTAL
};

GLuint g_program, g_attribPosition;
GLuint g_vbo[BUFFER_OBJECTS_TOTAL];
GLuint g_vao[VAO_TOTAL];
GLuint g_queries[QUERY_OBJECTS_TOTAL];
std::vector<glm::vec3> g_vertices;
std::vector<glm::ivec3> g_triangles;


// ============================== Helper Functions =========================

bool loadMeshFromFile(std::string fileName, std::vector<glm::vec3>& vertices, std::vector<glm::ivec3>& triangles) 
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

void calcNormalsCPU(const std::vector<glm::vec3>& vertices, const std::vector<glm::ivec3>& triangles, std::vector<glm::vec3>& normals) 
{
	// Normal for each triangle polygon
	normals.resize(triangles.size());	

	for(unsigned int i=0; i<triangles.size(); ++i)
	{
		glm::vec3 u = vertices[triangles[i][1]] - vertices[triangles[i][0]];
		glm::vec3 v = vertices[triangles[i][2]] - vertices[triangles[i][0]];
		glm::vec3 res = glm::cross(u,v);
		float norm = glm::length(res);
		if(norm == 0)
		{
			normals[i] = glm::vec3(0);
		}
		else
		{
			normals[i] = res/norm;
		}
	}
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

bool compileShader(GLuint shader) 
{
	GLint compiled;

	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	GLint blen = 0;	GLsizei slen = 0;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &blen);       

	if (blen > 1)
	{
	 GLchar* compiler_log = (GLchar*)malloc(blen);
	 glGetInfoLogARB(shader, blen, &slen, compiler_log);
	 printf("compiler_log:\n");
	 printf("%s\n", compiler_log);
	 free (compiler_log);
	}

	return compiled == 1;
}

GLuint createProgram(const char* srcVert, const char* srcGeom) 
{
	std::cout << "Creating program" << std::endl;

	bool OK = true;

	//puts("Compiling Vertex Shader\n");
	GLuint vsId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vsId, 1, (const GLchar**) &srcVert, NULL);
	OK = OK && compileShader(vsId);

	//puts("Compiling Fragment Shader\n");
	GLuint gsId = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(gsId, 1, (const GLchar**) &srcGeom, NULL);
	OK = OK && compileShader(gsId);

	if(!OK) 
	{
		printf("Failed to compile, quitting!\n");
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vsId);
	glAttachShader(program, gsId);

	// Declare transform feedback variables
	const GLchar* names[] = {"normal"};
	glTransformFeedbackVaryings(program, 1, names, GL_INTERLEAVED_ATTRIBS); // Write to a single buffer

	// Now ready to link (since oPosition target is bound)
	glLinkProgram(program);

	//glValidateProgram(program);

	if(checkProgramInfoLog(program) == 0) {
		printf("Failed to create program from shaders");
		return 0;
	} else {
		printf("Program created!\n");
	}

	return program;
}

void initVertexBufferObjects()
{
	glBeginQuery(GL_TIME_ELAPSED, g_queries[QUERY_UPLOAD_TIME]);
	// Create identifiers
	glGenBuffers(BUFFER_OBJECTS_TOTAL, g_vbo);
	
	// Transfer data
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION]);
	glBufferData(GL_ARRAY_BUFFER, g_vertices.size()*sizeof(glm::vec3), g_vertices.data(), GL_STATIC_DRAW);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[EBO_TRIANGLES]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, g_triangles.size()*sizeof(glm::ivec3), g_triangles.data(), GL_STATIC_DRAW);

	// Allocate data for "double buffer"
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_NORMAL]);
	glBufferData(GL_ARRAY_BUFFER, g_triangles.size()*sizeof(glm::vec3), NULL, GL_STATIC_COPY);
	glEndQuery(GL_TIME_ELAPSED);
}

void initVertexArrayObjects()
{
	// Keep all vertex attribute states in VAO
	glGenVertexArrays(VAO_TOTAL, g_vao);

	// Define the two drawing states
	glBindVertexArray(g_vao[VAO_VERTEX]);
	{
		glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION]);
		glVertexAttribPointer(g_attribPosition, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));
		glEnableVertexAttribArray(g_attribPosition);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[EBO_TRIANGLES]);
	}
	glBindVertexArray(0);
}

void calcNormalsGPU(std::vector<glm::vec3>& normals) 
{
	// Apply shaders
	glUseProgram(g_program);
	// Disable rasterizer (no need for it)
	glEnable(GL_RASTERIZER_DISCARD);

	// Count number of primitives generated (used for sanity - crucial for variable size primitive output)
	glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, g_queries[QUERY_PRIMITIVES]);
	glBeginQuery(GL_TIME_ELAPSED, g_queries[QUERY_CALC_TIME]);
	{
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, g_vbo[VBO_NORMAL]);
		glBeginTransformFeedback(GL_POINTS);
		{
			glBindVertexArray(g_vao[VAO_VERTEX]);
			{
				glDrawElements(GL_TRIANGLES, g_triangles.size()*3, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
			}
			glBindVertexArray(0);
		}
		glEndTransformFeedback();
	}
	glEndQuery(GL_TIME_ELAPSED);
	glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	// Verify expected number of primitives created
	GLuint primitivesNum;
	glGetQueryObjectuiv(g_queries[QUERY_PRIMITIVES], GL_QUERY_RESULT, &primitivesNum);
	assert(primitivesNum == g_triangles.size());
	glDisable(GL_RASTERIZER_DISCARD);

	normals.resize(g_triangles.size());
	
	// Download results from GPU
	glBeginQuery(GL_TIME_ELAPSED, g_queries[QUERY_DOWNLOAD_TIME]);
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_NORMAL]);
	{
		glGetBufferSubData(GL_ARRAY_BUFFER, 0, g_triangles.size()*sizeof(glm::vec3), normals.data());
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEndQuery(GL_TIME_ELAPSED);
}

void initShaders() {
	
	// Trivial write-through vertex shader
	const char* srcVert = 
		"#version 330\n"
		"in vec3 inPosition;\n"
		"out vec3 position;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	position = inPosition;\n"
		"}\n";

	std::string srcGeomStr = cgl::Shader::readFile(GEOMETRY_SHADER_FILENAME);

	g_program = createProgram(srcVert, srcGeomStr.c_str());	
	g_attribPosition = glGetAttribLocation(g_program, "inPosition");
}

void initQueries()
{
	// Create query objects
	glGenQueries(QUERY_OBJECTS_TOTAL, g_queries);
}

void init() 
{
	// Obtain geometry
	loadMeshFromFile(MESH_FILENAME, g_vertices, g_triangles);

	// Init query objects
	initQueries();

	// Init shaders
	initShaders();

	// Init feedback vertex data
	initVertexBufferObjects();

	// Init vertex data
	initVertexArrayObjects();
}

void benchmark()
{
	std::vector<glm::vec3> normalsCPU, normalsGPU;
	// Run CPU code
	long temp = clock();
	calcNormalsCPU(g_vertices, g_triangles, normalsCPU);
	std::cout << "CPU wallclock calc time: " << clock()-temp << " seconds" << std::endl;
	
	// Run GPU code
	glFinish();
	temp = clock();
	calcNormalsGPU(normalsGPU);
	std::cout << "GPU wallclock calc and download time: " << clock()-temp << " seconds" << std::endl;

	// Get elapsed time in nanoseconds
	GLuint64 uploadTime, calcTime, downloadTime;
	glGetQueryObjectui64v(g_queries[QUERY_CALC_TIME], GL_QUERY_RESULT, &calcTime);
	glGetQueryObjectui64v(g_queries[QUERY_UPLOAD_TIME], GL_QUERY_RESULT, &uploadTime);
	glGetQueryObjectui64v(g_queries[QUERY_DOWNLOAD_TIME], GL_QUERY_RESULT, &downloadTime);	
	std::cout << "Upload time elapsed: " << uploadTime << " nano-seconds" << std::endl;
	std::cout << "GPU calc time elapsed: " << calcTime << " nano-seconds" << std::endl;
	std::cout << "Download time elapsed: " << downloadTime << " nano-seconds" << std::endl;
	
	// Check correctness of CPU and GPU code
	bool isExact = true;
	for(unsigned int i=0; i<g_triangles.size(); ++i)
	{		
		isExact &= glm::length(normalsCPU[i] - normalsGPU[i]) < 0.0001;		
	}
	assert(isExact);
	if(isExact)
		std::cout << "Correctness verified!" << std::endl;
}

int main(int argc, char **argv) {

	glutInitContextVersion(3,3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_DEBUG);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(512, 512);
	glutCreateWindow("ex21 - Particles Feedback");
	
	// Glew limitation 
	// Ref: http://openglbook.com/glgenvertexarrays-access-violationsegfault-with-glew/
	glewExperimental = GL_TRUE; 
	glewInit();

	init();
	benchmark();
	
	//glutMainLoop();

	return 0;  
}
