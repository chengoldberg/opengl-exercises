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
#include "cgl/gl/common.hpp"

#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))
#define TES_SHADER_FILENAME "../res/shader/ex27.TES.glsl"
#define TCS_SHADER_FILENAME "../res/shader/ex27.TCS.glsl"

enum EVao
{
	VAO_VERTEX,
	VAO_TOTAL
};

enum EBufferObject
{
	VBO_POSITION,
	EBO_CURVES,
	BUFFER_OBJECTS_TOTAL
};

bool g_wireframeEnabled;
bool g_zoomMode;
float g_levelsOuter, g_levelsInner;
glm::ivec2 g_prevMouse;
cgl::Program g_programLines, g_programPoints;
GLuint g_attribPosition, g_uniformModelViewMatrix, g_uniformProjectionMatrix;
GLuint g_vbo[BUFFER_OBJECTS_TOTAL];
GLuint g_vao[VAO_TOTAL];
int g_totalElements;
glm::mat4 g_modelView(1), g_projection;
std::vector<glm::vec3> g_vertices;
glm::vec2 g_cameraRotationXY;
float g_cameraTranslation;

void rotate(double x, double y) 
{
	g_cameraRotationXY.x += x;
	g_cameraRotationXY.y += y;
}

// ============================== Helper Functions =========================

GLuint buildProgram(const char* srcVert, const char* srcTES, const char* srcFrag) 
{
	printf("\nCreating program");

	bool OK = true;

	GLuint vsId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vsId, 1, (const GLchar**) &srcVert, NULL);
	glCompileShader(vsId);
	OK = OK && cgl::checkShaderCompilationStatus(vsId);

	GLuint fsId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fsId, 1, (const GLchar**) &srcFrag, NULL);
	glCompileShader(fsId);
	OK = OK && cgl::checkShaderCompilationStatus(fsId);

	GLuint tesId = glCreateShader(GL_TESS_EVALUATION_SHADER);
	glShaderSource(tesId, 1, (const GLchar**) &srcTES, NULL);
	glCompileShader(tesId);
	OK = OK && cgl::checkShaderCompilationStatus(tesId);

	if(!OK) 
	{
		throw std::exception("Failed to compile, quitting!\n");
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vsId);
	glAttachShader(program, tesId);
	glAttachShader(program, fsId);

	// Now ready to link 
	glLinkProgram(program);

	//glValidateProgram(program);

	if(cgl::checkProgramInfoLog(program) == 0) 
	{
		throw std::exception("Failed to create program from shaders");
	} 
	else 
	{
		std::cout << "Program created!" << std::endl;
	}

	return program;
}

