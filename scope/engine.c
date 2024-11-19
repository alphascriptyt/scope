#include "engine.h"

#include "canvas.h"
#include "models.h"
#include "lights.h"
#include "render.h"
#include "camera.h"
#include "texture.h"

#include "maths/utils.h"

#include "utils/logger.h"
#include "utils/str_utils.h"
#include "utils/timer.h"

#include <Windows.h>

#include <stdio.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Engine* engine = 0;

    switch (uMsg)
    {
    case WM_NCCREATE:
    {
        // Recover the engine pointer.
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
        engine = (Engine*)lpcs->lpCreateParams;

        if (!engine)
        {
            printf("FAILLLLLL\N");
        }

        // Store the pointer safely for future use.
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)engine);

        break;
    }
    case WM_DESTROY:
    {
        printf("Call destroy event\n");
        PostQuitMessage(0);
        return S_OK;

    }
    case WM_EXITSIZEMOVE:
    {
        // Recover the engine pointer.
        engine = (Engine*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (engine)
        {
            on_resize(engine);
        }
        break;
    }
    case WM_KEYUP:
    {
        // Recover the engine pointer.
        engine = (Engine*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (engine)
        {
            if (VK_TAB == wParam)
            {
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
                }
            }
            else if (VK_ESCAPE == wParam)
            {
                engine->game_running = 0;
            }

            else if ('1' == wParam)
            {
                engine->render_settings->fov += 5;
                printf("FOV: %f\n", engine->render_settings->fov);
                update_projection_m4(engine->render_settings,
                    engine->render_target->canvas);
            }
            else if ('2' == wParam)
            {
                
                engine->render_settings->fov -= 5;
                printf("FOV: %f\n", engine->render_settings->fov);
                update_projection_m4(engine->render_settings,
                    engine->render_target->canvas);
            }

            // TODO: on_keyup?
        }

        break;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

Engine* init_engine()
{
    Engine* engine = malloc(sizeof(Engine));

    if (!engine)
    {
        printf("Failed to malloc for engine.\n");
        return 0;
    }

    // Initialise settings.
    engine->upscaling_factor = 1;

    // TODO: Move this stuff to a window class?
    // Or is that unnecessary.
    engine->window_width = 1600;
    engine->window_height = 900;
    
    // Create the render target.
    // TODO: I think refactoring the render target could be necessary. Although it makes sense to wrap the canvas and depth buffer so idk.
    engine->render_target = create_render_target((int)(engine->window_width / engine->upscaling_factor), (int)(engine->window_height / engine->upscaling_factor));

    engine->font = load_font();

    // Init the input state.
    engine->previous_mouse_x = 0;
    engine->previous_mouse_y = 0;
    engine->lock_mouse = 0;


    // TODO: Handle failure
    create_window(engine, "Test");

    return engine;
}


// TEMP!
#include "utils/memory_utils.h"

void start_engine(Engine* engine)
{
    RenderSettings render_settings = {
        .fov = 60.f,
        .near_plane = 1.f,
        .far_plane = 100.f,
    };

    update_projection_m4(&render_settings, engine->render_target->canvas);
    engine->render_settings = &render_settings;


    // TODO: How can textures be stored? I think just an array of texture* then the mesh has a texture id.
    Texture* menzter_texture = load_texture_from_bmp("C:\\Users\\olive\\source\\repos\\scope\\scope\\res\\textures\\menzter.bmp");
    
    // Use calloc to initialise all members to 0.
    engine->models = calloc(1, sizeof(Models));
    if (0 == engine->models)
    {
        log_error("Failed to calloc for Models.");
        return; // TODO: Status.
    }

    init_models(engine->models);



    engine->point_lights = calloc(1, sizeof(PointLights));
    if (0 == engine->point_lights)
    {
        log_error("Failed to calloc for point_lights.");
        return; // TODO: Status.
    }

    log_info("Engine started!");

    V3 pos = { 0, 0, -3 };
    V3 pos1 = { 0, 0, -10 };

    // TODO: this is currently PITCH, YAW, ROLL. This doesn't make much sense calling this eulers.
    V3 eulers = { 0, 0,  0}; // I think I'd rather have the option to set pitch,yaw,roll. 
    V3 eulers1 = { 0.5, 0,  0 }; // I think I'd rather have the option to set pitch,yaw,roll. 

    // TODO: Rename everything from orientation to eulers as thats what it is
    // TODO: Helper function to convert pitch,yaw,roll to direction and vice versa?
    V3 scale = { 1, 1, 1 };
    V3 plane_scale = { 10, 0.1, 10 };
    V3 scale1 = { 4, 4, 4 };
    
    //load_mesh_from_obj(engine->models, "C:/Users/olive/source/repos/scope/scope/res/models/suzanne.obj", pos, eulers1, scale);
    //load_mesh_from_obj(engine->models, "C:/Users/olive/source/repos/scope/scope/res/models/menzter.obj", pos1, eulers, plane_scale);

    ///// TEMP: TESTING.

    load_model_base_from_obj(engine->models, "C:/Users/olive/source/repos/scope/scope/res/models/suzanne.obj");
    load_model_base_from_obj(engine->models, "C:/Users/olive/source/repos/scope/scope/res/models/menzter.obj");
 
    int n0 = 1000;
    create_model_instances(engine->models, 0, n0);

    for (int i = 0; i < n0; ++i)
    {
        int index_transform = i * STRIDE_MI_TRANSFORM;

        engine->models->mis_transforms[index_transform] = pos[0];
        engine->models->mis_transforms[++index_transform] = pos[1];
        engine->models->mis_transforms[++index_transform] = pos[2]-i*3;
        engine->models->mis_transforms[++index_transform] = eulers1[0];
        engine->models->mis_transforms[++index_transform] = eulers1[1];
        engine->models->mis_transforms[++index_transform] = eulers1[2];
        engine->models->mis_transforms[++index_transform] = scale[0];
        engine->models->mis_transforms[++index_transform] = scale[1];
        engine->models->mis_transforms[++index_transform] = scale[2];

        engine->models->mis_transforms_updated_flags[i] = 1;
    }
    
    
    /*
    int n1 = 0;
    create_model_instances(engine->models, 0, n1);
    for (int i = n0; i < n0 + n1; ++i)
    {
        int index_transform = i * STRIDE_MI_TRANSFORM;

        engine->models->mis_transforms[index_transform] = pos[0] + 3;
        engine->models->mis_transforms[++index_transform] = pos[1];
        engine->models->mis_transforms[++index_transform] = pos[2] - i * 3;
        engine->models->mis_transforms[++index_transform] = eulers1[0];
        engine->models->mis_transforms[++index_transform] = eulers1[1];
        engine->models->mis_transforms[++index_transform] = eulers1[2];
        engine->models->mis_transforms[++index_transform] = scale[0];
        engine->models->mis_transforms[++index_transform] = scale[1];
        engine->models->mis_transforms[++index_transform] = scale[2];

        engine->models->mis_transforms_updated_flags[i] = 1;
    }
    */
    /*
    int n2 = 0;
    create_model_instances(engine->models, 1, n0);

    for (int i = n0 + n1; i < n0 + n1 + n2; ++i)
    {
        int index_transform = i * STRIDE_MI_TRANSFORM;

        engine->models->mis_transforms[index_transform] = pos[0];
        engine->models->mis_transforms[++index_transform] = pos[1];
        engine->models->mis_transforms[++index_transform] = pos[2] - i * 3;
        engine->models->mis_transforms[++index_transform] = eulers1[0];
        engine->models->mis_transforms[++index_transform] = eulers1[1];
        engine->models->mis_transforms[++index_transform] = eulers1[2];
        engine->models->mis_transforms[++index_transform] = scale[0];
        engine->models->mis_transforms[++index_transform] = scale[1];
        engine->models->mis_transforms[++index_transform] = scale[2];

        engine->models->mis_transforms_updated_flags[i] = 1;
    }*/
    

    V3 pos2 = { 5, 5, 0 };
    V3 red = { 1,0,0 };
    create_point_light(engine->point_lights, pos2, red, 3);

    char fps_str[64] = "fps";
    engine->ui_text[0] = create_text(fps_str, 10, 10, 0x00FF0000, 3);

    char dir_str[64] = "dir";
    engine->ui_text[1] = create_text(dir_str, 10, 40, 0x0000FF00, 3);

    char pos_str[64] = "pos";
    engine->ui_text[2] = create_text(pos_str, 10, 70, 0x0000FF00, 3);


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

    Camera camera =
    {
        .direction = {0, 0, -1.f},
        .position = {0, 0, 10.f},
        .yaw = PI // TODO: I would like to decouple this so the direction doesn't matter.
                    //       COuld just make a create_camera(position,direction) that sets the eulers.
    };
    M4 view_matrix;

    float dir = 1;
    engine->game_running = 1;
    while (engine->game_running)
    {
        // Handle window messages.
        if (!process_window_messages())
        {
            engine->game_running = 0;
            break;
        }

        // TODO: Handle input so I can see the effects better
        handle_input(engine, &camera, dt);
        calculate_view_matrix(&camera, view_matrix);

        
        // TEMP, spinning and scaling cause why not
        if (eulers[1] > radians(360))
        {
            eulers[0] = 0;
            eulers[1] = 0;
        }
        else
        {
            eulers[0] -= dt;
            eulers[1] += dt;
        }

        if (scale[0] > 3)
        {
            dir = -1;
        }
        else if (scale[0] < 1)
        {
            dir = 1;
        }
        scale[0] += dt * dir;
        scale[1] += dt * dir;
        scale[2] += dt * dir;

        M4 model_matrix;
        make_model_m4(pos, eulers, scale, model_matrix);

        engine->models->mis_transforms[3] = eulers[0];
        engine->models->mis_transforms[4] = eulers[1];
        engine->models->mis_transforms[5] = eulers[2];
        //engine->models->mis_transforms[6] = scale[0];
        //engine->models->mis_transforms[7] = scale[1];
        //engine->models->mis_transforms[8] = scale[2];


        engine->models->mis_transforms_updated_flags[0] = 1;
        

        // Clear the canvas.
        //fill_canvas(engine->canvas, 0x22222222);
        clear_render_target(engine->render_target, 0x22222222);

        // Render scene.
        render(engine->render_target, engine->render_settings, 
            engine->models, engine->point_lights, view_matrix);

        // Draw ui elements.
        draw_ui(engine);

        // Update the display.
        display_window(engine);

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

        snprintf(dir_str, sizeof(dir_str), "DIR: %.2f %.2f %.2f", camera.direction[0], camera.direction[1], camera.direction[2]);
        snprintf(pos_str, sizeof(pos_str), "POS: %.2f %.2f %.2f", camera.position[0], camera.position[1], camera.position[2]);

    }

    cleanup_engine(engine);
}

void create_window(Engine* engine, const char* title)
{
    DWORD window_style = WS_OVERLAPPEDWINDOW;

    // Use our desired client area (width, height) to calculate the 
    // full size of the window including the titlebar/borders.
    RECT rect = {
        .right = engine->window_width,
        .bottom = engine->window_height
    };
    if (!AdjustWindowRect(&rect, window_style, 0))
    {
        printf("ERROR: adjustwindowrect\n");
    }

    // Use the full size of the window, not the client area.
    int actual_width = rect.right - rect.left;
    int actual_height = rect.bottom - rect.top;

    HMODULE hinstance = GetModuleHandleA(NULL);

    // Define the window's class.
    WNDCLASS wc = {
        .lpszClassName = SCOPE_WINDOW_CLASS,
        .hInstance = hinstance,
        .lpfnWndProc = WindowProc,
        .hIcon = LoadIcon(NULL, IDI_APPLICATION),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .style = CS_OWNDC
    };

    if (!RegisterClassA(&wc))
    {
        printf("ERROR: RegisterClassA\n");
    }

    // Create the window
    engine->hwnd = CreateWindowExA(
        0,                          // Optional window styles
        SCOPE_WINDOW_CLASS,         // Window class
        title,                      // Window caption
        window_style,

        // Size and position
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        actual_width,
        actual_height,

        NULL,                   // Parent window    
        NULL,                   // Menu
        hinstance,              // Instance handle
        engine                    // Additional application data
    );

    if (engine->hwnd == NULL)
    {
        // TODO: Handle error, logging? LOok into handling constructor errors
        // Failed to create window
        //GetLastError()...
        printf("TODO: m_hwnd == NULL\n");
        return;
    }
        
    // Store the pointer to the engine, so that the WindowProc can fire callbacks.
    //SetWindowLongPtr(engine->hwnd, GWLP_USERDATA, engine);

    // Get the Device Context, as we are only drawing to it from this class,
    // I believe it is fine to keep this handle and not release it.
    engine->hdc = GetDC(engine->hwnd);

    // TODO: Think about this. Can I make it faster.
    // Look at: https://www.youtube.com/watch?v=hNKU8Jiza2g&list=PLnuhp3Xd9PYTt6svyQPyRO_AAuMWGxPzU&index=16
    // Handmade Hero 004.

    // Get the new size and create the frame bitmap info.
    engine->canvas_bitmap.bmiHeader.biSize = sizeof(engine->canvas_bitmap.bmiHeader);
    engine->canvas_bitmap.bmiHeader.biWidth = engine->render_target->canvas->width;
    engine->canvas_bitmap.bmiHeader.biHeight = -engine->render_target->canvas->height;
    engine->canvas_bitmap.bmiHeader.biPlanes = 1;
    engine->canvas_bitmap.bmiHeader.biBitCount = 32; // TODO: Make 24bit no need for A?
    engine->canvas_bitmap.bmiHeader.biCompression = BI_RGB; // Uncompressed.
    
    ShowWindow(engine->hwnd, SW_SHOW);
}

int process_window_messages()
{
    // TODO: Some heap is corrupted so i keep getting an error here......
    // TODO: I reckon the error is storing our pointer in the window?

    // Processes all messages and sends them to WindowProc
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            printf("quit\n");
            return FALSE;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return TRUE;
}

void draw_ui(Engine* engine)
{
    for (int i = 0; i < TEXT_COUNT; ++i)
    { 
        // TODO: Text needs to be scaled. Or how do I do it not scaled.
        draw_text(engine->render_target->canvas, &engine->ui_text[i], &engine->font, engine->upscaling_factor);
    }
}

void handle_input(Engine* engine, Camera* camera, float dt)
{
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
    }
}

void display_window(Engine* engine)
{
   
    // TODO: Could look into ways of speeding up blitting, however,
    //       not sure it's really possible, unless there is some 
    //       issue with the width/height needing to be multiples 
    //       of something..
    if (engine->upscaling_factor > 1)
    {
        StretchDIBits(
            engine->hdc,
            0, 0,
            engine->window_width, engine->window_height,
            0, 0,
            engine->render_target->canvas->width, engine->render_target->canvas->height,
            engine->render_target->canvas->pixels,
            &engine->canvas_bitmap,
            DIB_RGB_COLORS,
            SRCCOPY);
    }
    else
    {
        // This gives slightly better performance than StretchDIBits.
        SetDIBitsToDevice(
            engine->hdc,
            0, 0,
            engine->render_target->canvas->width, engine->render_target->canvas->height,
            0, 0,
            0,
            engine->render_target->canvas->height,
            engine->render_target->canvas->pixels,
            &engine->canvas_bitmap,
            DIB_RGB_COLORS);
    }   
}

void on_resize(Engine* engine)
{
    // Calculate the new window dimensions.
    RECT rect;
    GetClientRect(engine->hwnd, &rect);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Check the dimensions are different.
    if (width == engine->window_width && height == engine->window_height)
    {
        return;
    }

    // Update the dimensions.
    engine->window_width = width;
    engine->window_height = height;

    // Resize the render target.
    resize_render_target(engine->render_target, (int)(width / engine->upscaling_factor), (int)(height / engine->upscaling_factor));

    // Update the projection matrix.
    update_projection_m4(engine->render_settings, engine->render_target->canvas);

    // Update the bitmapinfo.
    engine->canvas_bitmap.bmiHeader.biWidth = engine->render_target->canvas->width;
    engine->canvas_bitmap.bmiHeader.biHeight = -engine->render_target->canvas->height;
}

void cleanup_engine(Engine* engine)
{
    free_models(engine->models);
    destroy_render_target(engine->render_target);

    free(engine->models);
    free(engine->point_lights);
    free(engine->font.pixels);
}