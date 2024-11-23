#ifndef UTILS_H
#define UTILS_H

#define _USE_MATH_DEFINES
#include <math.h>

#define PI (float)M_PI
#define PI_2 (float)M_PI_2

inline float radians(float degrees)
{
	return (float)(degrees * PI) / 180.f;
}

inline float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}


#endif