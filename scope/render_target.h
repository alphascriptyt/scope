#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include "canvas.h"

#include "utils/logger.h"

// Contains the different buffers needed for rendering.
typedef struct
{
	Canvas* canvas;
	float* depth_buffer;

} RenderTarget;


// TODO: .c file?
inline RenderTarget* create_render_target(const int width, const int height)
{
    // TODO: Why is this using malloc?
    RenderTarget* rt = malloc(sizeof(RenderTarget));

    if (!rt)
    {
        log_error("Failed to allocate memory for render target.");
        return 0;
    }

    rt->canvas = create_canvas(width, height);

    if (!rt->canvas)
    {
        log_error("Failed to create render target canvas.\n");
    }

    rt->depth_buffer = malloc((size_t)width * height * sizeof(float));

    if (!rt->depth_buffer)
    {
        log_error("Failed to allocate memory for depth buffer.");
    }

    return rt;
}

inline void resize_render_target(RenderTarget* rt, int width, int height)
{
    if (resize_canvas(rt->canvas, width, height))
    {
        // Allocate memory for the new array.
        float* new_db = realloc(rt->depth_buffer, (size_t)width * height * sizeof(float));

        // Check the allocation worked.
        if (!new_db)
        {
            // TODO: Kinda need to reset the canvas if this happens.
            // We should be able to re-use the old depth buffer
            log_warn("Failed to reallocate memory for rt depth buffer on resize.");
            return;
        }

        // Update the depth buffer.
        rt->depth_buffer = new_db;
    }
}

inline void destroy_render_target(RenderTarget* rt)
{
    free(rt->canvas->pixels);
    rt->canvas->pixels = 0;

    free(rt->depth_buffer);
    rt->depth_buffer = 0;

    free(rt);
    rt = 0;
}

inline void clear_render_target(RenderTarget* rt, const unsigned int bg_colour)
{
    // TODO: Look for some sort of blit or fill function 
    const int length = rt->canvas->width * rt->canvas->height;
    const float max_depth = 1.f; // The projection should map z to a max of 1.

    unsigned int* canvas_ptr = rt->canvas->pixels;
    float* depth_buffer_ptr = rt->depth_buffer;

    unsigned int i = length;

    while (i)
    {
        *canvas_ptr = bg_colour;
        *depth_buffer_ptr = max_depth;
        --i;
        ++canvas_ptr;
        ++depth_buffer_ptr;
    }
}

#endif