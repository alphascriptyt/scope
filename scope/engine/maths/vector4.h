#ifndef VECTOR4_H
#define VECTOR4_H

#include "utils/str_utils.h"

typedef struct
{
	float x, y, z, w;

} V4;

inline void v4_mul_eq_v4(V4* v0, V4 v1)
{
	v0->x *= v1.x;
	v0->y *= v1.y;
	v0->z *= v1.z;
	v0->w *= v1.w;
}

inline void v4_mul_eq_f(V4* v, float f)
{
	v->x *= f;
	v->y *= f;
	v->z *= f;
	v->w *= f;
}

inline V4 v4_mul_f(V4 v, float f)
{
	V4 out = {
		v.x * f,
		v.y * f,
		v.z * f,
		v.w * f
	};
	
	return out;
}

inline void v4_add_eq_f(V4* v, float f)
{
	v->x += f;
	v->y += f;
	v->z += f;
	v->w += f;
}

inline V4 v4_add_f(V4 v, float f)
{
	V4 out = {
		v.x + f,
		v.y + f,
		v.z + f,
		v.w + f
	};

	return out;
}

inline void v4_add_eq_v4(V4* v0, V4 v1)
{
	v0->x += v1.x;
	v0->y += v1.y;
	v0->z += v1.z;
	v0->w += v1.w;
}

inline void v4_sub_eq_f(V4* v, float f)
{
	v->x -= f;
	v->y -= f;
	v->z -= f;
	v->w -= f;
}

inline void v4_sub_eq_v4(V4* v0, V4 v1)
{
	v0->x -= v1.x;
	v0->y -= v1.y;
	v0->z -= v1.z;
	v0->w -= v1.w;
}

inline V4 v4_sub_v4(V4 v0, V4 v1)
{
	V4 out = {
		v0.x - v1.x,
		v0.y - v1.y,
		v0.z - v1.z,
		v0.w - v1.w
	};

	return out;
}

inline void v4_swap(V4* v0, V4* v1)
{
	V4 temp = *v0;
	*v0 = *v1;
	*v1 = temp;
}

inline char* v4_to_str(V4 v)
{
	return format_str("%f %f %f %f", v.x, v.y, v.z, v.w);
}

#endif