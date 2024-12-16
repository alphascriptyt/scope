#ifndef TEXTURE_H
#define TEXTURE_H

#include "common/status.h"

#include "utils/memory_utils.h"
#include "utils/logger.h"

#include <Windows.h>

#include <stdlib.h>
#include <string.h>

/*
TODO: Where are textures going to go? 

Ideally I think we just want to give a mi a texture index, this means we need some global list
of loaded textures right? 

So then where do these textures go? We can keep this as an array of structs.

Just gotta think does the engine manage some resources? how does the renderer access the textures.

*/

// Defines a texture that a model can use.
typedef struct
{
	int width;
	int height;
	unsigned int* pixels;

} Texture;

inline Status texture_load(Texture* texture, const char* file)
{
    // TODO: Do we need to use Windows.h here? If we're only loading
    //       bitmaps, the image loading code could be quite simple.

	// TODO: Load the texture from a file.
	// TODO: This could be called bitmap. I'm assuming we don't need an alpha value.

    // TODO:

    // Initialise the texture.
    memset(texture, 0, sizeof(Texture));

    // Load Resources, temporary. TODO: a ResourceManager?
    HBITMAP h_bitmap = (HBITMAP)LoadImageA(
        NULL,
        file,
        IMAGE_BITMAP,
        0, 0,
        LR_LOADFROMFILE
    );

    if (!h_bitmap)
    {
        log_error("Failed to load texture bitmap.");
        return STATUS_FAILURE;
    }

    // Get bitmap properties.
    BITMAP bitmap = { 0 };
    GetObject(h_bitmap, sizeof(BITMAP), &bitmap);

    texture->width = bitmap.bmWidth;
    texture->height = bitmap.bmHeight;

    // Create a compatible DC.
    HDC hdc = GetDC(NULL);
    HDC mem_hdc = CreateCompatibleDC(hdc);
    
    HGDIOBJ prev = SelectObject(mem_hdc, h_bitmap);

    // Prepare bitmap info.
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bitmap.bmWidth;
    bmi.bmiHeader.biHeight = -bitmap.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = bitmap.bmBitsPixel;
    bmi.bmiHeader.biCompression = BI_RGB; // Uncompressed RGB.

    // Allocate memory for pixels
    Status status = resize_int_buffer(&texture->pixels, bitmap.bmWidthBytes * bitmap.bmHeight);
    if (STATUS_OK != status)
    {
        return status;
    }

    // Get the pixels buffer.
    GetDIBits(mem_hdc, h_bitmap, 0, bitmap.bmHeight, texture->pixels, &bmi, DIB_RGB_COLORS);

    // Cleanup.

    if (!DeleteObject(h_bitmap))
    {

    }

    SelectObject(mem_hdc, prev);
    if (!DeleteDC(mem_hdc))
    {

    }
    
    if (!ReleaseDC(NULL, hdc))
    {

    }

    return STATUS_OK;
}

inline void texture_destroy(Texture* texture)
{
	if (texture->pixels)
		free(texture->pixels);

	texture = 0;
}

#endif