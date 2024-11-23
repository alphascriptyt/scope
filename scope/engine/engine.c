#include "engine.h"

#include "utils/logger.h"

#include <Windows.h>

#include <stdio.h>

Status engine_init(Engine* engine, int window_width, int window_height)
{
    log_info("Initialising the engine.");
    memset(engine, 0, sizeof(Engine));

    // Initialise the renderer.
    Status status = renderer_init(&engine->renderer, window_width, window_height);
    if (STATUS_OK != status)
    {
        log_error("Failed to renderer_init because of %s", status_to_str(status));
        return status;
    }

    // Initialise the window.
    status = window_init(&engine->window, &engine->renderer.target.canvas, (void*)engine);
    if (STATUS_OK != status)
    {
        log_error("Failed to window_init because of %s", status_to_str(status));
        return status;
    }

    // Set window callbacks.
    engine->window.on_resize = &engine_on_resize;
    engine->window.on_keyup = &engine_on_keyup;

    // Initialise the UI.
    status = ui_init(&engine->ui, &engine->renderer.target.canvas);
    if (STATUS_OK != status)
    {
        log_error("Failed to ui_init because of %s", status_to_str(status));
        return status;
    }

    


    // Set some default settings.
    //engine->upscaling_factor = 1;

    // Init the input state.
    //engine->previous_mouse_x = 0;
    //engine->previous_mouse_y = 0;
    //engine->lock_mouse = 0;

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

    // Start the timers.
    QueryPerformanceCounter(&startTime);

    float dt_counter = 0;

    char fps_str[64] = "fps";
    char dir_str[64] = "dir";
    char pos_str[64] = "pos";

    engine->running = 1;
    while (engine->running)
    {
        // Process the application window messages.
        if (!window_process_messages())
        {
            break;
        }

        engine_handle_input(engine, dt);

        // Clear the canvas.
        //fill_canvas(engine->canvas, 0x22222222);
        render_target_clear(&engine->renderer.target, 0x22222222);

        /*
        // Render scene.
        render(engine->render_target, engine->render_settings,
            engine->models, engine->point_lights, view_matrix);
        */

        // Draw ui elements.
        ui_draw(&engine->ui, 1.f); // TODO: upscaling_factor.
        
        // Update the display.
        window_display(&engine->window);

        // Calculate performance.
        QueryPerformanceCounter(&endTime);
        dt = (float)(endTime.QuadPart - startTime.QuadPart) / frequency.QuadPart;

        dt_counter += dt;

        startTime = endTime;

        fps = (int)(1.0f / dt);

        if (dt_counter > 2)
        {
            snprintf(fps_str, sizeof(fps_str), "FPS: %d", fps);
            printf("%s\n", fps_str);
            dt_counter = 0;
        }

        //snprintf(dir_str, sizeof(dir_str), "DIR: %.2f %.2f %.2f", camera.direction[0], camera.direction[1], camera.direction[2]);
        //snprintf(pos_str, sizeof(pos_str), "POS: %.2f %.2f %.2f", camera.position[0], camera.position[1], camera.position[2]);
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
    /*
    Camera* camera = &engine->renderer.camera;

    // TODO: Need to refactor all this input stuff,
    // lock mouse should be handling input maybe.
    if (!engine->lock_mouse)
    {
        return;
    }

    // TODO: Could move this to an input folder

    POINT mouse_position;
    GetCursorPos(&mouse_position);

    int rel_x = mouse_position.x - engine->previous_mouse_x;
    int rel_y = mouse_position.y - engine->previous_mouse_y;

    if (engine->lock_mouse)
    {
        RECT rect = { 0 };
        GetClientRect(engine->hwnd, &rect);
        POINT center = { 0 };
        center.x = (rect.left + rect.right) / 2;
        center.y = (rect.top + rect.bottom) / 2;

        ClientToScreen(engine->hwnd, &center);
        SetCursorPos(center.x, center.y);

        engine->previous_mouse_x = center.x;
        engine->previous_mouse_y = center.y;
    }

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
    }*/
}

// Events
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

void engine_on_keyup(void* ctx, WPARAM wParam) 
{
    Engine* engine = (Engine*)ctx;

    if (VK_TAB == wParam)
    {
        /*
        ShowCursor(engine->lock_mouse);
        engine->lock_mouse = !engine->lock_mouse;

        // Reset the cursor to the middle when we lock the mouse to avoid
        // a jump in movement.
        if (engine->lock_mouse)
        {
            RECT rect = { 0 };
            GetClientRect(engine->hwnd, &rect);
            POINT center = { 0 };
            center.x = (rect.left + rect.right) / 2;
            center.y = (rect.top + rect.bottom) / 2;

            ClientToScreen(engine->hwnd, &center);
            SetCursorPos(center.x, center.y);

            engine->previous_mouse_x = center.x;
            engine->previous_mouse_y = center.y;
        }*/
    }
    else if (VK_ESCAPE == wParam)
    {
        engine->running = 0;
        PostQuitMessage(0);
    }
}