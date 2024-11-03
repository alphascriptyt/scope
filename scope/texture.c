#include "texture.h"

#include "utils/logger.h"

#include <Windows.h>

#include <stdlib.h>

Texture* load_texture_from_bmp(const char* file)
{
	Texture* t = malloc(sizeof(Texture));
    if (!t)
    {
        log_error("Failed to malloc for texture.");
        return 0;
    }

    // Load the bitmap.
    HBITMAP h_bitmap = (HBITMAP)LoadImageA(NULL, file, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    if (!h_bitmap) {
        log_error("Failed to load texture bitmap.");
        printf("%d\n", GetLastError());
        return 0;
    }

    // Get the actual bitmap.
    BITMAP bitmap = { 0 };
    if (GetObject(h_bitmap, sizeof(BITMAP), &bitmap) == 0) {
        log_error("Failed to retrieve texture bitmap info.");
        return 0;
    }

    // Get the bitmap info.
    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bitmap.bmWidth;
    bi.biHeight = -bitmap.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = bitmap.bmBitsPixel;
    bi.biCompression = BI_RGB;

    if (bitmap.bmBitsPixel != 32)
    {
        log_error("Bits per pixel != 32.\n");
        return 0;
    }

    t->width = bi.biWidth;
    t->height = abs(bi.biHeight);

    unsigned int* new_pixels = malloc(sizeof(unsigned int) * t->width * t->height);
    if (!new_pixels)
    {
        log_error("Failed to allocate new pixels for texture.");
        return 0;
    }

    t->pixels = new_pixels;

    // Get the bitmap bits.
    HDC hdc = GetDC(NULL);
    if (!GetDIBits(hdc, h_bitmap, 0, bitmap.bmHeight, t->pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS)) 
    {
        log_error("Failed to GetDIBits()");
        return 0;
    }

    // Clean up
    ReleaseDC(NULL, hdc);
    DeleteObject(h_bitmap);

    return t;
}