void initVertexBufferObjects()
{
	std::vector<glm::vec3> vertices;
	std::vector<glm::ivec4> curves;
	
	vertices.push_back(glm::vec3(-0.5,0,0));
	vertices.push_back(glm::vec3(-0.5,0.5,0));
	vertices.push_back(glm::vec3(0,0.5,0));
	vertices.push_back(glm::vec3(0.5,0,0));
	curves.push_back(glm::ivec4(0,1,2,3));
	
	float temp[][12] = {{-0.814f,1.000f,0.0f,-0.869f,1.000f,0.0f,-0.914f,0.928f,0.0f,-0.948f,0.783f,0.0f},{-0.948f,0.783f,0.0f,-0.983f,0.638f,0.0f,-1.000f,0.436f,0.0f,-1.000f,0.179f,0.0f},{-1.000f,0.179f,0.0f,-1.000f,0.044f,0.0f,-0.993f,-0.083f,0.0f,-0.978f,-0.205f,0.0f},{-0.978f,-0.205f,0.0f,-0.963f,-0.326f,0.0f,-0.941f,-0.423f,0.0f,-0.913f,-0.493f,0.0f},{-0.913f,-0.493f,0.0f,-0.884f,-0.563f,0.0f,-0.851f,-0.599f,0.0f,-0.815f,-0.599f,0.0f},{-0.815f,-0.599f,0.0f,-0.781f,-0.599f,0.0f,-0.750f,-0.566f,0.0f,-0.721f,-0.503f,0.0f},{-0.721f,-0.503f,0.0f,-0.692f,-0.439f,0.0f,-0.669f,-0.345f,0.0f,-0.653f,-0.222f,0.0f},{-0.653f,-0.222f,0.0f,-0.638f,-0.098f,0.0f,-0.630f,0.041f,0.0f,-0.630f,0.198f,0.0f},{-0.630f,0.198f,0.0f,-0.630f,0.352f,0.0f,-0.637f,0.491f,0.0f,-0.652f,0.612f,0.0f},{-0.652f,0.612f,0.0f,-0.667f,0.734f,0.0f,-0.689f,0.829f,0.0f,-0.718f,0.897f,0.0f},{-0.718f,0.897f,0.0f,-0.747f,0.965f,0.0f,-0.779f,1.000f,0.0f,-0.814f,1.000f,0.0f},{0.520f,0.998f,0.0f,0.482f,0.998f,0.0f,0.448f,0.967f,0.0f,0.418f,0.904f,0.0f},{0.418f,0.904f,0.0f,0.389f,0.841f,0.0f,0.366f,0.744f,0.0f,0.350f,0.614f,0.0f},{0.350f,0.614f,0.0f,0.335f,0.484f,0.0f,0.327f,0.343f,0.0f,0.327f,0.193f,0.0f},{0.327f,0.193f,0.0f,0.327f,0.041f,0.0f,0.335f,-0.097f,0.0f,0.350f,-0.220f,0.0f},{0.350f,-0.220f,0.0f,0.366f,-0.343f,0.0f,0.390f,-0.436f,0.0f,0.421f,-0.501f,0.0f},{0.421f,-0.501f,0.0f,0.452f,-0.566f,0.0f,0.486f,-0.599f,0.0f,0.525f,-0.599f,0.0f},{0.525f,-0.599f,0.0f,0.553f,-0.599f,0.0f,0.580f,-0.579f,0.0f,0.607f,-0.538f,0.0f},{0.607f,-0.538f,0.0f,0.634f,-0.498f,0.0f,0.660f,-0.436f,0.0f,0.685f,-0.356f,0.0f},{0.685f,-0.356f,0.0f,0.685f,-0.165f,0.0f,0.685f,0.026f,0.0f,0.685f,0.217f,0.0f},{0.685f,0.217f,0.0f,0.630f,0.216f,0.0f,0.575f,0.215f,0.0f,0.521f,0.215f,0.0f},{0.521f,0.215f,0.0f,0.521f,0.154f,0.0f,0.521f,0.094f,0.0f,0.521f,0.033f,0.0f},{0.521f,0.033f,0.0f,0.559f,0.033f,0.0f,0.597f,0.033f,0.0f,0.634f,0.033f,0.0f},{0.634f,0.033f,0.0f,0.634f,-0.063f,0.0f,0.634f,-0.158f,0.0f,0.634f,-0.254f,0.0f},{0.634f,-0.254f,0.0f,0.623f,-0.292f,0.0f,0.608f,-0.329f,0.0f,0.587f,-0.363f,0.0f},{0.587f,-0.363f,0.0f,0.566f,-0.397f,0.0f,0.544f,-0.414f,0.0f,0.522f,-0.414f,0.0f},{0.522f,-0.414f,0.0f,0.496f,-0.414f,0.0f,0.472f,-0.391f,0.0f,0.449f,-0.346f,0.0f},{0.449f,-0.346f,0.0f,0.427f,-0.301f,0.0f,0.409f,-0.233f,0.0f,0.397f,-0.141f,0.0f},{0.397f,-0.141f,0.0f,0.385f,-0.048f,0.0f,0.380f,0.067f,0.0f,0.380f,0.205f,0.0f},{0.380f,0.205f,0.0f,0.380f,0.316f,0.0f,0.384f,0.421f,0.0f,0.394f,0.516f,0.0f},{0.394f,0.516f,0.0f,0.400f,0.572f,0.0f,0.408f,0.622f,0.0f,0.419f,0.668f,0.0f},{0.419f,0.668f,0.0f,0.429f,0.714f,0.0f,0.443f,0.753f,0.0f,0.460f,0.781f,0.0f},{0.460f,0.781f,0.0f,0.477f,0.809f,0.0f,0.497f,0.823f,0.0f,0.521f,0.823f,0.0f},{0.521f,0.823f,0.0f,0.540f,0.823f,0.0f,0.558f,0.810f,0.0f,0.574f,0.783f,0.0f},{0.574f,0.783f,0.0f,0.590f,0.756f,0.0f,0.603f,0.720f,0.0f,0.611f,0.675f,0.0f},{0.611f,0.675f,0.0f,0.620f,0.630f,0.0f,0.627f,0.568f,0.0f,0.633f,0.489f,0.0f},{0.633f,0.489f,0.0f,0.648f,0.506f,0.0f,0.663f,0.523f,0.0f,0.679f,0.540f,0.0f},{0.679f,0.540f,0.0f,0.672f,0.644f,0.0f,0.662f,0.728f,0.0f,0.650f,0.793f,0.0f},{0.650f,0.793f,0.0f,0.637f,0.857f,0.0f,0.619f,0.908f,0.0f,0.596f,0.944f,0.0f},{0.596f,0.944f,0.0f,0.574f,0.981f,0.0f,0.548f,0.998f,0.0f,0.520f,0.998f,0.0f},{0.758f,0.971f,0.0f,0.758f,0.457f,0.0f,0.758f,-0.058f,0.0f,0.758f,-0.572f,0.0f},{0.758f,-0.572f,0.0f,0.839f,-0.572f,0.0f,0.919f,-0.572f,0.0f,1.000f,-0.572f,0.0f},{1.000f,-0.572f,0.0f,1.000f,-0.511f,0.0f,1.000f,-0.451f,0.0f,1.000f,-0.390f,0.0f},{1.000f,-0.390f,0.0f,0.936f,-0.390f,0.0f,0.873f,-0.390f,0.0f,0.809f,-0.390f,0.0f},{0.809f,-0.390f,0.0f,0.809f,0.064f,0.0f,0.809f,0.518f,0.0f,0.809f,0.971f,0.0f},{0.809f,0.971f,0.0f,0.792f,0.971f,0.0f,0.775f,0.971f,0.0f,0.758f,0.971f,0.0f},{-0.814f,0.823f,0.0f,-0.789f,0.823f,0.0f,-0.766f,0.797f,0.0f,-0.745f,0.746f,0.0f},{-0.745f,0.746f,0.0f,-0.725f,0.694f,0.0f,-0.709f,0.622f,0.0f,-0.699f,0.528f,0.0f},{-0.699f,0.528f,0.0f,-0.688f,0.434f,0.0f,-0.683f,0.325f,0.0f,-0.683f,0.200f,0.0f},{-0.683f,0.200f,0.0f,-0.683f,0.001f,0.0f,-0.695f,-0.154f,0.0f,-0.720f,-0.262f,0.0f},{-0.720f,-0.262f,0.0f,-0.745f,-0.370f,0.0f,-0.777f,-0.424f,0.0f,-0.815f,-0.424f,0.0f},{-0.815f,-0.424f,0.0f,-0.853f,-0.424f,0.0f,-0.884f,-0.369f,0.0f,-0.910f,-0.262f,0.0f},{-0.910f,-0.262f,0.0f,-0.935f,-0.155f,0.0f,-0.947f,-0.010f,0.0f,-0.947f,0.176f,0.0f},{-0.947f,0.176f,0.0f,-0.947f,0.408f,0.0f,-0.934f,0.575f,0.0f,-0.908f,0.675f,0.0f},{-0.908f,0.675f,0.0f,-0.882f,0.774f,0.0f,-0.851f,0.823f,0.0f,-0.814f,0.823f,0.0f},{-0.445f,0.572f,0.0f,-0.464f,0.572f,0.0f,-0.479f,0.558f,0.0f,-0.492f,0.530f,0.0f},{-0.492f,0.530f,0.0f,-0.505f,0.501f,0.0f,-0.516f,0.459f,0.0f,-0.527f,0.402f,0.0f},{-0.527f,0.402f,0.0f,-0.527f,0.450f,0.0f,-0.527f,0.498f,0.0f,-0.527f,0.547f,0.0f},{-0.527f,0.547f,0.0f,-0.541f,0.547f,0.0f,-0.556f,0.547f,0.0f,-0.570f,0.547f,0.0f},{-0.570f,0.547f,0.0f,-0.570f,0.031f,0.0f,-0.570f,-0.484f,0.0f,-0.570f,-1.000f,0.0f},{-0.570f,-1.000f,0.0f,-0.554f,-1.000f,0.0f,-0.538f,-1.000f,0.0f,-0.522f,-1.000f,0.0f},{-0.522f,-1.000f,0.0f,-0.522f,-0.819f,0.0f,-0.522f,-0.637f,0.0f,-0.522f,-0.456f,0.0f},{-0.522f,-0.456f,0.0f,-0.514f,-0.496f,0.0f,-0.504f,-0.530f,0.0f,-0.492f,-0.557f,0.0f},{-0.492f,-0.557f,0.0f,-0.479f,-0.584f,0.0f,-0.465f,-0.597f,0.0f,-0.449f,-0.597f,0.0f},{-0.449f,-0.597f,0.0f,-0.427f,-0.597f,0.0f,-0.406f,-0.572f,0.0f,-0.387f,-0.523f,0.0f},{-0.387f,-0.523f,0.0f,-0.367f,-0.474f,0.0f,-0.352f,-0.404f,0.0f,-0.342f,-0.313f,0.0f},{-0.342f,-0.313f,0.0f,-0.332f,-0.221f,0.0f,-0.326f,-0.119f,0.0f,-0.326f,-0.004f,0.0f},{-0.326f,-0.004f,0.0f,-0.326f,0.102f,0.0f,-0.331f,0.201f,0.0f,-0.340f,0.291f,0.0f},{-0.340f,0.291f,0.0f,-0.350f,0.380f,0.0f,-0.364f,0.449f,0.0f,-0.382f,0.498f,0.0f},{-0.382f,0.498f,0.0f,-0.400f,0.547f,0.0f,-0.422f,0.572f,0.0f,-0.445f,0.572f,0.0f},{-0.154f,0.572f,0.0f,-0.193f,0.572f,0.0f,-0.224f,0.519f,0.0f,-0.248f,0.415f,0.0f},{-0.248f,0.415f,0.0f,-0.272f,0.311f,0.0f,-0.285f,0.165f,0.0f,-0.285f,-0.023f,0.0f},{-0.285f,-0.023f,0.0f,-0.285f,-0.205f,0.0f,-0.273f,-0.345f,0.0f,-0.249f,-0.446f,0.0f},{-0.249f,-0.446f,0.0f,-0.224f,-0.546f,0.0f,-0.192f,-0.597f,0.0f,-0.151f,-0.597f,0.0f},{-0.151f,-0.597f,0.0f,-0.118f,-0.597f,0.0f,-0.092f,-0.566f,0.0f,-0.071f,-0.503f,0.0f},{-0.071f,-0.503f,0.0f,-0.050f,-0.440f,0.0f,-0.035f,-0.351f,0.0f,-0.027f,-0.237f,0.0f},{-0.027f,-0.237f,0.0f,-0.044f,-0.228f,0.0f,-0.060f,-0.220f,0.0f,-0.077f,-0.211f,0.0f},{-0.077f,-0.211f,0.0f,-0.084f,-0.291f,0.0f,-0.094f,-0.350f,0.0f,-0.106f,-0.387f,0.0f},{-0.106f,-0.387f,0.0f,-0.118f,-0.423f,0.0f,-0.133f,-0.441f,0.0f,-0.151f,-0.441f,0.0f},{-0.151f,-0.441f,0.0f,-0.174f,-0.441f,0.0f,-0.194f,-0.408f,0.0f,-0.209f,-0.343f,0.0f},{-0.209f,-0.343f,0.0f,-0.225f,-0.278f,0.0f,-0.234f,-0.184f,0.0f,-0.235f,-0.061f,0.0f},{-0.235f,-0.061f,0.0f,-0.166f,-0.061f,0.0f,-0.096f,-0.061f,0.0f,-0.026f,-0.061f,0.0f},{-0.026f,-0.061f,0.0f,-0.026f,-0.039f,0.0f,-0.026f,-0.022f,0.0f,-0.026f,-0.011f,0.0f},{-0.026f,-0.011f,0.0f,-0.026f,0.174f,0.0f,-0.038f,0.317f,0.0f,-0.062f,0.419f,0.0f},{-0.062f,0.419f,0.0f,-0.085f,0.520f,0.0f,-0.116f,0.572f,0.0f,-0.154f,0.572f,0.0f},{0.164f,0.572f,0.0f,0.125f,0.572f,0.0f,0.096f,0.510f,0.0f,0.075f,0.387f,0.0f},{0.075f,0.387f,0.0f,0.075f,0.440f,0.0f,0.075f,0.493f,0.0f,0.075f,0.547f,0.0f},{0.075f,0.547f,0.0f,0.061f,0.547f,0.0f,0.046f,0.547f,0.0f,0.032f,0.547f,0.0f},{0.032f,0.547f,0.0f,0.032f,0.174f,0.0f,0.032f,-0.199f,0.0f,0.032f,-0.572f,0.0f},{0.032f,-0.572f,0.0f,0.048f,-0.572f,0.0f,0.064f,-0.572f,0.0f,0.080f,-0.572f,0.0f},{0.080f,-0.572f,0.0f,0.080f,-0.368f,0.0f,0.080f,-0.164f,0.0f,0.080f,0.040f,0.0f},{0.080f,0.040f,0.0f,0.080f,0.183f,0.0f,0.087f,0.280f,0.0f,0.102f,0.331f,0.0f},{0.102f,0.331f,0.0f,0.117f,0.382f,0.0f,0.134f,0.407f,0.0f,0.154f,0.407f,0.0f},{0.154f,0.407f,0.0f,0.167f,0.407f,0.0f,0.178f,0.396f,0.0f,0.188f,0.373f,0.0f},{0.188f,0.373f,0.0f,0.197f,0.350f,0.0f,0.203f,0.320f,0.0f,0.207f,0.282f,0.0f},{0.207f,0.282f,0.0f,0.211f,0.244f,0.0f,0.213f,0.186f,0.0f,0.213f,0.109f,0.0f},{0.213f,0.109f,0.0f,0.213f,-0.118f,0.0f,0.213f,-0.345f,0.0f,0.213f,-0.572f,0.0f},{0.213f,-0.572f,0.0f,0.229f,-0.572f,0.0f,0.244f,-0.572f,0.0f,0.260f,-0.572f,0.0f},{0.260f,-0.572f,0.0f,0.260f,-0.343f,0.0f,0.260f,-0.114f,0.0f,0.260f,0.115f,0.0f},{0.260f,0.115f,0.0f,0.260f,0.203f,0.0f,0.260f,0.266f,0.0f,0.258f,0.301f,0.0f},{0.258f,0.301f,0.0f,0.255f,0.355f,0.0f,0.250f,0.401f,0.0f,0.243f,0.441f,0.0f},{0.243f,0.441f,0.0f,0.236f,0.480f,0.0f,0.225f,0.511f,0.0f,0.211f,0.535f,0.0f},{0.211f,0.535f,0.0f,0.197f,0.559f,0.0f,0.181f,0.572f,0.0f,0.164f,0.572f,0.0f},{-0.450f,0.424f,0.0f,-0.429f,0.424f,0.0f,-0.412f,0.389f,0.0f,-0.397f,0.318f,0.0f},{-0.397f,0.318f,0.0f,-0.382f,0.247f,0.0f,-0.375f,0.140f,0.0f,-0.375f,-0.003f,0.0f},{-0.375f,-0.003f,0.0f,-0.375f,-0.152f,0.0f,-0.383f,-0.263f,0.0f,-0.398f,-0.334f,0.0f},{-0.398f,-0.334f,0.0f,-0.413f,-0.406f,0.0f,-0.431f,-0.441f,0.0f,-0.452f,-0.441f,0.0f},{-0.452f,-0.441f,0.0f,-0.473f,-0.441f,0.0f,-0.490f,-0.407f,0.0f,-0.505f,-0.338f,0.0f},{-0.505f,-0.338f,0.0f,-0.520f,-0.269f,0.0f,-0.527f,-0.163f,0.0f,-0.527f,-0.019f,0.0f},{-0.527f,-0.019f,0.0f,-0.527f,0.125f,0.0f,-0.519f,0.235f,0.0f,-0.503f,0.311f,0.0f},{-0.503f,0.311f,0.0f,-0.488f,0.386f,0.0f,-0.470f,0.424f,0.0f,-0.450f,0.424f,0.0f},{-0.161f,0.415f,0.0f,-0.158f,0.416f,0.0f,-0.156f,0.415f,0.0f,-0.153f,0.415f,0.0f},{-0.153f,0.415f,0.0f,-0.129f,0.415f,0.0f,-0.109f,0.379f,0.0f,-0.094f,0.306f,0.0f},{-0.094f,0.306f,0.0f,-0.084f,0.259f,0.0f,-0.078f,0.189f,0.0f,-0.076f,0.095f,0.0f},{-0.076f,0.095f,0.0f,-0.128f,0.095f,0.0f,-0.181f,0.095f,0.0f,-0.233f,0.095f,0.0f},{-0.233f,0.095f,0.0f,-0.231f,0.193f,0.0f,-0.223f,0.269f,0.0f,-0.208f,0.328f,0.0f},{-0.208f,0.328f,0.0f,-0.195f,0.379f,0.0f,-0.179f,0.409f,0.0f,-0.161f,0.415f,0.0f}};

	// Create identifiers
	glGenBuffers(BUFFER_OBJECTS_TOTAL, g_vbo);

	// Transfer data
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_POSITION]);
	//glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, sizeof(temp), temp, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[EBO_CURVES]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, curves.size()*sizeof(glm::ivec4), curves.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	g_totalElements = sizeof(temp)/sizeof(float)/3;
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
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[EBO_CURVES]);
	}
	glBindVertexArray(0);
}

