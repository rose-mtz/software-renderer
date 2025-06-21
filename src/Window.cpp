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
int to_rgba_int(Vec4f rgba)
{
    int r_int = (rgba.x * 255.9999f);
    int g_int = (rgba.y * 255.9999f);
    int b_int = (rgba.z * 255.9999f);
    int a_int = (rgba.w * 255.9999f);

    int result = (a_int << 24) | (b_int << 16) | (g_int << 8) | r_int;
    return result;
}

// Assumes pixels is same size as global window.
void blit_window(Vec4f *pixels)
{
    for (int i = 0; i < window.width * window.height; i++)
    {
        window.pixels[i] = to_rgba_int(pixels[i]);
    }

    SDL_BlitSurface(window.draw_surface, NULL, SDL_GetWindowSurface(window.handle), NULL);
    SDL_UpdateWindowSurface(window.handle);
}