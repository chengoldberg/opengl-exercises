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

#ifndef __SKYBOX_H_INCLUDED__
#define __SKYBOX_H_INCLUDED__


class CSkybox
{
public:
	CSkybox();
	~CSkybox();

	void init(float size, char *dir);
	
	void Render(float cameraX, float cameraY, float cameraZ);

	void Release();

	enum {
		SKY_TOP,
		SKY_BOTTOM,
		SKY_FRONT,
		SKY_BACK,
		SKY_LEFT,
		SKY_RIGHT
	};

protected:
	unsigned int	m_textures[6];		// 6 texture objects
	float			m_size;
	char*			m_directory;
public:
	unsigned int	m_tex;

private:
	void loadTexture(char* filename, int index);
};

#endif // __SKYBOX_H_INCLUDED__

