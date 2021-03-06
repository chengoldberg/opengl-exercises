#pragma once

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>

using namespace std;

#define PI 3.1416

/**
 * Algebric vector
 *
 */
class Vec {
public:
	Vec() {
		m_data[0] = 0;
		m_data[1] = 0;
		m_data[2] = 0;
	}

	Vec(float x, float y, float z);

	Vec(FILE *fp);

	Vec(Vec &vec){
		m_data[0] = vec.get(0);
		m_data[1] = vec.get(1);
		m_data[2] = vec.get(2);
	}

	bool compare(Vec *vec);

	void set(Vec *vec){m_data[0]=vec->get(0); m_data[1]=vec->get(1); m_data[2]=vec->get(2);}

	Vec *Subtract(Vec *b);

	void static subtract(Vec *a, Vec *b, Vec *c){c->set(a->get(0)-b->get(0),a->get(1)-b->get(1),a->get(2)-b->get(2));}

	float dotProduct(Vec *vec){
		return 
			m_data[0] * vec->get(0) +
			m_data[1] * vec->get(1) +
			m_data[2] * vec->get(2);
	}

	float getNorm();

	void normalize();

	void add(float x, float y, float z){m_data[0]+=x;m_data[1]+=y;m_data[2]+=z;}

	void multiply(float s){m_data[0]*=s;m_data[1]*=s;m_data[2]*=s;}

	float length(Vec *vec, bool isSqrt = true);

	void mac(Vec &vec, float scalar){
		m_data[0] += scalar*vec.get(0);
		m_data[1] += scalar*vec.get(1);
		m_data[2] += scalar*vec.get(2);
	}

	float get(int i){return m_data[i];}
	float* get(){return m_data;}
	void set(int i, float value){m_data[i] = value;}
	void set(float x, float y, float z){m_data[0]=x;m_data[1]=y;m_data[2]=z;}	

	void print(){printf("[%f, %f, %f]\n", m_data[0],m_data[1],m_data[2]);}

	enum Dimensions {DIM_X,DIM_Y,DIM_Z};

	void rotate(float deg, Vec *axis){
		float rad = (float) deg2rad(deg);
		float kx = axis->get(0);
		float ky = axis->get(1);
		float kz = axis->get(2);
		
		float c = cos(rad);
		float t = 1-c;
		float s = sin(rad);
		float x,y,z;
		
		x = (kx*kx*t+c)		* m_data[0]	+ (kx*ky*t-kz*s) * m_data[1] + (kx*kz*t+ky*s) * m_data[2];
		y =	(ky*kx*t+kz*s)	* m_data[0]	+ (ky*ky*t+c)	 * m_data[1] + (ky*kz*t-kx*s) * m_data[2];
		z = (kz*kx*t-ky*s)	* m_data[0] + (kz*ky*t+kx*s) * m_data[1] + (kz*kz*t+c)	  * m_data[2];

		set(x,y,z);
	}

	void rotate(float deg, Dimensions dim){
		float rad = (float) deg2rad(deg);
		float cosAlpha = cos(rad);
		float sinAlpha = sin(rad);
		float temp;

		switch(dim){
			case DIM_X:
				temp = cosAlpha*m_data[1] - sinAlpha*m_data[2];				
				m_data[2] = sinAlpha*m_data[1] + cosAlpha*m_data[2];
				m_data[1] = temp;
				break;
			case DIM_Z:
				temp = cosAlpha*m_data[0] - sinAlpha*m_data[1];				
				m_data[1] = sinAlpha*m_data[0] + cosAlpha*m_data[1];
				m_data[0] = temp;
				break;
			case DIM_Y:
				temp = cosAlpha*m_data[0] + sinAlpha*m_data[2];
				m_data[2] = -sinAlpha*m_data[0] + cosAlpha*m_data[2];
				m_data[0] = temp;
				break;
		}
	}

	void progressiveRotation(float xrotation,float yrotation){
		set(0,0,-1);
		rotate(xrotation, DIM_X);
		rotate(yrotation, DIM_Y);
	}

	float calcDistance(Vec *vec, bool isSqrt = true){
		float x = m_data[0] - vec->get(0);
		float y = m_data[1] - vec->get(1);
		float z = m_data[2] - vec->get(2);

		float result = x*x + y*y + z*z;

		if(isSqrt)
			return sqrtf(result);
		else
			return result;
	}

	static void crossProduct(Vec *u, Vec *v, Vec *result){
		result->set(
				u->get(1)*v->get(2)-u->get(2)*v->get(1),
				u->get(0)*v->get(2)-u->get(2)*v->get(0),
				u->get(0)*v->get(1)-u->get(1)*v->get(0));
	}

	void averageSum(Vec *vec, int currentCount){
		if(currentCount == 0){
			set(vec);
		} else {
			mac(*vec, 1.0f/(float)currentCount);	
			multiply((float)(currentCount)/(float)(currentCount + 1));
		}		
	}

	static void calcLookAtMatrix(Vec *direction, Vec *up, float *matrix){
	
		Vec s,u;
		Vec::crossProduct(direction, up, &s);
		s.normalize();
		Vec::crossProduct(&s,direction,&u);		
		u.normalize();	//TODO: Consider removing

		matrix[0] = s.get(0);
		matrix[1] = s.get(1);
		matrix[2] = s.get(2);
		matrix[3] = 0;

		matrix[4] = u.get(0);
		matrix[5] = -u.get(1); //TODO: This is a strange thing!
		matrix[6] = u.get(2);
		matrix[7] = 0;

		matrix[8] = -direction->get(0);
		matrix[9] = -direction->get(1);
		matrix[10] = -direction->get(2);
		matrix[11] = 0;

		matrix[12] = 0;
		matrix[13] = 0;
		matrix[14] = 0;
		matrix[15] = 1;
	}

	static void calcIdentityMatrix(float *matrix){
	
		matrix[0] = 1;
		matrix[1] = 0;
		matrix[2] = 0;
		matrix[3] = 0;

		matrix[4] = 0;
		matrix[5] = 1;
		matrix[6] = 0;
		matrix[7] = 0;

		matrix[8] = 0;
		matrix[9] = 0;
		matrix[10] = 1;
		matrix[11] = 0;

		matrix[12] = 0;
		matrix[13] = 0;
		matrix[14] = 0;
		matrix[15] = 1;
	}


	inline double static deg2rad(double deg){
		return deg*PI/180.0;
	}
	inline double static rad2deg(double rad){
		return rad*180.0/PI;
	}

	private:
	float m_data[3];
};

