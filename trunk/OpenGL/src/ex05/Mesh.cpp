/*
 * Copyright (C) 2010  Chen Goldberg
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
	m_displayList = glGenLists(1);
	glNewList(m_displayList, GL_COMPILE);
	renderImmediateMode();
	glEndList();
}

void Mesh::renderDisplayList() {
	glCallList(m_displayList);
}

void Mesh::initVertexArray() {

	// Hopper
	// Nothing needed!

	// Sequential
	m_verticesArrayPacked = (float*) malloc(m_facesAmount * sizeof(float) * 3 * 3);
	m_normalsArrayPacked = (float*) malloc(m_facesAmount * sizeof(float) * 3 * 3);
	int vertexIndex = 0;
	for (int i=0; i < m_facesAmount; ++i){
		for(int j=0; j < 3;++j){
			vertexIndex = m_faces[3*i + j];
			for(int k=0;k<3;++k) {
				m_verticesArrayPacked[i*3*3+j*3+k] = m_vertices[vertexIndex*3+k];
				m_normalsArrayPacked[i*3*3+j*3+k] = m_normals[vertexIndex].get(k);
			}
		}
	}	
}

void Mesh::renderVertexArray() {

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	switch(m_renderMode) {
	case RENDER_VERTEX_ARRAY_HOP:
		glVertexPointer(3, GL_FLOAT, 0, m_vertices);
		glNormalPointer(GL_FLOAT, 0, m_normals);
		glDrawElements(GL_TRIANGLES, m_facesAmount*3, GL_UNSIGNED_INT, m_faces); //Needs to be unsigned!!!
		break;
	case RENDER_VERTEX_ARRAY_SEQ:
		glVertexPointer(3, GL_FLOAT, 0, m_verticesArrayPacked);
		glNormalPointer(GL_FLOAT, 0, m_normalsArrayPacked);
		glDrawArrays(GL_TRIANGLES, 0, m_facesAmount*3);
		break;
	default:
		int vertexIndex = 0;
		glBegin(GL_TRIANGLES);
		{
			for (int i=0; i < m_facesAmount; ++i){
				for(int j=0; j < 3;++j){
					vertexIndex = m_faces[3*i + j];
					glArrayElement(vertexIndex);
				}
			}
		}
		glEnd();
	}			
}

void Mesh::initVBO() {

	// Hopper
	glGenBuffers(1, &m_vbObject);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbObject);
	glBufferData(GL_ARRAY_BUFFER, m_verticesAmount*3*sizeof(float), m_vertices, GL_STATIC_DRAW);	//Note: need to pass number of bytes. If multi-dimentional array then sizeof is enough. else you need calculate it yourself.
	
	glGenBuffers(1, &m_nbObject);
	glBindBuffer(GL_ARRAY_BUFFER, m_nbObject);
	glBufferData(GL_ARRAY_BUFFER, m_verticesAmount*3*sizeof(float), m_normals, GL_STATIC_DRAW);	

	glGenBuffers(1, &m_ibObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_facesAmount*3*sizeof(unsigned int), m_faces, GL_STATIC_DRAW);
	
	// Sequential
	glGenBuffers(1, &m_vabObject);
	glBindBuffer(GL_ARRAY_BUFFER, m_vabObject);
	glBufferData(GL_ARRAY_BUFFER, m_facesAmount*3*3*sizeof(float), m_verticesArrayPacked, GL_STATIC_DRAW);

	glGenBuffers(1, &m_nabObject);
	glBindBuffer(GL_ARRAY_BUFFER, m_nabObject);
	glBufferData(GL_ARRAY_BUFFER, m_facesAmount*3*3*sizeof(float), m_normalsArrayPacked, GL_STATIC_DRAW);

	// Important: deselect buffer object!
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::renderVBO() {

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	switch(m_renderMode) {
	case RENDER_VBO_HOP:
		glBindBuffer(GL_ARRAY_BUFFER, m_vbObject);
		glVertexPointer(3, GL_FLOAT, 0, BUFFER_OFFSET(0));
		glBindBuffer(GL_ARRAY_BUFFER, m_nbObject);
		glNormalPointer(GL_FLOAT, 0, BUFFER_OFFSET(0));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibObject);
		glDrawElements(GL_TRIANGLES, m_facesAmount*3, GL_UNSIGNED_INT, BUFFER_OFFSET(0));		
		break;
	case RENDER_VBO_SEQ:
		glBindBuffer(GL_ARRAY_BUFFER, m_vabObject);
		glVertexPointer(3, GL_FLOAT, 0, BUFFER_OFFSET(0));
		glBindBuffer(GL_ARRAY_BUFFER, m_nabObject);
		glNormalPointer(GL_FLOAT, 0, BUFFER_OFFSET(0));

		glDrawArrays(GL_TRIANGLES, 0, m_facesAmount*3);		
		break;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
