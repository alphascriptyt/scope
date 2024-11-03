#include "canvas.h"

#include "utils/logger.h"

#include <stdlib.h>
#include <stdio.h>

Canvas* create_canvas(int width, int height)
{
	Canvas* canvas = malloc(sizeof(Canvas));

	if (!canvas)
	{
		log_error("Failed to allocate memory for canvas.");
		return 0;
	}

	canvas->width = width;
	canvas->height = height;
	canvas->pixels = malloc((size_t)width * height * sizeof(unsigned int));

	if (!canvas->pixels)
	{
		log_error("Failed to allocate memory for canvas pixels.");
		return 0;
	}

	return canvas;
}

void destroy_canvas(Canvas* canvas)
{
	free(canvas->pixels);
	canvas->pixels = 0;

	free(canvas);
	canvas = 0;
}

int resize_canvas(Canvas* canvas, int width, int height)
{
	// Check the size has changed.
	if (canvas->width == width && canvas->height == height)
	{
		return 0;
	}
	
	// Allocate memory for the new array.
	unsigned int* new_pixels = realloc(canvas->pixels, (size_t)width * height * sizeof(unsigned int));

	// Check the allocation worked.
	if (!new_pixels)
	{
		// We should be able to re-use the old pixels.
		log_warn("Failed to reallocate memory for canvas pixels on resize.");
		return 0;
	}

	// Update the canvas.
	canvas->pixels = new_pixels;
	canvas->width = width;
	canvas->height = height;

	return 1;
}

void fill_canvas(Canvas* canvas, const unsigned int colour)
{
	// TODO: Look for some sort of blit or fill function 
	const int length = canvas->width * canvas->height;	
	unsigned int* ptr = canvas->pixels;
	
	unsigned int i = length;
	
	while (i)
	{
		*ptr = colour;
		--i;
		++ptr;
	}
}
