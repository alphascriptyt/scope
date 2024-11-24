#ifndef ENGINE_H
#define ENGINE_H

#include "window.h"
#include "ui/ui.h"
#include "renderer/renderer.h"

#include "renderer/render.h"

#include "common/status.h"

#include "models.h"
#include "lights.h"

typedef struct
{
	// Engine components.
	Window window;
	UI ui;
	Renderer renderer;

	// TEMP: TODO: Scenes.
	Models models;
	PointLights lights;

	// Engine settings
	int running;

	// TODO: Move these somewhere?
	int lock_mouse;
	float upscaling_factor;

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