void drawMesh() 
{
	glBindVertexArray(g_vao[VAO_VERTEX]);
	{
        glPatchParameteri(GL_PATCH_VERTICES, 4);
		float outer[] = {1, /*g_levelsOuter*/ 10};
		float inner[] = {0};
        glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outer);
        glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL , inner);
		//glDrawElements(GL_PATCHES, g_totalElements, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
		glDrawArrays(GL_PATCHES, 0, g_totalElements);
	}
	glBindVertexArray(0);
}

void initShaders() {
	
	const char* srcVert = 
		"#version 330\n"
		"in vec3 aPosition;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	gl_Position = vec4(aPosition,1);\n"
		"}\n";

	const char* srcFrag = 
		"#version 330\n"
		"out vec4 oColor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	oColor = vec4(1,1,1,1);\n"		
		"}\n";

	std::string srcTES  = cgl::Shader::readFile(TES_SHADER_FILENAME);
	std::string srcTCS  = cgl::Shader::readFile(TCS_SHADER_FILENAME);

	std::string srcTESPoints(srcTES);
	srcTESPoints.replace(srcTESPoints.find("/*, point_mode*/"), sizeof("/*, point_mode*/"), ", point_mode");
	try
	{		
		{
			std::vector<cgl::Shader> shaders;
			shaders.push_back(cgl::Shader(GL_VERTEX_SHADER, srcVert));
			shaders.push_back(cgl::Shader(GL_FRAGMENT_SHADER, srcFrag));
			shaders.push_back(cgl::Shader(GL_TESS_CONTROL_SHADER, srcTCS));
			shaders.push_back(cgl::Shader(GL_TESS_EVALUATION_SHADER, srcTES));
			g_programLines.build(shaders);		
		}
		{
			std::vector<cgl::Shader> shaders;
			shaders.push_back(cgl::Shader(GL_VERTEX_SHADER, srcVert));
			shaders.push_back(cgl::Shader(GL_FRAGMENT_SHADER, srcFrag));
			shaders.push_back(cgl::Shader(GL_TESS_CONTROL_SHADER, srcTCS));
			shaders.push_back(cgl::Shader(GL_TESS_EVALUATION_SHADER, srcTESPoints));
			g_programPoints.build(shaders);	
		}
	}
	catch(std::exception ex)
	{
		std::cout << ex.what();
		throw ex;
	}
	g_attribPosition = glGetAttribLocation(g_programLines.getId(), "aPosition");
	g_uniformModelViewMatrix = glGetUniformLocation(g_programLines.getId(), "uModelViewMatrix");
	g_uniformProjectionMatrix = glGetUniformLocation(g_programLines.getId(), "uProjectionMatrix");
}


