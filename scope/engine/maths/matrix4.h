#ifndef MATRIX4_H
#define MATRIX4_H

#include "vector4.h"
#include "vector3.h"

#include "utils/str_utils.h"

/*
- Memory Layout:
We're using a column major memory layout for better cache usage
when multiplying m4 and v4 as columns are stored contingous memory.
After testing, there actually seemed to be no real noticeable 
difference in performance though.

- Multiplication
We post-multiply (matrix first) a m4 and v4, this means that the v4
is a column vector (4row x 1col) matrix. This is because the cols
in the first matrix much match the rows in the second.


// TODO: I want to stop using column major storage as both opengl and directx do
		 not, therefore it's just confusing and weird. Right?.... 

		 Not sure it even matters. Can be a refactor in future if necessary.

*/
typedef float M4[16];

// TODO: Rename _out?
void m4_mul_m4(const M4 m0, const M4 m1, M4 out);

void m4_mul_v4(const M4 m, const V4 v, V4 out);

void m4_identity(M4 out);

void m4_translation(const V3 position, M4 out);

void m4_rotation(const float pitch, const float yaw, const float roll, M4 out);

void look_at(const V3 position, const V3 direction, M4 out);

void m4_model_matrix(const V3 position, const V3 eulers, const V3 scale, M4 out);

void m4_normal_matrix(const V3 eulers, const V3 scale, M4 out);

void m4_transposed(const M4 in, M4 out);

void m4_copy_m3(const M4 in, M4 out);

char* m4_to_str(const M4 m);

#endif
