#ifndef CANVAS_H
#define CANVAS_H

#include "common/status.h"

typedef struct
{
	int width, height;
	unsigned int* pixels;

} Canvas;


Status canvas_init(Canvas* canvas, int width, int height);

Status canvas_resize(Canvas* canvas, int width, int height);

void canvas_fill(Canvas* canvas, const unsigned int colour);

void canvas_destroy(Canvas* canvas);

#endif