void init() 
{	
	glEnable(GL_CULL_FACE);
	g_wireframeEnabled = true;
	g_levelsOuter = 1;
	g_levelsInner = 1;
	g_zoomMode = false;

	// Init shaders
	initShaders();

	// Set background color to gray
	glClearColor(0.25, 0.25, 0.25, 1);

	// Init states		
	glPointSize(2);

	// Init feedback vertex data
	initVertexBufferObjects();

	// Init vertex data
	initVertexArrayObjects();

	// Place camera	
	//g_modelView = glm::translate(g_modelView, glm::vec3(0,0,-1.5));
}
 
/**
* Create camera transformation such that the model is rotated around the
* world's X-axis and Y-axis. 
*/
void setupCamera() 
{	
	g_modelView = glm::scale(g_modelView, glm::vec3(1+g_cameraTranslation));
	//g_modelView[3] *= 1+g_cameraTranslation;
	g_modelView[3][0] *= 1+g_cameraTranslation;
	g_modelView[3][1] *= 1+g_cameraTranslation;

	g_modelView = glm::translate(g_modelView, glm::vec3(-g_cameraRotationXY.x, g_cameraRotationXY.y, 0));

	g_cameraRotationXY = glm::ivec2(0,0);
	g_cameraTranslation = 0;
}

