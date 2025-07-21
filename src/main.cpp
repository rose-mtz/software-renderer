#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <vector>
#include "Window.h"
#include "Math.h"
#include "Rasterize.h"

Buffer* low_res_buffer = nullptr;
Buffer* screen_res_buffer = nullptr;

// Vec2f mouse_pos;
bool running = true;

int active_user_poly = 0;
std::vector<std::vector<Vertex>> list_of_user_polys (1);

auto last_frame_start = std::chrono::high_resolution_clock::now();
float dt = 0;

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
    screen_res_buffer = new Buffer();
    low_res_buffer = new Buffer();

    init_window(640, 480);    
    resize_buffer(screen_res_buffer, 640, 480);
    resize_buffer(low_res_buffer, 320, 240);
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
            resize_buffer(screen_res_buffer, new_width, new_height);
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            // mouse_pos.x = event.motion.x;
            // mouse_pos.y = window.height - event.motion.y;
        } 
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            Vec3f POLY_COLOR  (1.0f, 1.0f, 1.0f);
            float x_basis_scale = ((float) low_res_buffer->width)  / screen_res_buffer->width;
            float y_basis_scale = ((float) low_res_buffer->height) / screen_res_buffer->height;
            Vertex to_add;
            to_add.color = POLY_COLOR;
            to_add.device = Vec2f(event.button.x * x_basis_scale, (window.height - event.button.y) * y_basis_scale);
            list_of_user_polys[active_user_poly].push_back(to_add);
        }
        else if (event.type == SDL_EVENT_KEY_DOWN)
        {
            active_user_poly++;
            list_of_user_polys.push_back(std::vector<Vertex>());
        }
    }
}

void draw()
{
    clear_buffer(Vec3f(0.0f, 0.0f, 0.0f), screen_res_buffer);
    clear_buffer(Vec3f(0.0f, 0.0f, 0.0f), low_res_buffer);

    Vec3f POINT_COLOR (0.0f, 1.0f, 0.0f);
    Vec3f LINE_COLOR  (1.0f, 0.0f, 0.0f);

    for (int i = 0; i < list_of_user_polys.size(); i++)
    {
        std::vector<Vertex>& poly = list_of_user_polys[i];

        if (poly.size() > 2)
        {
            rasterize_polygon(poly, low_res_buffer);
        }

        if (poly.size() > 1)
        {
            for (int j = 0; j < poly.size(); j++)
            {
                Vertex start = poly[j];
                Vertex end = poly[(j + 1) % poly.size()];

                start.color = LINE_COLOR;
                end.color = LINE_COLOR;

                rasterize_line(start, end, 2, low_res_buffer);
            }
        }

        for (int j = 0; j < poly.size(); j++)
        {
            Vertex v = poly[j];
            v.color = POINT_COLOR;
            int POINT_SIZE = 4;
            
            rasterize_point(v, POINT_SIZE, low_res_buffer);
        }
    }

    blit_buffer(low_res_buffer, screen_res_buffer);
    blit_window(screen_res_buffer->pixels);
}