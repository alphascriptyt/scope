#ifndef CANVAS_H
#define CANVAS_H

#include "common/status.h"

typedef struct
{
	int width, height;
	unsigned int* pixels;

} Canvas;

Status canvas_init(Canvas* canvas, int width, int height);

Status canvas_init_from_bitmap(Canvas* canvas, const char* file);

Status canvas_resize(Canvas* canvas, int width, int height);

void canvas_fill(Canvas* canvas, const unsigned int colour);

void canvas_draw(const Canvas* source, Canvas* target, int x_offset, int y_offset);

void canvas_destroy(Canvas* canvas);

#endif