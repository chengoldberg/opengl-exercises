#define _CRT_SECURE_NO_WARNINGS
#include "GL/glew.h"
#include "Mesh.h"

#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))

bool Mesh::loadFromFile(const char *fileName) {

	int i;
	FILE *fp;

	if((fp = fopen (fileName, "rb"))==NULL){
		printf("File not found: %s\n", fileName);
		return false;
	}
	printf("Loaded successfully: %s\n", fileName);

	fscanf(fp, "OFF %d %d 0", &m_verticesAmount, &m_facesAmount);

	m_vertices	= (float*)	malloc(3 * m_verticesAmount	*	sizeof(float));
	m_faces		= (unsigned int*)	malloc(3 * m_facesAmount	*	sizeof(unsigned int));

	for (i=0; i < m_verticesAmount; ++i)
		fscanf(fp, "%f %f %f", 
		&(m_vertices[3*i+0]), 
		&(m_vertices[3*i+1]),
		&(m_vertices[3*i+2]));

	for (i=0; i < m_facesAmount; ++i)
		fscanf(fp, " 3 %d %d %d", 
		&(m_faces[3*i]), 
		&(m_faces[3*i+2]), 
		&(m_faces[3*i+1]));	

	fclose(fp);

	return true;
}

void Mesh::genNormals() {

	int vertexIndex = 0;
	int count;

	m_normals = (Vec*) malloc(m_verticesAmount * sizeof(Vec));
	int *normalsCount = (int *) malloc(m_verticesAmount * sizeof(int));

	for(int i = 0;i<m_verticesAmount;++i)
		normalsCount[i] = 0;

	Vec vec[3];
	Vec plane[2];
	Vec pen;

	for (int i=0; i < m_facesAmount; ++i){

		vertexIndex = m_faces[3*i + 0];
		float x1 = m_vertices[3*vertexIndex + 0];
		float y1 = m_vertices[3*vertexIndex + 1];
		float z1 = m_vertices[3*vertexIndex + 2];

		vertexIndex = m_faces[3*i + 1];
		float x2 = m_vertices[3*vertexIndex + 0];
		float y2 = m_vertices[3*vertexIndex + 1];
		float z2 = m_vertices[3*vertexIndex + 2];

		vertexIndex = m_faces[3*i + 2];
		float x3 = m_vertices[3*vertexIndex + 0];
		float y3 = m_vertices[3*vertexIndex + 1];
		float z3 = m_vertices[3*vertexIndex + 2];

		pen.set(
			y1 * (z2 - z3) + y2 * (z3 - z1) + y3 * (z1 - z2),
			z1 * (x2 - x3) + z2 * (x3 - x1) + z3 * (x1 - x2),
			x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));

		pen.normalize();

		for(int j=0; j < 3;++j){
			vertexIndex = m_faces[3*i + j];			
			count = normalsCount[vertexIndex]++;			
			m_normals[vertexIndex].averageSum(&pen, count);
		}
	}

	for(int i=0;i<m_verticesAmount;++i)
		m_normals[vertexIndex].normalize();
}

void Mesh::render(){
	switch(m_renderMode) {
	case RENDER_IMMEDIATE:
		renderImmediateMode();
		break;
	case RENDER_DISPLAY_LIST:
		renderDisplayList();
		break;
	case RENDER_VERTEX_ARRAY_HOP:
	case RENDER_VERTEX_ARRAY_SEQ:
		renderVertexArray();
		break;
	case RENDER_VBO_HOP:
	case RENDER_VBO_SEQ:
		renderVBO();
		break;
	}	
}

void Mesh::renderImmediateMode(){

	int vertexIndex = 0;

	glBegin(GL_TRIANGLES);
	{
		for (int i=0; i < m_facesAmount; ++i){
			for(int j=0; j < 3;++j){
				vertexIndex = m_faces[3*i + j];

				glNormal3fv(m_normals[vertexIndex].get());
				glVertex3f(
					m_vertices[3*vertexIndex + 0],
					m_vertices[3*vertexIndex + 1],
					m_vertices[3*vertexIndex + 2]);
			}
		}
	}
	glEnd();
}

void Mesh::initDisplayList() {
	//TODO:
}

void Mesh::renderDisplayList() {
	//TODO:
}

void Mesh::initVertexArray() {

	// Hopper
	// Nothing needed!

	// Sequential
	//TODO:
}

void Mesh::renderVertexArray() {

	switch(m_renderMode) {
	case RENDER_VERTEX_ARRAY_HOP:
		//TODO:
		break;
	case RENDER_VERTEX_ARRAY_SEQ:
		//TODO:
		break;
	}			
}

void Mesh::initVBO() {

	// Hopper
	//TODO:	
	
	// Sequential
	//TODO:
}

void Mesh::renderVBO() {

	switch(m_renderMode) {
	case RENDER_VBO_HOP:
		//TODO:
		break;
	case RENDER_VBO_SEQ:
		//TODO:
		break;
	}
}