void display(void) 
{
	// Clear FrameBuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		

	//glPolygonMode(GL_FRONT_AND_BACK, g_wireframeEnabled?GL_LINE:GL_POINT);
	
	// Apply shaders
	if(g_wireframeEnabled)
	{
		g_programLines.use();
	}
	else
	{
		g_programPoints.use();				
	}

	// Create camera transformation
	setupCamera();		
	// Save camera transformation

	//glm::mat4 modelView = glm::translate(g_modelView, 2.0f*glm::vec3(-g_texSize.x/2.0f, -g_texSize.y/2.0f, 0));

	// Update model-view matrix
	glUniformMatrix4fv(g_uniformModelViewMatrix, 1, false, glm::value_ptr(g_modelView));	
	
	// draw Meshes
	drawMesh();

	// Swap double buffer
	glutSwapBuffers();
}

void reshape(int width, int height) {

	// Setup viewport
	glViewport(0,0,width,height);

	// Setup projection transformation
	g_projection = glm::ortho(-5.0f,5.0f,-5.0f, 5.0f, -50.0f, 50.0f);	
	//g_projection = glm::ortho(0.0f,100.0f,0.0f, 100.0f, -1.0f, 1.0f);	

	//g_projection = glm::perspective(90.0f, (float)width/height, 0.01f, 10.0f);
	//initFrameBufferObject(width, height);

	// Create projection transformation	
	g_programLines.use();
	glUniformMatrix4fv(g_uniformProjectionMatrix, 1, false, glm::value_ptr(g_projection));
	g_programPoints.use();
	glUniformMatrix4fv(g_uniformProjectionMatrix, 1, false, glm::value_ptr(g_projection));
	glUseProgram(0);
}

