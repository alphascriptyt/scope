#include "engine.h"

#include "utils/logger.h"
#include "utils/timer.h"

#include "common/colour.h"

#include "globals.h"

#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>

Status engine_init(Engine* engine, int window_width, int window_height)
{
    log_info("Initialising the engine.");
    memset(engine, 0, sizeof(Engine));

    // Set some default settings.
    engine->current_scene_id = -1;
    engine->upscaling_factor = 1;
    engine->lock_mouse = 0;

    // Initialise the renderer.
    Status status = renderer_init(&engine->renderer, (int)(window_width / engine->upscaling_factor), (int)(window_height / engine->upscaling_factor));
    if (STATUS_OK != status)
    {
        log_error("Failed to renderer_init because of %s", status_to_str(status));
        return status;
    }

    // Initialise the window.
    status = window_init(&engine->window, &engine->renderer.target.canvas, (void*)engine, window_width, window_height);
    if (STATUS_OK != status)
    {
        log_error("Failed to window_init because of %s", status_to_str(status));
        return status;
    }

    // Set window callbacks.
    engine->window.on_resize = &engine_on_resize;
    engine->window.on_keyup = &engine_process_keyup;

    // Initialise the UI.
    status = ui_init(&engine->ui, &engine->renderer.target.canvas);
    if (STATUS_OK != status)
    {
        log_error("Failed to ui_init because of %s", status_to_str(status));
        return status;
    }

    // Initialise the resources.
    resources_init(&engine->resources);

    // Initialise a random seed
    srand((unsigned int)time(NULL));
    
    log_info("Fired engine_on_init event.");
    engine_on_init(engine);

    log_info("Engine successfully initialised.");
    return STATUS_OK;
}

