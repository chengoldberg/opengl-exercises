#include "Vec3f.h"

Vec3f::Vec3f(float x, float y, float z)
{
	this->m_data[0] = x;
	this->m_data[1] = y;
	this->m_data[2] = z;
	this->m_data[3] = 1;
}

Vec3f *Vec3f::Subtract(Vec3f *b)
{
	Vec3f *result = new Vec3f(); 
	for(int i=0;i<3;++i)
		result->set(i, this->m_data[i] - b->get(i));
	return result;
}

float Vec3f::getNorm()
{
	float result = 0;
	for(int i=0;i<3;++i)
		result += pow(this->m_data[i],2);
	result = powf(result, 1.0/2.0);
	return result;
}

void Vec3f::normalize()
{
	float norm = getNorm();
	for(int i=0;i<3;++i){
		this->m_data[i] /= norm;
	}
}

float Vec3f::length(Vec3f *vec, bool isSqrt)
{
	float result = 0;
	for(int i=0;i<3;++i)
		result += pow(vec->get(i) - this->m_data[i],2);

	if(isSqrt)
		result = sqrt(result);
	
	return result;
}
