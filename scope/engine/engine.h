#ifndef ENGINE_H
#define ENGINE_H

#include "window.h"
#include "ui/ui.h"
#include "renderer/renderer.h"

#include "common/status.h"

typedef struct
{
	// Engine components.
	Window window;
	UI ui;
	Renderer renderer;

	// Engine settings
	int running;

	// TODO: Move these somewhere?
	float upscaling_factor;

	int lock_mouse;
	int previous_mouse_x, previous_mouse_y;

} Engine;

// Main API
Status engine_init(Engine* engine, int window_width, int window_height);

void engine_run(Engine* engine);

void engine_destroy(Engine* engine);

// TODO: Some sort of input handler? Fine here for now.
void engine_handle_input(Engine* engine, float dt);

// Window events.
void engine_on_resize(void* ctx);

void engine_on_keyup(void* ctx, WPARAM wParam);

#endif