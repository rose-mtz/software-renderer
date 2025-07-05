#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include <cmath>
#include "Window.h"
#include "Math.h"

// Device space: top left of screen is (0,0)

// 1D array, first pixel is top-left pixel, last is bottom right pixel
Vec4f* color_buffer = nullptr;

Vec2f mouse_pos;

void resize_color_buffer();
void clear_color_buffer(Vec4f color);
void rasterize_point(Vec3f point, Vec4f color, int width);
void rasterize_line(Vec2f point_1, Vec2f point_2, Vec4f color, int width);

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

        rasterize_line(Vec2f(window.width/2.0f,window.height/2.0f), mouse_pos, Vec4f(1.0f, 1.0f, 1.0f, 1.0f), 10);
        rasterize_point(Vec3f(mouse_pos.x, mouse_pos.y, 0.0f), Vec4f(1.0f, 0.0f, 0.0f, 1.0f), 5);

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
    // Ignoring depth, and AA for now
    
    // Width can be 2x2, 4x4, 6x6, ...
    width = (width) * 2;

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

void rasterize_line(Vec2f point_1, Vec2f point_2, Vec4f color, int width)
{
    // Each increase in width adds 1 layer of 'thickness' (i.e two lines on each side)
    int line_count = 1 + (width - 1) * 2;

    float m = (point_2.y - point_1.y) / (point_2.x - point_1.x);
    float inc_x;
    float inc_y;
    float steep_slope = m > 1.0f || m < -1.0f;
 
    if (!steep_slope)
    {
        // March in increasing x-direction
        if (point_1.x > point_2.x) 
        {
            Vec2f temp = point_1;
            point_1 = point_2;
            point_2 = temp;
        }   

        inc_x = 1.0f;
        inc_y = m;
    }
    else
    {
        // March in increasing y-direction
        if (point_1.y > point_2.y) 
        {
            Vec2f temp = point_1;
            point_1 = point_2;
            point_2 = temp;
        }

        inc_y = 1.0f;
        inc_x = 1.0f/m;   
    }

    Vec2f cur_point = point_1;
    while (true) // March line
    {
        int rounded_x = std::round(cur_point.x);
        int rounded_y = std::round(cur_point.y);

        for (int i = 0; i < line_count; i++) // thickness
        {
            if (!steep_slope)
            {
                int current_line_y = (rounded_y - (width - 1)) + i;
                if (current_line_y < 0 || current_line_y >= window.height) continue; // out of bounds fragment

                int index = rounded_x + current_line_y * window.width;
                color_buffer[index] = color;
            }
            else
            {
                int current_line_x = (rounded_x - (width - 1)) + i;
                if (current_line_x < 0 || current_line_x >= window.width) continue; // out of bounds fragment

                int index = current_line_x + rounded_y * window.width;
                color_buffer[index] = color;
            }
        }

        cur_point.x += inc_x;
        cur_point.y += inc_y;

        // March ends when past point_2
        if ((steep_slope && cur_point.y > point_2.y) || (!steep_slope && cur_point.x > point_2.x))
        {
            break;
        }
    }
}