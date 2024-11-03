#ifndef TEXT_H
#define TEXT_H

#include "font.h"
#include "../canvas.h"

typedef struct
{
	char* text;
	int x;
	int y;
	int colour;
	int scale;

} Text;

Text create_text(char* text, int x, int y, int colour, int scale);
void draw_text(Canvas* canvas, Text* text, Font* font, float upscaling_factor);

#endif