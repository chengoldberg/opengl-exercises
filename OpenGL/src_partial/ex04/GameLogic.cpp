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