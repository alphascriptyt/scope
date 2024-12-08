#ifndef COMMON_H
#define COMMON_H

// TODO: NOt sure what to call this.

#include <stdlib.h>

inline void swap(float** a, float** b)
{
	float* temp = *a;
	*a = *b;
	*b = temp;
}

inline float random_float()
{
	return (float)rand() / (float)RAND_MAX;
}

#endif