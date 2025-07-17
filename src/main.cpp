#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <vector>
#include "Window.h"
#include "Math.h"
#include "Rasterize.h"


Buffer* render_buffer = nullptr;

Vec2f mouse_pos;
bool running = true;

int active_user_poly = 0;
std::vector<std::vector<Vec2f>> list_of_user_polys (1);

auto last_frame_start = std::chrono::high_resolution_clock::now();
float dt = 0;

void resize_buffer(Buffer* buffer, int width, int height);
void clear_buffer(Vec3f color, Buffer* buffer);

void draw();
void handle_events();
void handle_time();
void init();

int main()
{
    init();

    while (running)
    {
        handle_time();
        handle_events();
        draw();
    }
}

void init()
{
    render_buffer = new Buffer();

    init_window(640, 480);    
    resize_buffer(render_buffer, 640, 480);
}

void handle_time()
{
    auto current_frame_start = std::chrono::high_resolution_clock::now();
    dt = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(current_frame_start - last_frame_start).count();
    last_frame_start = current_frame_start;
    std::cout << dt << std::endl;
}

void handle_events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            running = false;
        }
        else if (event.type == SDL_EVENT_WINDOW_RESIZED)
        {
            int new_width = event.window.data1;
            int new_height = event.window.data2;

            resize_window(new_width, new_height);
            resize_buffer(render_buffer, new_width, new_height);
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            mouse_pos.x = event.motion.x;
            mouse_pos.y = window.height - event.motion.y;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            list_of_user_polys[active_user_poly].push_back(Vec2f(event.button.x, window.height - event.button.y));
        }
        else if (event.type == SDL_EVENT_KEY_DOWN)
        {
            active_user_poly++;
            list_of_user_polys.push_back(std::vector<Vec2f>());
        }
    }
}

void draw()
{
    clear_buffer(Vec3f(0.0f, 0.0f, 0.0f), render_buffer);

    for (int i = 0; i < list_of_user_polys.size(); i++)
    {
        std::vector<Vec2f>& poly = list_of_user_polys[i];

        if (poly.size() > 2)
        {
            rasterize_polygon(poly, render_buffer);
        }

        if (poly.size() > 1)
        {
            for (int j = 0; j < poly.size(); j++)
            {
                Vec2f start = poly[j];
                Vec2f end = poly[(j + 1) % poly.size()];

                rasterize_line(start, end, Vec3f(1.0f, 0.0f, 0.0f), 2, render_buffer);
            }
        }

        for (int j = 0; j < poly.size(); j++)
        {
            rasterize_point(poly[j], Vec3f(1.0f, 0.0f, 0.0f), 5, render_buffer);
        }
    }

    blit_window(render_buffer->pixels);
}

void resize_buffer(Buffer* buffer, int width, int height)
{
    if (!buffer->pixels) delete[] buffer->pixels;

    buffer->pixels = new Vec3f[width * height];
    buffer->width = width;
    buffer->height = height;
}

void clear_buffer(Vec3f color, Buffer* buffer)
{
    for (int i = 0; i < buffer->width * buffer->height; i++)
    {
        buffer->pixels[i] = color;
    }
}