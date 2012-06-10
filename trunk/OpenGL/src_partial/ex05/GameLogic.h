#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <vector>
#include "Vec.h"
#include <math.h>

#define RANDOM ((double)rand() / ((double)(RAND_MAX)+(double)(1)))

// Bezier curve definition
static double p[4][2] = {
	{0,0},
	{20,0},
	{15,25},
	{30,6}	
};
double g_time;

//
// Represent an object flying on a Bezier curve
//
class Airplane {

private:
public:
	double startTime;
	double x,y,angle; 
	Vec pigment;

	Airplane() {}

	Airplane(Airplane* a) {}

	Airplane(double startTime) {
		this->setStartTime(startTime);
		pigment.set((float)RANDOM,(float)RANDOM,(float)RANDOM);
	}

	void setStartTime(double startTime) {
		this->startTime = startTime;
	}

	double getStartTime() {
		return startTime;
	}

	//
	// Advance airplane and set new angle.
	//
	void update() {

		double u = (g_time - startTime) -(int)(g_time - startTime);

		// Calc new position
		double c0 = (1-u)*(1-u)*(1-u)*p[0][0]+ 3*(1-u)*(1-u)*u*p[1][0] + 3*(1-u)*u*u*p[2][0] + u*u*u*p[3][0];
		double c1 = (1-u)*(1-u)*(1-u)*p[0][1]+ 3*(1-u)*(1-u)*u*p[1][1] + 3*(1-u)*u*u*p[2][1] + u*u*u*p[3][1];
		x = c0;
		y = c1;

		// Calc new derivative
		double d0 = 3*(p[1][0]-p[0][0])*(1-u)*(1-u) + 
			3*(p[2][0]-p[1][0])*2*u*(1-u) + 
			3*(p[3][0]-p[2][0])*u*u;
		double d1 = 3*(p[1][1]-p[0][1])*(1-u)*(1-u) + 
			3*(p[2][1]-p[1][1])*2*u*(1-u) + 
			3*(p[3][1]-p[2][1])*u*u;

		// Transform derivative to degrees 
		angle = 90-atan2(d0,d1)/M_PI*180;			
	}
};


//
// Simple game that moves airplanes on a curve
//
class GameLogic {
public:
	double timeStep;
	std::vector<Airplane*> airplanes;

public:

	GameLogic() {
		timeStep = 0.003;
		g_time = 0;
		addAirplane();
	}

	//
	// Update game model for one turn
	//
	void update() {

		// Advance time counter
		g_time += timeStep;

		for(unsigned int i=0;i<airplanes.size();++i) {
			airplanes[i]->update();
		}
	}

	void addAirplane() {
		airplanes.insert(airplanes.end(),new Airplane(g_time));
		printf("Added airplane - now has %d\n", airplanes.size());
	}

	void removeAirplane() {
		if(airplanes.size()>1)
			airplanes.pop_back();
	}

	void speedUp() {
		timeStep *= 2;
	}

	void speedDown() {
		timeStep /= 2;
	}

	std::vector<Airplane*> getAirplanes() {
		return airplanes;
	}

	//
	// Return bezier curve position at parameter value u
	//
	double* calcCurveAt(double u) {
		double c0 = (1-u)*(1-u)*(1-u)*p[0][0]+ 3*(1-u)*(1-u)*u*p[1][0] + 3*(1-u)*u*u*p[2][0] + u*u*u*p[3][0];
		double c1 = (1-u)*(1-u)*(1-u)*p[0][1]+ 3*(1-u)*(1-u)*u*p[1][1] + 3*(1-u)*u*u*p[2][1] + u*u*u*p[3][1];

		double* res = (double*) malloc(sizeof(double)*2);
		res[0] = c0;
		res[1] = c1;
		return res;
	}

};