void motionFunc(int x, int y) 
{
	// Calc difference from previous mouse location
	glm::ivec2 prev = g_prevMouse;
	int dx = prev.x - x;
	int dy = prev.y - y;

	if(g_zoomMode)
	{
		g_cameraTranslation += dy/100.0f;
	}
	else
	{
		// Rotate model
		rotate(dx/100.0f, dy/100.0f);
	}

	// Remember mouse location 
	g_prevMouse = glm::ivec2(x,y);	
}

void mouseFunc(int button, int state, int x, int y) 
{
	if(button == GLUT_LEFT_BUTTON)
	{
		g_zoomMode = false;
		g_prevMouse = glm::ivec2(x,y);
	}
	if(button == GLUT_RIGHT_BUTTON)
	{
		g_zoomMode = true;
		g_prevMouse = glm::ivec2(x,y);	
	}
}

void keyboardSpecialFunc(int key, int x, int y) 
{
	switch(key) {
	case GLUT_KEY_DOWN: 
		g_levelsOuter -= 1;
		break;
	case GLUT_KEY_UP:
		g_levelsOuter += 1;
		break;
	case GLUT_KEY_LEFT: 
		g_levelsInner -= 1;
		break;
	case GLUT_KEY_RIGHT:
		g_levelsInner += 1;
		break;
	case GLUT_KEY_F1:
		g_wireframeEnabled ^= true;
		break;
	}
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

void timer(int value) 
{
	glutPostRedisplay();
	glutTimerFunc(16,timer,0);
}

int main(int argc, char **argv) {

	glutInitContextVersion(4,0);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_DEBUG);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(512, 512);

	int windowId = glutCreateWindow("ex27 - Curve Tessellation");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);	
	glutMotionFunc(motionFunc);
	glutMouseFunc(mouseFunc);
	glutSpecialFunc(keyboardSpecialFunc);
	glutKeyboardFunc(keyboardFunc);
	glutTimerFunc(16,timer,0);
	//glutInit
	//glutFullScreen();

	// Glew limitation 
	// Ref: http://openglbook.com/glgenvertexarrays-access-violationsegfault-with-glew/
	glewExperimental = GL_TRUE; 
	glewInit();

	try
	{
		init();
	}
	catch(std::exception ex)
	{
		// Wait key
		glutDestroyWindow(windowId);
		std::getchar();
		return -1;
	}

	std::cout << "Usage:" << std::endl;
	std::cout << "left arrow/right arrow		Draw and process less/more particles" << std::endl;
	std::cout << "down arrow/up arrow			Speed up or down particles" << std::endl;
	std::cout << "Use mouse to look around" << std::endl;

	glutMainLoop();

	return 0;  
}
