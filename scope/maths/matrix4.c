#include "matrix4.h"

void m4_mul_m4(const M4 m0, const M4 m1, M4 out)
{
	for (int c = 0; c < 4; ++c)
	{
		for (int r = 0; r < 4; ++r)
		{
			int rowOffset = r * 4;

			out[rowOffset + c] = m1[rowOffset] * m0[c] +
				m1[rowOffset + 1] * m0[c + 4] +
				m1[rowOffset + 2] * m0[c + 8] +
				m1[rowOffset + 3] * m0[c + 12];
		}
	}
}

void m4_mul_v4(const M4 m, const V4 v, V4 out)
{
	out[0] = m[0] * v[0] + m[4] * v[1] + m[8]  * v[2] + m[12] * v[3];
	out[1] = m[1] * v[0] + m[5] * v[1] + m[9]  * v[2] + m[13] * v[3];
	out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14] * v[3];
	out[3] = m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15] * v[3];
}

void identity(M4 out)
{
	out[0] = 1;
	out[1] = 0;
	out[2] = 0;
	out[3] = 0;
	out[4] = 0;
	out[5] = 1;
	out[6] = 0;
	out[7] = 0;
	out[8] = 0;
	out[9] = 0;
	out[10] = 1;
	out[11] = 0;
	out[12] = 0;
	out[13] = 0;
	out[14] = 0;
	out[15] = 1;
}

void make_translation_m4(const V3 position, M4 out)
{
	identity(out);
	out[12] = position[0];
	out[13] = position[1];
	out[14] = position[2];
}

void make_rotation_m4(const float pitch, const float yaw, const float roll, M4 out)
{
	
	float sinPitch = sinf(pitch);
	float sinYaw = sinf(yaw);
	float sinRoll = sinf(roll);

	float cosPitch = cosf(pitch);
	float cosYaw = cosf(yaw);
	float cosRoll = cosf(roll);

	M4 pitch_rot;
	identity(pitch_rot);
	/*
	pitch_rot[5] = cosPitch;
	pitch_rot[6] = sinPitch;
	pitch_rot[9] = -sinPitch;
	pitch_rot[10] = cosPitch;
	*/

	pitch_rot[5] = cosPitch;
	pitch_rot[9] = sinPitch;
	pitch_rot[6] = -sinPitch;
	pitch_rot[10] = cosPitch;

	M4 yaw_rot;
	identity(yaw_rot);
	/*
	yaw_rot[0] = cosYaw;
	yaw_rot[2] = sinYaw;
	yaw_rot[8] = -sinYaw;
	yaw_rot[10] = cosYaw;*/

	yaw_rot[0] = cosYaw;
	yaw_rot[8] = sinYaw;
	yaw_rot[2] = -sinYaw;
	yaw_rot[10] = cosYaw;

	M4 roll_rot;
	identity(roll_rot);
	/*
	roll_rot[0] = cosRoll;
	roll_rot[1] = -sinRoll;
	roll_rot[4] = sinRoll;
	roll_rot[5] = cosRoll;
	*/

	roll_rot[0] = cosRoll;
	roll_rot[4] = -sinRoll;
	roll_rot[1] = sinRoll;
	roll_rot[5] = cosRoll;


	// TODO: Multiply by roll. Will this give us the euler lock thing?
	m4_mul_m4(pitch_rot, yaw_rot, out);
}

void look_at(const V3 position, const V3 direction, M4 out)
{
	V3 world_up = { 0, 1, 0 };

	// Calculate the axis from the direction being the z axis.
	V3 x_axis;
	cross(world_up, direction, x_axis);
	normalise(x_axis);
	
	V3 y_axis;
	cross(direction, x_axis, y_axis);

	// Set the out matrix to the combined translation and rotation matrix.
	identity(out);

	out[0] = x_axis[0]; out[1] = y_axis[0]; out[2] = direction[0];
	out[4] = x_axis[1]; out[5] = y_axis[1]; out[6] = direction[1];
	out[8] = x_axis[2]; out[9] = y_axis[2]; out[10] = direction[2];

	out[12] = -dot(x_axis, position);
	out[13] = -dot(y_axis, position);
	out[14] = -dot(direction, position);
}

void make_model_m4(const V3 position, const V3 orientation, const V3 scale, M4 out)
{
	// TODO: Look into avoiding the matrix multiplications? Can I set translation without multiplying?
	//		 Pretty sure I can.
	M4 translation_m4;
	make_translation_m4(position, translation_m4);

	M4 rotation_m4;
	make_rotation_m4(orientation[0], orientation[1], orientation[2], rotation_m4);

	M4 scale_m4;
	identity(scale_m4);
	scale_m4[0] = scale[0];
	scale_m4[5] = scale[1];
	scale_m4[10] = scale[2];
	scale_m4[15] = 1;

	// We have to define an output matrix each time.
	// Although in my opinion this is fine it makes it more clear.
	M4 translation_rotation_m4;
	m4_mul_m4(translation_m4, rotation_m4, translation_rotation_m4);
	m4_mul_m4(translation_rotation_m4, scale_m4, out);
}

void transpose_m4(const M4 in, M4 out)
{
	// Flip the matrix along the diagonal. Essentially column major to row major.
	out[0] = in[0];
	out[1] = in[4];
	out[2] = in[8];
	out[3] = in[12];
	out[4] = in[1];
	out[5] = in[5];
	out[6] = in[9];
	out[7] = in[13];
	out[8] = in[2];
	out[9] = in[6];
	out[10] = in[10];
	out[11] = in[14];
	out[12] = in[3];
	out[13] = in[7];
	out[14] = in[11];
	out[15] = in[15];
}

void invert_m3(const M4 in, M4 out)
{
}

char* m4_to_str(const M4 m)
{
	return format_str("%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",
		m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
}