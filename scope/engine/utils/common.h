#ifndef COMMON_H
#define COMMON_H

// TODO: NOt sure what to call this.

inline void swap(float** a, float** b)
{
	float* temp = *a;
	*a = *b;
	*b = temp;
}

#endif