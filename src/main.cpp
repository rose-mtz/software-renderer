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

Vec2f mouse_pos;
bool running = true;

bool update_active_vertex_color = false;
bool drag_active_vertex = false;
int active_vertex = -1;
int active_user_poly = 0;
std::vector<std::vector<Vertex>> list_of_user_polys (1);

auto last_frame_start = std::chrono::high_resolution_clock::now();
float dt = 0;

int active_color = 0;
int color_count = 5;
const Vec3f colors[] = {Vec3f(1.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f), Vec3f(0.0f, 0.0f, 1.0f), Vec3f(1.0f, 1.0f, 1.0f), Vec3f(0.0f, 0.0f, 0.0f) };

void update();
void draw();
void handle_events();
void handle_time();
void init();

Vec3f rnd_color();

int main()
{
    init();

    while (running)
    {
        handle_time();
        handle_events();
        update();
        draw();
    }
}

void init()
{
    screen_res_buffer = new Buffer();
    low_res_buffer = new Buffer();

    init_window(640, 480);    
    resize_buffer(screen_res_buffer, 640, 480);
    resize_buffer(low_res_buffer, 640, 480);
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
            float x_basis_scale = ((float) low_res_buffer->width)  / screen_res_buffer->width;
            float y_basis_scale = ((float) low_res_buffer->height) / screen_res_buffer->height;
            mouse_pos.x = event.motion.x * x_basis_scale;
            mouse_pos.y = (window.height - event.motion.y) * y_basis_scale;
        } 
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            if (event.button.button == SDL_BUTTON_LEFT) // Add a vertex to active polygon
            {
                Vertex vertex;
                vertex.color = rnd_color();
                vertex.device = Vec2f(mouse_pos.x, mouse_pos.y);
                list_of_user_polys[active_user_poly].push_back(vertex);
            }
            else if (event.button.button == SDL_BUTTON_RIGHT) // Drag vertex
            {
                // Find closest vertex of active polygon

                int closest = 0; // for now I'll assume atleast 1 vertex
                Vec2f closest_pos = list_of_user_polys[active_user_poly][closest].device;
                float closest_distance = sqrt((closest_pos.x - mouse_pos.x)*(closest_pos.x - mouse_pos.x) + (closest_pos.y - mouse_pos.y)*(closest_pos.y - mouse_pos.y));
                for (int i = 1; i < list_of_user_polys[active_user_poly].size(); i++)
                {
                    Vec2f current_pos = list_of_user_polys[active_user_poly][i].device;
                    float current_distance = sqrt((current_pos.x - mouse_pos.x)*(current_pos.x - mouse_pos.x) + (current_pos.y - mouse_pos.y)*(current_pos.y - mouse_pos.y));

                    if (current_distance < closest_distance)
                    {
                        closest = i;
                        closest_distance = current_distance;
                    }
                }

                active_vertex = closest;
                drag_active_vertex = true;
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT)
            {
                drag_active_vertex = false;
            }
        }
        else if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_SPACE)
            {
                active_user_poly++;
                list_of_user_polys.push_back(std::vector<Vertex>());
            }
            else if (event.key.key == SDLK_RETURN)
            {
                update_active_vertex_color = true;
            }
        }
    }
}

void draw() // TODO: pass in buffer to render into so that I can skip low res buffer if its size is equal to screen buffer
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
            // v.color = POINT_COLOR;
            int POINT_SIZE = 4;
            
            rasterize_point(v, POINT_SIZE, low_res_buffer);
        }
    }

    blit_buffer(low_res_buffer, screen_res_buffer);
    blit_window(screen_res_buffer->pixels);
}


void update()
{
    if (drag_active_vertex)
    {
        Vertex& vertex = list_of_user_polys[active_user_poly][active_vertex];
        vertex.device = mouse_pos;
    }

    if (update_active_vertex_color && active_vertex != -1)
    {
        Vertex& vertex = list_of_user_polys[active_user_poly][active_vertex];
        vertex.color = colors[active_color];

        active_color = (active_color + 1) % color_count;
        update_active_vertex_color = false;
    }
}


Vec3f rnd_color()
{
    static int i = -1;
    i++;

    return colors[i % color_count];
}