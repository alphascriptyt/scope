#ifndef FONT_H
#define FONT_H

typedef struct
{
	int char_width;
	int char_height;
	int chars_per_row;

	int bitmap_width;

	const char* defined_chars;
	unsigned int* pixels;

} Font;

Font load_font();
int font_get_char_index(Font* font, char c);

#endif