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

#include "GameLogic.h"

int GameLogic::horWalls[18][2] = {
	{1,1},
	{2,1},
	{3,1},
	{4,1},

	{5,1},
	{6,1},
	{7,1},
	{8,1},

	{9,1},
	{10,1},
	{11,1},
	{12,1},

	{1,5},
	{4,5},

	{5,5},
	{8,5},

	{9,5},
	{12,5}				
};

int GameLogic::verWalls[16][2] = {
	{1,1},
	{1,2},
	{1,3},
	{1,4},

	{5,1},
	{5,2},
	{5,3},
	{5,4},

	{9,1},
	{9,2},
	{9,3},
	{9,4},

	{13,1},
	{13,2},
	{13,3},
	{13,4}				
};		
