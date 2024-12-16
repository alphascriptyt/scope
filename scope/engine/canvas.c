#include "canvas.h"

#include "common/status.h"

#include "utils/logger.h"

#include <stdlib.h>
#include <string.h>

Status canvas_init(Canvas* canvas, int width, int height)
{
	memset(canvas, 0, sizeof(Canvas));

	canvas->width = width;
	canvas->height = height;
	canvas->pixels = malloc((size_t)width * height * sizeof(unsigned int));

	if (!canvas->pixels)
	{
		log_error("Failed to allocate memory for canvas pixels.");
		return STATUS_ALLOC_FAILURE;
	}

	return STATUS_OK;
}

Status canvas_resize(Canvas* canvas, int width, int height)
{
	// Check the size has changed.
	if (canvas->width == width && canvas->height == height)
	{
		return STATUS_OK;
	}
	
	// Allocate memory for the new array.
	// TODO: Use my memory allocating helpers for this.
	unsigned int* new_pixels = realloc(canvas->pixels, (size_t)width * height * sizeof(unsigned int));

	// Check the allocation worked.
	if (!new_pixels)
	{
		log_error("Failed to reallocate memory for canvas pixels on resize.");
		return STATUS_ALLOC_FAILURE;
	}

	// Update the canvas.
	canvas->pixels = new_pixels;
	canvas->width = width;
	canvas->height = height;

	return STATUS_OK;
}

void canvas_fill(Canvas* canvas, const unsigned int colour)
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

void canvas_destroy(Canvas* canvas)
{
	free(canvas->pixels);
	canvas->pixels = 0;

	free(canvas);
	canvas = 0; // TODO: Do we want to do this?
}