#include "Vec.h"

Vec::Vec(float x, float y, float z){
	this->m_data[0] = x;
	this->m_data[1] = y;
	this->m_data[2] = z;
}

Vec::Vec(FILE *fp){
	fscanf(fp,"%f %f %f", &(m_data[0]),&(m_data[1]), &(m_data[2]));
}

Vec *Vec::Subtract(Vec *b){

	Vec *result = new Vec(); 
	for(int i=0;i<3;++i)
		result->set(i, this->m_data[i] - b->get(i));
	return result;
}

float Vec::getNorm(){

	float result = 0;
	for(int i=0;i<3;++i)
		result += pow(this->m_data[i],2);
	result = powf(result, 1.0/2.0);
	return result;
}

void Vec::normalize(){

	float norm = getNorm();

	for(int i=0;i<3;++i){
		this->m_data[i] /= norm;
	}
}

float Vec::length(Vec *vec, bool isSqrt){

	float result = 0;
	for(int i=0;i<3;++i)
		result += pow(vec->get(i) - this->m_data[i],2);

	if(isSqrt)
		result = sqrt(result);
	
	return result;
}
