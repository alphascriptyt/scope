#include "window.h"

#include "common/status.h"
#include "utils/logger.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_NCCREATE:
    {
        // TODO: Find the best way to do this.

        // Recover the window pointer.
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
        Window* window = (Window*)lpcs->lpCreateParams;

        if (!window)
        {
            log_error("Failed to recover window pointer.");
        }

        // Store the pointer safely for future use.
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)window);
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return S_OK;
    }
    case WM_EXITSIZEMOVE:
    {
        Window* window = (Window*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

        // Calculate the new window dimensions.
        RECT rect;
        GetClientRect(window->hwnd, &rect);

        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        // Check the dimensions are different.
        if (width != window->width || height != window->height)
        {
            // Update the dimensions.
            window->width = width;
            window->height = height;
            window->bitmap.bmiHeader.biWidth = width;
            window->bitmap.bmiHeader.biHeight = -height;

            // Fire the resize callback.
            window->on_resize(window->ctx);
        }

        break;
    }
    case WM_KEYUP:
    {
        Window* window = (Window*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
        window->on_keyup(window->ctx, wParam);

        break;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

Status window_init(Window* window, const Canvas* canvas, void* ctx)
{
	log_info("Initialising the window.");
	memset(window, 0, sizeof(Window));

    window->canvas = canvas;
    window->ctx = ctx;

    // Initialise the window size to the canvas size?
    window->width = canvas->width;
    window->height = canvas->height;

    DWORD window_style = WS_OVERLAPPEDWINDOW;

    // Use our desired client area (width, height) to calculate the 
    // full size of the window including the titlebar/borders.
    RECT rect = {
        .right = window->width, 
        .bottom = window->height
    };

    if (!AdjustWindowRect(&rect, window_style, 0))
    {
        log_error("Failed to AdjustWindowRect.");
        return STATUS_WIN32_FAILURE;
    }

    // Use the full size of the window, not the client area.
    const int actual_width = rect.right - rect.left;
    const int actual_height = rect.bottom - rect.top;

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
        log_error("Failed to RegisterClassA.");
        return STATUS_WIN32_FAILURE;
    }

    // Create the window
    window->hwnd = CreateWindowExA(
        0,                          // Window styles, TODO: PASS window_style?
        SCOPE_WINDOW_CLASS,         // Window class
        SCOPE_WINDOW_TITLE,         // Window caption
        window_style,

        // Size and position
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        actual_width,
        actual_height,

        NULL,                   // Parent window    
        NULL,                   // Menu
        hinstance,              // Instance handle
        window                  // Additional application data
    );

    if (NULL == window->hwnd)
    {
        log_error("Failed to CreateWindowExA.");
        return STATUS_WIN32_FAILURE;
    }

    // Get the Device Context, as we are only drawing to it from this class,
    // I believe it is fine to keep this handle and not release it.
    window->hdc = GetDC(window->hwnd);

    // TODO: Think about this. Can I make it faster.
    // Look at: https://www.youtube.com/watch?v=hNKU8Jiza2g&list=PLnuhp3Xd9PYTt6svyQPyRO_AAuMWGxPzU&index=16
    // Handmade Hero 004.

    // Get the new size and create the frame bitmap info.
    window->bitmap.bmiHeader.biSize = sizeof(window->bitmap.bmiHeader);
    window->bitmap.bmiHeader.biWidth = window->canvas->width;
    window->bitmap.bmiHeader.biHeight = -window->canvas->height;
    window->bitmap.bmiHeader.biPlanes = 1;
    window->bitmap.bmiHeader.biBitCount = 32;
    window->bitmap.bmiHeader.biCompression = BI_RGB; // Uncompressed.

    ShowWindow(window->hwnd, SW_SHOW);

    return STATUS_OK;
}

int window_process_messages()
{
    // TODO: Some heap is corrupted so i keep getting an error here......
    // TODO: The issue comes when we close the window sometimes and sometimes on startup.
    //       Potentially something to do with storing the window pointer.

    // Processes all messages and sends them to WindowProc
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            log_info("WM_QUIT message received.");
            return FALSE;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return TRUE;
}

void window_display(Window* window)
{
    // TODO: Could look into ways of speeding up blitting, however,
    //       not sure it's really possible, unless there is some 
    //       issue with the width/height needing to be multiples 
    //       of something..
    if (window->upscaling_factor > 1)
    {
        StretchDIBits(
            window->hdc,
            0, 0,
            window->width, window->height,
            0, 0,
            window->canvas->width, window->canvas->height,
            window->canvas->pixels,
            &window->bitmap,
            DIB_RGB_COLORS,
            SRCCOPY);
    }
    else
    {
        // This gives slightly better performance than StretchDIBits.
        SetDIBitsToDevice(window->hdc,
            0, 0,
            window->width, window->height,
            0, 0,
            0,
            window->height,
            window->canvas->pixels,
            &window->bitmap,
            DIB_RGB_COLORS);
    }
}

void window_destroy(Window* window)
{
    // TODO: Not sure how necessary this is.
}