#ifndef VECTOR3_H
#define VECTOR3_H

#include "utils/str_utils.h"

#include <math.h>

// TODO: LOok into chaining these functions, i think if i return a pointer i can.
// TODO: Also, potentially, we could swap [] for the way we fill for better performance.
//		 but would have to test the difference it would make not sure.

typedef float V3[3];

inline void cross(const V3 v0, const V3 v1, V3 out)
{
	out[0] = v0[1] * v1[2] - v0[2] * v1[1];
	out[1] = v0[2] * v1[0] - v0[0] * v1[2];
	out[2] = v0[0] * v1[1] - v0[1] * v1[0];
}

inline float size(const V3 v)
{
	return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

inline float size_squared(const V3 v)
{
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

inline void normalise(V3 v)
{
	const float inv_size = 1.f / size(v);
	v[0] *= inv_size;
	v[1] *= inv_size;
	v[2] *= inv_size;
}

inline float dot(const V3 v0, const V3 v1)
{
	return v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2];
}

inline void v3_mul_v3(V3 v0, const V3 v1)
{
	v0[0] *= v1[0];
	v0[1] *= v1[1];
	v0[2] *= v1[2];
}

inline void v3_mul_f(V3 v, const float f)
{
	v[0] *= f;
	v[1] *= f;
	v[2] *= f;
}

inline void v3_mul_f_out(const V3 v, const float f, V3 out)
{
	out[0] = v[0] * f;
	out[1] = v[1] * f;
	out[2] = v[2] * f;
}

inline void v3_add_f(V3 v, const float f)
{
	v[0] += f;
	v[1] += f;
	v[2] += f;
}

inline void v3_add_v3(V3 v0, const V3 v1)
{
	v0[0] += v1[0];
	v0[1] += v1[1];
	v0[2] += v1[2];
}

inline void v3_add_v3_out(const V3 v0, const V3 v1, V3 out)
{
	out[0] = v0[0] + v1[0];
	out[1] = v0[1] + v1[1];
	out[2] = v0[2] + v1[2];
}

inline void v3_sub_f(V3 v, const float f)
{
	v[0] -= f;
	v[1] -= f;
	v[2] -= f;
}

inline void v3_sub_v3(V3 v0, const V3 v1)
{
	v0[0] -= v1[0];
	v0[1] -= v1[1];
	v0[2] -= v1[2];
}

inline void v3_sub_v3_out(const V3 v0, const V3 v1, V3 out)
{
	out[0] = v0[0] - v1[0];
	out[1] = v0[1] - v1[1];
	out[2] = v0[2] - v1[2];
}

inline char* v3_to_str(const V3 v)
{
	return format_str("%f %f %f", v[0], v[1], v[2]);
}

inline int is_front_face(const V3 v0, const V3 v1, const V3 v2)
{
	V3 v1v0, v2v0;
	v3_sub_v3_out(v1, v0, v1v0);
	v3_sub_v3_out(v2, v0, v2v0);
	
	V3 dir;
	cross(v1v0, v2v0, dir);

	// TODO: I actually think this is reversed now due to new coordinate system, not sure, must test.
	// Or its the cross producxt that is reversed. - think i've already done this.....
	return dot(v0, dir) <= 0;
}

#endif