#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include "Window.h"
#include "Math.h"

// 1D array w/ top left pixel being (0,0)
// TODO: maybe make this a 2D array and flip so bottom left is (0,0), thats what I know
Vec4f* color_buffer = nullptr;

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
        }

        clear_color_buffer(Vec4f(1.0f, 0.0f, 1.0f, 1.0f));

        // Draw one point at center of screen
        rasterize_point(Vec3f(window.width/2.0f, window.height/2.0f, 0.0f), Vec4f(1.0f, 1.0f, 1.0f, 1.0f), 20);

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
    // width can be 2x2, 4x4, 6x6, ...
    // Ignoring clipping, depth, and AA for now.

    // Center of point
    int center_x = point.x; // round down
    int center_y = point.y; // round down

    // Bottom left of point
    int bottom_left_x  = center_x - width/2;
    int bottom_left_y  = center_y - width/2;

    for (int i = bottom_left_y; i < bottom_left_y + width; i++)
    {
        for (int j = bottom_left_x; j < bottom_left_x + width; j++)
        {
            // Frag grid coords in device space
            int pixel_x = j;
            int pixel_y = (window.height - 1) - i; // flip cause top left is (0,0)

            int index = pixel_y * window.width + pixel_x;
            color_buffer[index] = color;
        }
    }
}