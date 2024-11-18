#include "font.h"

#include "utils/logger.h"

#include <Windows.h>

Font load_font()
{
	// Initialise the character data.
	Font font = {
		.defined_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-=()[]{}<>/*:#%!?.,'\"@&$",
		.chars_per_row = 13,
		.char_width = 5,
		.char_height = 9
	};

	// TODO: Make this some sort of load_bitmap function.
	// Load the bitmap containing the defined characters.
	HBITMAP h_bitmap = LoadImageA(NULL, 
		"C:/Users/olive/source/repos/scope/scope/res/fonts/minogram_6x10_font.bmp",
		IMAGE_BITMAP,
		0, 0,
		LR_LOADFROMFILE
	);

	if (!h_bitmap)
	{
		log_error("Failed to load font bitmap.");
		return font;
	}

	// Get bitmap properties.
	BITMAP bitmap = { 0 };
	GetObject(h_bitmap, sizeof(BITMAP), &bitmap);

	// Create a compatible device context.
	HDC hdc = GetDC(NULL);
	HDC hdc_mem = CreateCompatibleDC(hdc);
	SelectObject(hdc_mem, h_bitmap);

	// Prepare bitmap info.
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bitmap.bmWidth;
	bmi.bmiHeader.biHeight = -bitmap.bmHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = bitmap.bmBitsPixel;
	bmi.bmiHeader.biCompression = BI_RGB;

	// Store the bitmap info.
	font.bitmap_width = bitmap.bmWidth;

	// Allocate memory for pixels.
	font.pixels = malloc((size_t)bitmap.bmWidthBytes * bitmap.bmHeight);

	// Get the pixels.
	GetDIBits(hdc_mem, h_bitmap, 0, bitmap.bmHeight, font.pixels, &bmi, DIB_RGB_COLORS);

	// Clear the bitmap.
	if (!DeleteObject(h_bitmap))
	{
		log_error("Failed to release font bitmap.");
	}

	return font;
}

int font_get_char_index(Font* font, char c)
{
	// TODO: Switch to a map of precalculated offsets possibly?
	int defined = 0;
	int charIndex;

	for (charIndex = 0; charIndex < strlen(font->defined_chars); ++charIndex)
	{
		if (font->defined_chars[charIndex] == c)
		{
			defined = 1;
			break;
		}
	}

	if (!defined)
	{
		char error[256];
		snprintf(error, sizeof(error), "Char not defined: %c", c);
		log_error(error);
		return -1;
	}

	// Calculate the position of the char on the bitmap.
	int cy = (charIndex / font->chars_per_row);
	int cx = charIndex - font->chars_per_row * cy;

	// Add one to the charHeight as there is 1 pixel between rows.
	int rowOffset = cy * (font->char_height + 1) * font->bitmap_width;

	// Add one to the charWidth as there is 1 pixel between characters.
	int colOffset = cx * (font->char_width + 1);

	return rowOffset + colOffset;
}
