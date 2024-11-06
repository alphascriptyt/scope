#ifndef MATRIX4_H
#define MATRIX4_H

#include "vector4.h"
#include "vector3.h"

#include "utils/str_utils.h"

typedef float M4[16];

// TODO: Rename _out?
void m4_mul_m4(const M4 m0, const M4 m1, M4 out);

void m4_mul_v4(const M4 m, const V4 v, V4 out);

void identity(M4 out);

// TODO: MODEL MATRIX STUFF
void make_translation_m4(const V3 position, M4 out);

void make_rotation_m4(const float pitch, const float yaw, const float roll, M4 out);

void look_at(const V3 position, const V3 direction, M4 out);

void make_model_m4(const V3 position, const V3 orientation, const V3 scale, M4 out);

void transpose_m4(const M4 in, M4 out);

char* m4_to_str(const M4 m);

#endif
