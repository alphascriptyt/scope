#ifndef COLOUR_H
#define COLOUR_H

// TODO: Rename colour helpers or something?

inline int float_rgb_to_int(float r, float g, float b)
{
	return ((int)(r * 255) << 16
		| (int)(g * 255) << 8
		| (int)(b * 255));
}

inline int int_rgb_to_int(int r, int g, int b)
{
	return (r << 16 | g << 8 | b);
}

inline void unpack_int_rgb_to_floats(int colour, float* r, float* g, float* b)
{
	float n = 1.f / 255.f;

	*r = ((colour >> 16) & 0xFF) * n;
	*g = ((colour >> 8) & 0xFF) * n;
	*b = (colour & 0xFF) * n;
}

#endif