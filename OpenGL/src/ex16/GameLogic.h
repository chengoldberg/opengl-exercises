#include <math.h>

/**
* Simple game where player walks on a plane with three rooms. 
* Supports wall collisions. 
*
*/

class GameLogic {

public:

	float rotX, rotY;

	GameLogic() 
	{
		rotX = 0;
		rotY = 0;
	}

	/**
	* Update game model for one turn; Advance player.
	*/
	void update() {

	}

	void rotate(double x, double y) 
	{
		rotX += x;
		rotY += y;
	}
};


