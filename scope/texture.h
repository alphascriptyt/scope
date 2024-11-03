#ifndef TEXTURE_H
#define TEXTURE_H

// TODO: Could this be a canvas?
typedef struct
{
	unsigned int* pixels;
	int width;
	int height;

} Texture;

Texture* load_texture_from_bmp(const char* file);


#endif