void engine_run(Engine* engine)
{
    // TEMP
    LARGE_INTEGER frequency = { 0 };
    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER startTime = { 0 };
    LARGE_INTEGER endTime = { 0 };

    // TODO: Struct of perf data?
    int fps = 0;
    float dt = 0;
    int draw_ui_ms = 0;

    // Start the timers.
    QueryPerformanceCounter(&startTime);

    float dt_counter = 0;

    char fps_str[64] = "";
    char dir_str[64] = "";
    char pos_str[64] = "";

    
    char process_messages_str[64] = "";
    char handle_input_str[64] = "";
    char rt_clear_str[64] = "";
    char render_str[64] = "";
    char ui_draw_str[64] = "";
    char display_str[64] = "";
    char update_str[64] = "";

    int y = 10;
    int h = 30;

    // TODO: This can be an add text function. Was thinking about 'draw_text()' but then something like this would be annoying.
    engine->ui.text[engine->ui.text_count++] = text_create(fps_str, 10, engine->ui.text_count * h + 10, COLOUR_LIME, 3);
    engine->ui.text[engine->ui.text_count++] = text_create(dir_str, 10, engine->ui.text_count * h + 10, COLOUR_RED, 3);
    engine->ui.text[engine->ui.text_count++] = text_create(pos_str, 10, engine->ui.text_count * h + 10, COLOUR_RED, 3);
    engine->ui.text[engine->ui.text_count++] = text_create(process_messages_str, 10, engine->ui.text_count * h + 10, COLOUR_WHITE, 3);
    engine->ui.text[engine->ui.text_count++] = text_create(handle_input_str, 10, engine->ui.text_count * h + 10, COLOUR_WHITE, 3);
    engine->ui.text[engine->ui.text_count++] = text_create(rt_clear_str, 10, engine->ui.text_count * h + 10, COLOUR_WHITE, 3);
    engine->ui.text[engine->ui.text_count++] = text_create(render_str, 10, engine->ui.text_count * h + 10, COLOUR_WHITE, 3);
    engine->ui.text[engine->ui.text_count++] = text_create(ui_draw_str, 10, engine->ui.text_count * h + 10, COLOUR_WHITE, 3);
    engine->ui.text[engine->ui.text_count++] = text_create(display_str, 10, engine->ui.text_count * h + 10, COLOUR_WHITE, 3);
    engine->ui.text[engine->ui.text_count++] = text_create(update_str, 10, engine->ui.text_count * h + 10, COLOUR_WHITE, 3);

    engine->running = 1;
    while (engine->running)
    {
        Timer t = timer_start();

        // Process the application window messages.
        if (!window_process_messages())
        {
            break;
        }

        snprintf(process_messages_str, sizeof(process_messages_str), "ProcMsgs: %d", timer_get_elapsed(&t));

        // Handle any keyboard/mouse input.
        timer_restart(&t);
        engine_handle_input(engine, dt);
        snprintf(handle_input_str, sizeof(handle_input_str), "HandleInput: %d", timer_get_elapsed(&t));
        
        M4 view_matrix;
        calculate_view_matrix(&engine->renderer.camera, view_matrix);

        // Clear the canvas.
        timer_restart(&t);
        render_target_clear(&engine->renderer.target, 0x22222222);
        snprintf(rt_clear_str, sizeof(rt_clear_str), "RTClear: %d", timer_get_elapsed(&t));

        // Render scene.
        timer_restart(&t);
        if (engine->current_scene_id > -1 && engine->current_scene_id < engine->scenes_count)
        {
            render(&engine->renderer.target, &engine->renderer.settings,
                &engine->scenes[engine->current_scene_id], &engine->resources, view_matrix);
        }
        snprintf(render_str, sizeof(render_str), "Render: %d", timer_get_elapsed(&t));

        // Fire the engine update event.
        timer_restart(&t);
        engine_on_update(engine, dt);
        snprintf(update_str, sizeof(update_str), "UpdateEvent: %d", timer_get_elapsed(&t));

        // Draw ui elements.
        timer_restart(&t);
        ui_draw(&engine->ui, engine->upscaling_factor);
        snprintf(ui_draw_str, sizeof(render_str), "DrawUI: %d", draw_ui_ms);
        draw_ui_ms = timer_get_elapsed(&t); // Must be done a frame late.

        // Update the display.
        timer_restart(&t);
        window_display(&engine->window);
        snprintf(display_str, sizeof(display_str), "Display: %d", timer_get_elapsed(&t));

        // Calculate performance.
        QueryPerformanceCounter(&endTime);
        dt = (float)(endTime.QuadPart - startTime.QuadPart) / frequency.QuadPart;

        dt_counter += dt;

        startTime = endTime;

        fps = (int)(1.0f / dt);

        if (dt_counter > 2)
        {
            snprintf(fps_str, sizeof(fps_str), "FPS: %d", fps);
            dt_counter = 0;
        }

        snprintf(dir_str, sizeof(dir_str), "DIR: %.2f %.2f %.2f", engine->renderer.camera.direction[0], engine->renderer.camera.direction[1], engine->renderer.camera.direction[2]);
        snprintf(pos_str, sizeof(pos_str), "POS: %.2f %.2f %.2f", engine->renderer.camera.position[0], engine->renderer.camera.position[1], engine->renderer.camera.position[2]);
    }
}

void engine_destroy(Engine* engine)
{
    /*
    free_models(engine->models);
    destroy_render_target(engine->render_target);

    free(engine->models);
    free(engine->point_lights);
    free(engine->font.pixels);
    */

    ui_destroy(&engine->ui);
    window_destroy(&engine->window);
}

