#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include "Window.h"
#include "Math.h"

// Device space: top left of screen is (0,0)

// 1D array, first pixel is top-left pixel, last is bottom right pixel
Vec4f* color_buffer = nullptr;

Vec2f mouse_pos;

void resize_color_buffer();
void clear_color_buffer(Vec4f color);
void rasterize_point(Vec3f point, Vec4f color, int width);

int main()
{
    init_window(640, 480);    
    resize_color_buffer();

    bool running = true;
    auto last_frame_start = std::chrono::high_resolution_clock::now();
    while (running)
    {
        auto current_frame_start = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(current_frame_start - last_frame_start).count();
        last_frame_start = current_frame_start;
        std::cout << dt << std::endl;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                resize_window(event.window.data1, event.window.data2);   
                resize_color_buffer();
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                mouse_pos.x = event.motion.x;
                mouse_pos.y = event.motion.y;
            }
        }

        clear_color_buffer(Vec4f(1.0f, 0.0f, 1.0f, 1.0f));

        // Draw one point that follows mouse
        rasterize_point(Vec3f(mouse_pos.x, mouse_pos.y, 0.0f), Vec4f(1.0f, 1.0f, 1.0f, 1.0f), 8);

        blit_window(color_buffer);
    }
}

// Resizes/init to match window size
void resize_color_buffer()
{
    if (!color_buffer) delete[] color_buffer;

    color_buffer = new Vec4f[window.width * window.height];
}

void clear_color_buffer(Vec4f color)
{
    for (int i = 0; i < window.width * window.height; i++)
    {
        color_buffer[i] = color;
    }
}

void rasterize_point(Vec3f point, Vec4f color, int width)
{
    // Assumes point is in device coords, and is inside bounds.
    // Width can be 2x2, 4x4, 6x6, ...
    // Ignoring depth, and AA for now

    // Center of point
    int center_x = point.x; // round down
    int center_y = point.y; // round down

    // Bottom left of point
    int bottom_left_x  = center_x - width/2;
    int bottom_left_y  = center_y - width/2;

    for (int i = bottom_left_y; i < bottom_left_y + width; i++)
    {
        if (i < 0 || i >= window.height) continue; // out of bounds fragment

        for (int j = bottom_left_x; j < bottom_left_x + width; j++)
        {
            if (j < 0 || j >= window.width) continue; // out of bounds fragment

            int index = i * window.width + j;
            color_buffer[index] = color;
        }
    }
}