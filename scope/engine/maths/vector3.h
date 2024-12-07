#ifndef VECTOR3_H
#define VECTOR3_H

#include "vector4.h"

#include "utils/str_utils.h"

#include <math.h>

// TODO: Should we just use x,y,z?

// TODO: LOok into chaining these functions, i think if i return a pointer i can.
// TODO: Also, potentially, we could swap [] for the way we fill for better performance.
//		 but would have to test the difference it would make not sure.

// TODO: Is the order of in/out okay?


// TODO: I think using x,y,z for all this is going to be a good refactor. I can convert it all and benchmark
//		 with my 1000 monkeys. I don't think there will be any performance difference. as there should be no padding.
//		 We will need to pass everything as pointers but should be fine.
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

inline void v3_mul_v3_out(const V3 v0, const V3 v1, V3 out)
{
	out[0] = v0[0] * v1[0];
	out[1] = v0[1] * v1[1];
	out[2] = v0[2] * v1[2];
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

inline void v3_init(V3 v, float x, float y, float z)
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

// TODO: Better name for this. out being the second param here feels wrong.
//		 Should make this like v3_sub_v3.
inline void v3_copy(const V3 in, V3 out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

inline void v3_to_v4_point(const V3 in, V4 out)
{
	// Copy the v3.
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];

	// Set w = 1 so an m4's translation will be applied.
	out[3] = 1; 
}

inline char* v3_to_str(const V3 v)
{
	return format_str("%f %f %f", v[0], v[1], v[2]);
}

// TODO: This shouldn't be in V3.
inline int is_front_face(const V3 v0, const V3 v1, const V3 v2)
{
	V3 v1v0, v2v0;
	v3_sub_v3_out(v1, v0, v1v0);
	v3_sub_v3_out(v2, v0, v2v0);
	
	V3 dir;
	cross(v1v0, v2v0, dir);
	
	// TODO: Check this is the correct way.
	return dot(v0, dir) <= 0;
}

#endif