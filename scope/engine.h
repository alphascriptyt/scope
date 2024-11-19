#ifndef ENGINE_H
#define ENGINE_H

#include "render_target.h"
#include "camera.h"
#include "models.h"
#include "lights.h"
#include "render_settings.h"

#include "ui/text.h"

#include <Windows.h>


#define SCOPE_WINDOW_CLASS "scope_window_class"
#define TEXT_COUNT 3

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef struct
{
	int game_running;
	//Canvas* canvas;
	RenderTarget* render_target;
	
	// Application Window
    HWND hwnd;

	HDC hdc;

	BITMAPINFO canvas_bitmap;

	// UI
	Font font;
	Text ui_text[TEXT_COUNT];

	// Entities

	// State
	int previous_mouse_x;
	int previous_mouse_y;
	int lock_mouse;
	
	// Settings
	float upscaling_factor;
	int window_width;
	int window_height;
	RenderSettings* render_settings;
	
	// TODO: Some sort of scene thing rather than just models/lights
	Models* models;
	PointLights* point_lights;

} Engine;

Engine* init_engine();

void start_engine(Engine* engine);

void create_window(Engine* engine, const char* title);

void on_resize(Engine* engine);

int process_window_messages(Engine* engine);

void draw_ui(Engine* engine);

void handle_input(Engine* engine, Camera* camera, float dt);

void display_window(Engine* engine);

void cleanup_engine(Engine* engine);

#endif