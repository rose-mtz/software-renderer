#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
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
void rasterize_trapezoid(Vec2f p1, Vec2f p2, Vec2f p3, Vec2f p4, Vec4f color);

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

        // rasterize_line(Vec2f(window.width/2.0f,window.height/2.0f), mouse_pos, Vec4f(1.0f, 1.0f, 1.0f, 1.0f), 10);
        // rasterize_point(Vec3f(mouse_pos.x, mouse_pos.y, 0.0f), Vec4f(1.0f, 0.0f, 0.0f, 1.0f), 5);

        float bottom_y = window.height * 0.25f;
        Vec2f bottom_left (window.width * 0.20, bottom_y);
        Vec2f bottom_right (window.width * 0.80, bottom_y);
        float top_y = mouse_pos.y; // window.height * 0.75;
        Vec2f top_left (window.width* 0.15, top_y);
        Vec2f top_right (mouse_pos.x, top_y);

        rasterize_trapezoid(bottom_left, top_left, top_right, bottom_right, Vec4f(1.0f, 1.0f, 1.0f, 1.0f));

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
        if ((steep_slope && cur_point.y > point_2.y) || (!steep_slope && cur_point.x > point_2.x)) // BUG?: point_2.x/y = right/top edge of screen
        {
            break;
        }
    }
}

// Trapezoid MUST have flat top & bottoms
void rasterize_trapezoid(Vec2f p1, Vec2f p2, Vec2f p3, Vec2f p4, Vec4f color)
{  
    // IDEA: I think its safe to assume culling for backfaceing traps has already occurred
    // IDEA: raster policy: don't rasterize at or past top edge
    // IDEA: raster policy: don't rasterize at or past right edge

    // Sort by y (bubble sort)
    if (p1.y > p2.y) std::swap(p1,p2);
    if (p2.y > p3.y) std::swap(p2,p3);
    if (p3.y > p4.y) std::swap(p3,p4);
    if (p2.y > p3.y) std::swap(p2,p3);

    // Sort by x
    if (p1.x > p2.x) std::swap(p1,p2);
    if (p3.x > p4.x) std::swap(p3,p4);

    //  p1 is bottom left
    //  p2 is bottom right
    //  p3 is top left
    //  p4 is top right

    float dx_dy_l = (p3.x - p1.x) / (p3.y - p1.y);
    float dx_dy_r = (p4.x - p2.x) / (p4.y - p2.y);

    float delta_y = 1.0f;

    float delta_h_l = ceil(p1.y) - p1.y;
    float delta_w_l = delta_h_l * dx_dy_l;
    Vec2f cur_p_l (p1.x + delta_w_l, p1.y + delta_h_l);

    float delta_h_r = ceil(p2.y) - p2.y;
    float delta_w_r = delta_h_r * dx_dy_r;
    Vec2f cur_p_r (p2.x + delta_w_r, p2.y + delta_h_r);

    while (cur_p_l.y < p3.y) // March up sides of trapezoid
    {
        int pixel_x = cur_p_l.x; // floor
        int pixel_y = cur_p_l.y; // floor, but should already be integer (decimal part is all zeros)
        int index = pixel_x + pixel_y * window.width;

        while (pixel_x < cur_p_r.x) // March left to right across trapezoid
        {
            color_buffer[index] = color;

            pixel_x++;
            index++;
        }

        cur_p_l.x += dx_dy_l;
        cur_p_l.y += delta_y;
        
        cur_p_r.x += dx_dy_r;
        cur_p_r.y += delta_y;
    }
}