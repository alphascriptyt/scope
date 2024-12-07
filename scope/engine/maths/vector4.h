#ifndef VECTOR4_H
#define VECTOR4_H

#include "utils/str_utils.h"

// TODO: After testing float array vs struct components, performance is exactly the same.
//		 The question is whether returning a struct with components would be slower than
//		 just updating the array. I actually don't mind using these as an array though.
//		 But I assume that returning a V4 is never going to have a performance issue.
//		 Could simplify some parts of code so maybe worth it.
typedef float V4[4];

inline void v4_mul_v4(V4 v0, const V4 v1)
{
	v0[0] *= v1[0];
	v0[1] *= v1[1];
	v0[2] *= v1[2];
	v0[3] *= v1[3];
}

inline void v4_mul_f(V4 v, const float f)
{
	v[0] *= f;
	v[1] *= f;
	v[2] *= f;
	v[3] *= f;
}

inline void v4_mul_f_out(const V4 v, const float f, V4 out)
{
	out[0] = v[0] * f;
	out[1] = v[1] * f;
	out[2] = v[2] * f;
	out[3] = v[3] * f;
}

inline void v4_add_f(V4 v, const float f)
{
	v[0] += f;
	v[1] += f;
	v[2] += f;
	v[3] += f;
}

inline void v4_add_f_out(const V4 v, const float f, V4 out)
{
	out[0] = v[0] + f;
	out[1] = v[1] + f;
	out[2] = v[2] + f;
	out[3] = v[3] + f;
}

inline void v4_add_v4(V4 v0, const V4 v1)
{
	v0[0] += v1[0];
	v0[1] += v1[1];
	v0[2] += v1[2];
	v0[3] += v1[3];
}

inline void v4_sub_f(V4 v, const float f)
{
	v[0] -= f;
	v[1] -= f;
	v[2] -= f;
	v[3] -= f;
}

inline void v4_sub_v4(V4 v0, const V4 v1)
{
	v0[0] -= v1[0];
	v0[1] -= v1[1];
	v0[2] -= v1[2];
	v0[3] -= v1[3];
}

inline void v4_sub_v4_out(const V4 v0, const V4 v1, V4 out)
{
	out[0] = v0[0] - v1[0];
	out[1] = v0[1] - v1[1];
	out[2] = v0[2] - v1[2];
	out[3] = v0[3] - v1[3];
}

inline void v4_copy(const V4 v, V4 out)
{
	out[0] = v[0];
	out[1] = v[1];
	out[2] = v[2];
	out[3] = v[3];
}

inline char* v4_to_str(V4 v)
{
	return format_str("%f %f %f %f", v[0], v[1], v[2], v[3]);
}

#endif