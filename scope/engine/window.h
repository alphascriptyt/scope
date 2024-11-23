#ifndef WINDOW_H
#define WINDOW_H

#include "canvas.h"

#include "common/status.h"

#include <Windows.h>

#define SCOPE_WINDOW_CLASS "scope_window_class"
#define SCOPE_WINDOW_TITLE "Scope"

// TODO: Top of file comments.


// Window message handling.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int window_process_messages();

typedef struct
{
	Canvas* canvas;
	
	HWND hwnd;
	HDC hdc;
	BITMAPINFO bitmap;

	int width;
	int height;
	int upscaling_factor;

	// Event callbacks
	void* ctx; // Set to Engine* so we can use it in the callbacks.
	void (*on_resize)(void*);
	void (*on_keyup)(void*, WPARAM);
	void (*on_quit)(void*);
	
} Window;

Status window_init(Window* window, const Canvas* canvas, void* ctx);

void window_display(Window* window);

void window_destroy(Window* window);


#endif