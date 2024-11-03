#ifndef CANVAS_H
#define CANVAS_H

#include <stdlib.h>

typedef struct
{
	int width, height;
	unsigned int* pixels;

} Canvas;


Canvas* create_canvas(int width, int height);
void destroy_canvas(Canvas* canvas);
int resize_canvas(Canvas* canvas, int width, int height);
void fill_canvas(Canvas* canvas, const unsigned int colour);

#endif