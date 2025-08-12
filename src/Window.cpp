#include <iostream>
#include "Window.h"
#include <cmath>

Window window;

void init_window(int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        std::cout << "Error: could not initialize SDL\n";
        return;
    }

    // Create window
    const char *title = "Particle System";
    window.handle = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    window.width = width;
    window.height = height;

    // Create draw surface
    window.pixels = new int[width * height];
    int row_size_bytes = 4 * width; // 4 bytes per pixel
    window.draw_surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, window.pixels, row_size_bytes);
    SDL_SetSurfaceBlendMode(window.draw_surface, SDL_BLENDMODE_NONE); // no blending
}

void resize_window(int width, int height)
{
    // Free surface & pixels
    SDL_DestroySurface(window.draw_surface);
    window.draw_surface = nullptr;
    delete[] window.pixels;
    window.pixels = nullptr;

    // Update state
    window.width = width;
    window.height = height;

    // Create new draw surface & pixels
    window.pixels = new int[width * height];
    int row_size_bytes = 4 * width; // 4 bytes per pixel
    window.draw_surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, window.pixels, row_size_bytes);
    SDL_SetSurfaceBlendMode(window.draw_surface, SDL_BLENDMODE_NONE); // no blending
}

// Assumes rgba are all in [0,1]
int to_rgb_int(float* rgb)
{
    int r_int = (rgb[0] * 255.9999f);
    int g_int = (rgb[1] * 255.9999f);
    int b_int = (rgb[2] * 255.9999f);

    int result = (0xFF << 24) | (b_int << 16) | (g_int << 8) | r_int;
    return result;
}

// Assumes pixels is same size as global window.
// Note, SDL has top left as (0,0), but input should have (0,0) as bottom left
void blit_window(float *pixels)
{
    int pixel_count = window.width * window.height;
    int offset = pixel_count;
    for (int i = 0; i < pixel_count; i++)
    {
        if (i % window.width == 0) offset -= window.width;
        int mapped_index = offset + (i % window.width);

        window.pixels[mapped_index] = to_rgb_int(&pixels[i * 3]);
    }

    SDL_BlitSurface(window.draw_surface, NULL, SDL_GetWindowSurface(window.handle), NULL);
    SDL_UpdateWindowSurface(window.handle);
}

void poll_events()
{
    window.input.window.did_resize = false;
    window.input.mouse.did_move = false;
    window.input.mouse.left.prev_state = window.input.mouse.left.is_down;
    window.input.mouse.right.prev_state = window.input.mouse.right.is_down;

    for (int i = 0; i < KEY_COUNT; i++)
    {
        window.input.keys[(KEY_CODES) i].prev_state = window.input.keys[(KEY_CODES) i].is_down;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                window.input.quit = true;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                window.input.window.did_resize = true;
                window.input.window.new_width = event.window.data1;
                window.input.window.new_height = event.window.data2;
                break;
            case SDL_EVENT_MOUSE_MOTION:
                window.input.mouse.did_move = true;
                window.input.mouse.delta.x = (event.motion.x - window.input.mouse.pos.x);
                window.input.mouse.delta.y = (event.motion.y - window.input.mouse.pos.y);
                window.input.mouse.pos.x = event.motion.x;
                window.input.mouse.pos.y = event.motion.y;
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                switch (event.button.button)
                {
                    case SDL_BUTTON_LEFT:
                        window.input.mouse.left.is_down = false;
                        break;
                    case SDL_BUTTON_RIGHT:
                        window.input.mouse.right.is_down = false;
                        break;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                switch (event.button.button)
                {
                    case SDL_BUTTON_LEFT:
                        window.input.mouse.left.is_down = true;
                        break;
                    case SDL_BUTTON_RIGHT:
                        window.input.mouse.right.is_down = true;
                        break;
                }
                break;
            case SDL_EVENT_KEY_DOWN:
                switch (event.key.key)
                {
                    case SDLK_A:
                        window.input.keys[KEY_A].is_down = true;
                        break;
                    case SDLK_SPACE:
                        window.input.keys[KEY_SPACE].is_down = true;
                        break;
                    case SDLK_RETURN:
                        window.input.keys[KEY_ENTER].is_down = true;
                        break;
                    case SDLK_UP:
                        window.input.keys[KEY_UP].is_down = true;
                        break;
                    case SDLK_DOWN:
                        window.input.keys[KEY_DOWN].is_down = true;
                        break;
                    case SDLK_LEFT:
                        window.input.keys[KEY_LEFT].is_down = true;
                        break;
                    case SDLK_RIGHT:
                        window.input.keys[KEY_RIGHT].is_down = true;
                        break;
                }
                break;
            case SDL_EVENT_KEY_UP:
                switch (event.key.key)
                {
                    case SDLK_A:
                        window.input.keys[KEY_A].is_down = false;
                        break;
                    case SDLK_SPACE:
                        window.input.keys[KEY_SPACE].is_down = false;
                        break;
                    case SDLK_RETURN:
                        window.input.keys[KEY_ENTER].is_down = false;
                        break;
                    case SDLK_UP:
                        window.input.keys[KEY_UP].is_down = false;
                        break;
                    case SDLK_DOWN:
                        window.input.keys[KEY_DOWN].is_down = false;
                        break;
                    case SDLK_LEFT:
                        window.input.keys[KEY_LEFT].is_down = false;
                        break;
                    case SDLK_RIGHT:
                        window.input.keys[KEY_RIGHT].is_down = false;
                        break;
                }
                break;
        }
    }
}