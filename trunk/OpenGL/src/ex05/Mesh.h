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

#pragma once

#include <GL/glut.h>
#include "Vec.h"

#define _CRT_SECURE_NO_DEPRECATE

enum {
	RENDER_IMMEDIATE,
	RENDER_DISPLAY_LIST,
	RENDER_VERTEX_ARRAY_HOP,
	RENDER_VERTEX_ARRAY_SEQ,
	RENDER_VBO_HOP,
	RENDER_VBO_SEQ
};

/* 
 * Repsresnting a full 3D shape (mesh) which useually loaded from 
 * an .OFF file, or a result of decompressing.
 */
class Mesh {
public:

	Mesh() {	
		m_vertices = NULL;
		m_faces = NULL;		
		m_verticesAmount = 0;
		m_facesAmount = 0;
		m_normals = NULL;
		m_renderMode = RENDER_IMMEDIATE;
	}

	~Mesh(){}

	void initAll() {
		genNormals();
		initDisplayList();
		initVertexArray();
		initVBO();
	}

	virtual bool loadFromFile(const char *fileName);

	virtual void genNormals();

	virtual void initDisplayList();
	virtual void initVertexArray();
	virtual void initVBO();

	virtual void render();

	virtual void renderImmediateMode();
	virtual void renderDisplayList();
	virtual void renderVertexArray();
	virtual void renderVBO();

	void setRenderMode(int val) {
		m_renderMode = val;
	}

protected:

	GLuint	m_displayList;
	float 	*m_vertices;		// Array of shape's vertices
	unsigned int*	m_faces;	// Array of shape's faces
	int 	m_verticesAmount;	// Amount of vectrices 
	int		m_facesAmount;		// Amount of faces 
	Vec		*m_normals;			// Array of mesh's normals (one per vertex)

	// Data in vertex Array format
	float*	m_verticesArrayPacked;
	float*	m_normalsArrayPacked;

	// Buffer objects
	GLuint m_vbObject;
	GLuint m_nbObject;
	GLuint m_ibObject;
	GLuint m_vabObject;	
	GLuint m_nabObject;	

	int		m_renderMode;
};

