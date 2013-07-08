#include <math.h>
#include <stdlib.h>
#include <vector>

#define OBJECTS_TOTAL 16

/**
* Simple game where player walks on a plane with three rooms. 
* Supports wall collisions. 
*
*/

float Colors[][3] = {
	{0,0,1},
	{0,1,0},
	{0,1,1},
	{1,0,0},
	{1,0,1},
	{1,1,0},
	{1,0.5,0.5},
	{0.5,0.5,1},
};

class Object 
{
public:
	bool isGL;
	float pos[4];
	int colorIndex;
};

class GameLogic 
{

public:

	float rotX, rotY;
	std::vector<Object> objects;
	int raycastTotal;

	GameLogic() 
	{		
		rotX = 0;
		rotY = 0;
		raycastTotal = 0;
		init();
	}

	void init()
	{
		objects.resize(OBJECTS_TOTAL);
		for(int i=0; i<OBJECTS_TOTAL; ++i)
		{
			float r = ((float)i)/OBJECTS_TOTAL;
			float x = 10*cos(r*2*M_PI);
			float y = 10*sin(r*2*M_PI);	
			//float z = sin(r*4*M_PI);
			float z = 0;
			objects[i].pos[0] = x;
			objects[i].pos[1] = z;
			objects[i].pos[2] = y;
			objects[i].pos[3] = 1;
			objects[i].isGL = i&0x1;
			objects[i].colorIndex = (i/2)%8;
			raycastTotal += !objects[i].isGL;
		}
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


