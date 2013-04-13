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
#include <math.h>

#define HOR_TOTAL 18
#define VER_TOTAL 16
#define BOARD_WIDTH 14
#define BOARD_HEIGHT 20

/**
* Simple game where player walks on a plane with three rooms. 
* Supports wall collisions. 
*
*/
class GameLogic {

public:

	double angle;

	bool _isMoveForward;
	bool _isMoveBackward;
	bool _isTurnLeft;
	bool _isTurnRight;

	double loc[2];

	static int horWalls[18][2];
	static int verWalls[16][2];

	bool horWallsTable[BOARD_WIDTH+1][BOARD_HEIGHT+1];
	bool verWallsTable[BOARD_WIDTH+1][BOARD_HEIGHT+1];

	GameLogic() {
		angle = 0;		
		loc[0]=8;loc[1]=8;

		for(int i=0;i<18;++i) {
			int* cell = horWalls[i];
			horWallsTable[cell[0]][cell[1]] = true;
		}
		for(int i=0;i<16;++i) {
			int* cell = verWalls[i];		
			verWallsTable[cell[0]][cell[1]] = true;
		}
		for(int i = 0;i<BOARD_WIDTH;++i) {
			horWallsTable[i][0] = true;
			horWallsTable[i][BOARD_HEIGHT] = true;
		}
		for(int i = 0;i<BOARD_HEIGHT;++i) {
			verWallsTable[0][i] = true;
			verWallsTable[BOARD_WIDTH][i] = true;
		}
	}

	/**
	* Update game model for one turn; Advance player.
	*/
	void update() {
		if(_isMoveForward || _isMoveBackward) {
			double dx = cos(angle);
			double dy = sin(angle);

			if(_isMoveForward) {
				loc[0] -= dx*0.1;
				loc[1] -= dy*0.1;
			}
			if(_isMoveBackward) {
				loc[0] += dx*0.1;
				loc[1] += dy*0.1;
			}			

			int curX = (int) floor(loc[0]);
			int curY = (int) floor(loc[1]);
			double offX = loc[0]-curX;
			double offY = loc[1]-curY;

			double pad = 0.2;
			if(offY<pad && horWallsTable[curX][curY])
				loc[1] = curY + pad;
			if(offY>1-pad && horWallsTable[curX][curY+1])
				loc[1] = curY + 1 - pad;
			if(offX<pad && verWallsTable[curX][curY])
				loc[0] = curX + pad;
			if(offX>1-pad && verWallsTable[curX+1][curY])
				loc[0] = curX + 1 - pad;							
		}
		if(_isTurnLeft)
			angle -= 0.075;
		if(_isTurnRight)
			angle += 0.075;		
	}

	bool isMoveForward() {
		return _isMoveForward;
	}

	void setMoveForward(bool value) {
		this->_isMoveForward = value;
	}

	bool isMoveBackward() {
		this->_isMoveBackward;
	}

	void setMoveBackward(bool value) {
		this->_isMoveBackward = value;
	}

	bool isTurnLeft() {
		this->_isTurnLeft;
	}

	void setTurnLeft(bool value) {
		this->_isTurnLeft = value;		
	}

	bool isTurnRight() {
		this->_isTurnRight;
	}

	void setTurnRight(bool value) {
		this->_isTurnRight = value;
	}

	double* getPlayerLoc() {
		return loc;
	}

	void setPlayerLoc(double* loc) {		
		loc = loc;
	}

	double getAngle() {
		return angle;
	}

	void setAngle(double angle) {
		this->angle = angle;
	}
	/*
	int** GameLogic::getHorWaslls() {
		//return this->horWalls;
		return GameLogic::horWalls;
	}

	int** GameLogic::getVerWalls() {
		return verWalls;
	}
	*/
	int getBoardWidth() {
		return BOARD_WIDTH;
	}

	int getBoardHeight() {
		return BOARD_HEIGHT;
	}
};


