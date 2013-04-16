#include <math.h>

#define HOR_TOTAL 18
#define VER_TOTAL 16
#define BOARD_WIDTH (14*4)
#define BOARD_HEIGHT (20*4)

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
	bool _isStrafeLeft;
	bool _isStrafeRight;

	double loc[2];

	GameLogic() {
		angle = M_PI/2;		
		loc[0]=0;loc[1]=8;
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
		}
		if(_isStrafeLeft || _isStrafeRight) {
			double dx = cos(angle-M_PI/2.0);
			double dy = sin(angle-M_PI/2.0);

			if(_isStrafeLeft) {
				loc[0] -= dx*0.1;
				loc[1] -= dy*0.1;
			}
			if(_isStrafeRight) {
				loc[0] += dx*0.1;
				loc[1] += dy*0.1;
			}			
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

	void setStrafeRight(bool value) {
		this->_isStrafeRight = value;
	}

	void setStrafeLeft(bool value) {
		this->_isStrafeLeft = value;
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