void engine_handle_input(Engine* engine, float dt)
{
    Camera* camera = &engine->renderer.camera;

    // TODO: Need to refactor all this input stuff,
    // lock mouse should be handling input maybe.
    if (!engine->lock_mouse)
    {
        return;
    }

    // TODO: Could move this to an input handler or something.
    POINT mouse_position;
    GetCursorPos(&mouse_position);

    int rel_x = engine->window.mouse_dx;
    int rel_y = engine->window.mouse_dy;

    // Reset the delta mouse position as we're now responding to it.
    engine->window.mouse_dx = 0;
    engine->window.mouse_dy = 0;

    // Calculate the camera movement in radians.
    float sens = 0.05f;
    float dy = radians(rel_x * sens);
    float dp = radians(rel_y * sens);

    camera->yaw -= dy;

    // Reset yaw after full 360 spin, accounting for amount past.
    const float TWO_PI = PI * 2;
    if (camera->yaw < -TWO_PI)
    {
        camera->yaw += TWO_PI;
    }
    else if (camera->yaw > TWO_PI)
    {
        camera->yaw -= TWO_PI;
    }

    camera->pitch -= dp;

    // Clamp pitch so we don't look past 90 degrees behind the camera.
    float maxPitch = PI_2 - 0.001f;
    camera->pitch = min(max(camera->pitch, -maxPitch), maxPitch);

    float cosPitch = cosf(camera->pitch);

    // Calculate the camera's direction.
    camera->direction[0] = sinf(camera->yaw) * cosPitch;
    camera->direction[1] = sinf(camera->pitch);
    camera->direction[2] = cosf(camera->yaw) * cosPitch;
    normalise(camera->direction);


    // TODO: How do I make the engine actually m/s?

    // Direct position changes must be multipled by dt.
    const float SPEED = 10.f;
    float meters_per_second = 10 * dt;

    // Process keyboard input.
    BYTE keys[256];
    if (!GetKeyboardState(keys))
    {
        // TODO: Handle error?
        log_error("Failed to get keyboard state.");
        return;
    }

    const int KeyDown = 0x80;

    if (keys['W'] & KeyDown)
    {
        V3 offset;
        v3_mul_f_out(camera->direction, meters_per_second, offset);
        v3_add_v3(camera->position, offset);
    }
    if (keys['S'] & KeyDown)
    {
        V3 offset;
        v3_mul_f_out(camera->direction, meters_per_second, offset);
        v3_sub_v3(camera->position, offset);
    }
    if (keys['A'] & KeyDown)
    {
        V3 up = { 0, 1, 0 };
        V3 right;
        cross(camera->direction, up, right);
        normalise(right);
        v3_mul_f(right, meters_per_second);
        v3_sub_v3(camera->position, right);
    }
    if (keys['D'] & KeyDown)
    {
        V3 up = { 0, 1, 0 };
        V3 right;
        cross(camera->direction, up, right);
        normalise(right);
        v3_mul_f(right, meters_per_second);
        v3_add_v3(camera->position, right);
    }
    if (keys[VK_LSHIFT] & KeyDown)
    {
        camera->position[1] -= meters_per_second;
    }
    if (keys[VK_SPACE] & KeyDown)
    {
        camera->position[1] += meters_per_second;
    }
}

// Window events
void engine_on_resize(void* ctx)
{
    Engine* engine = (Engine*)ctx;

    Status status = renderer_resize(&engine->renderer, 
        engine->window.width, engine->window.height);

    if (STATUS_OK != status)
    {
        // TODO: What to do here?
    }
}

void engine_process_keyup(void* ctx, WPARAM wParam) 
{
    Engine* engine = (Engine*)ctx;

    // TODO: Switch.
    if (VK_TAB == wParam)
    {   
        ShowCursor(engine->lock_mouse);
        engine->lock_mouse = !engine->lock_mouse;
        
        if (engine->lock_mouse)
        {
            RECT rect = { 0 };
            GetClientRect(engine->window.hwnd, &rect);

            POINT center = { 0 };
            center.x = (rect.left + rect.right) / 2;
            center.y = (rect.top + rect.bottom) / 2;

            // Reset the cursor to the center of the screen.
            ClientToScreen(engine->window.hwnd, &center);
            SetCursorPos(center.x, center.y);

            // Reset the delta mouse movement.
            engine->window.mouse_dx = 0;
            engine->window.mouse_dy = 0;

            // Restrict the cursor to the center of the screen.
            RECT cursor_area = { 0 };
            cursor_area.left = center.x;
            cursor_area.top = center.y;
            cursor_area.right = center.x;
            cursor_area.bottom = center.y;
            
            ClipCursor(&cursor_area);
        }
        else
        {
            // Release the cursor so it can move freely..
            ClipCursor(NULL);
        }
    }
    else if (VK_ESCAPE == wParam)
    {
        engine->running = 0;
        PostQuitMessage(0);
    }

    // Fire the engine callback.
    // TODO: Would be nice to not use WPARAM.
    engine_on_keyup(engine, wParam);
}