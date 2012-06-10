#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#include <GL/glut.h>
#include "Vec.h"

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

	int		m_renderMode;

	float 	*m_vertices;		// Array of shape's vertices
	unsigned int*	m_faces;	// Array of shape's faces
	int 	m_verticesAmount;	// Amount of vectrices 
	int		m_facesAmount;		// Amount of faces 
	Vec		*m_normals;			// Array of mesh's normals (one per vertex)

	//TODO: Add fields as you need (display list object, buffer object...)
};

