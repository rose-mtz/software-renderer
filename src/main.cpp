#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <vector>
#include "Window.h"
#include "Vec.h"
#include "Rasterize.h"

struct Actions
{
    bool create_new_polygon = false;
    bool add_vertex = false;
    bool change_active_vertex_color = false;
    bool update_mouse_pos = false;
    bool exit_program = false;
    bool select_vertex = false;
    bool stop_dragging_selected_vertex = false;
    bool resize_window = false;
    bool start_drag_active_vertex = false;
} input_actions;

struct ProgramState
{
    bool is_dragging_vertex;
    int active_vertex = -1;
    int active_user_poly = 0;
    int active_color = 0;
    bool running = true;
    std::chrono::_V2::system_clock::time_point last_frame_start = std::chrono::high_resolution_clock::now();
    float dt = 0;
    Buffer* low_res_buffer = nullptr;
    Buffer* screen_res_buffer = nullptr;
    Vec2f mouse_pos_screen_adj;
    std::vector<std::vector<Vertex>> list_of_user_polys = std::vector<std::vector<Vertex>>(1);
} state;


const int color_count = 5;
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

    while (state.running)
    {
        handle_time();
        handle_events();
        update();
        draw();
    }
}

void init()
{
    state.screen_res_buffer = new Buffer();
    state.low_res_buffer = new Buffer();

    init_window(640, 480);    
    resize_buffer(state.screen_res_buffer, 640, 480);
    resize_buffer(state.low_res_buffer, 160, 120);
}

void handle_time()
{
    auto current_frame_start = std::chrono::high_resolution_clock::now();
    state.dt = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(current_frame_start - state.last_frame_start).count();
    state.last_frame_start = current_frame_start;
    std::cout << state.dt << std::endl;
}

void handle_events()
{
    poll_events();

    input_actions.create_new_polygon = (window.input.keys[KEY_SPACE].is_down && !window.input.keys[KEY_SPACE].prev_state);
    input_actions.add_vertex = (window.input.mouse.left.is_down && !window.input.mouse.left.prev_state);
    input_actions.change_active_vertex_color = (window.input.keys[KEY_ENTER].is_down && !window.input.keys[KEY_ENTER].prev_state);
    input_actions.update_mouse_pos = (window.input.mouse.did_move);
    input_actions.exit_program = (window.input.quit);
    input_actions.select_vertex = (window.input.mouse.right.is_down && !window.input.mouse.right.prev_state);
    input_actions.resize_window = (window.input.window.did_resize);
    input_actions.start_drag_active_vertex = (window.input.mouse.right.is_down && !window.input.mouse.right.prev_state);
    input_actions.stop_dragging_selected_vertex = (!window.input.mouse.right.is_down && window.input.mouse.right.prev_state);
}

void render_scene(Buffer* frame_buffer)
{
    Vec3f POINT_COLOR (0.0f, 1.0f, 0.0f);
    Vec3f LINE_COLOR  (1.0f, 0.0f, 0.0f);

    for (int i = 0; i < state.list_of_user_polys.size(); i++)
    {
        std::vector<Vertex>& poly = state.list_of_user_polys[i];

        if (poly.size() > 2)
        {
            rasterize_polygon(poly, frame_buffer);
        }

        if (poly.size() > 1)
        {
            for (int j = 0; j < poly.size(); j++)
            {
                const Vertex& start = poly[j];
                const Vertex& end = poly[(j + 1) % poly.size()];
                rasterize_line(start, end, 2, frame_buffer);
            }
        }

        for (int j = 0; j < poly.size(); j++)
        {
            Vertex v = poly[j];
            int POINT_SIZE = 4;
            
            rasterize_point(v, POINT_SIZE, frame_buffer);
        }
    }
}

void draw()
{
    Vec3f CLEAR_COLOR (0.0f, 0.0f, 0.0f);

    if ((state.low_res_buffer->width == state.screen_res_buffer->width) && (state.low_res_buffer->height == state.screen_res_buffer->height))
    {
        clear_buffer(CLEAR_COLOR, state.screen_res_buffer);
        render_scene(state.screen_res_buffer);
    }
    else
    {
        clear_buffer(CLEAR_COLOR, state.screen_res_buffer);
        clear_buffer(CLEAR_COLOR, state.low_res_buffer);
        render_scene(state.low_res_buffer);
        blit_buffer(state.low_res_buffer, state.screen_res_buffer);
    }

    blit_window(state.screen_res_buffer->pixels);
}

void update()
{
    if (input_actions.exit_program)
    {
        state.running = false; // TODO: maybe have a exit() function
    }

    if (input_actions.resize_window)
    {
        resize_window(window.input.window.new_width, window.input.window.new_height);
        resize_buffer(state.screen_res_buffer, window.input.window.new_width, window.input.window.new_height);
    }

    if (input_actions.update_mouse_pos)
    {
        float x_basis_scale = ((float) state.low_res_buffer->width)  / state.screen_res_buffer->width;
        float y_basis_scale = ((float) state.low_res_buffer->height) / state.screen_res_buffer->height;
        float y_pos_flipped = (window.height - window.input.mouse.pos.y);
        state.mouse_pos_screen_adj.x = window.input.mouse.pos.x * x_basis_scale;
        state.mouse_pos_screen_adj.y = y_pos_flipped * y_basis_scale;
    }

    if (input_actions.add_vertex)
    {
        Vertex vertex;
        vertex.color = rnd_color();
        vertex.device = Vec2f(state.mouse_pos_screen_adj.x, state.mouse_pos_screen_adj.y);
        state.list_of_user_polys[state.active_user_poly].push_back(vertex);
    }

    if (input_actions.select_vertex)
    {
        // Find closest vertex of active polygon

        int closest = 0; // for now I'll assume atleast 1 vertex
        Vec2f closest_pos = state.list_of_user_polys[state.active_user_poly][closest].device;
        float closest_distance = sqrt((closest_pos.x - state.mouse_pos_screen_adj.x)*(closest_pos.x - state.mouse_pos_screen_adj.x) + (closest_pos.y - state.mouse_pos_screen_adj.y)*(closest_pos.y - state.mouse_pos_screen_adj.y));
        for (int i = 1; i < state.list_of_user_polys[state.active_user_poly].size(); i++)
        {
            Vec2f current_pos = state.list_of_user_polys[state.active_user_poly][i].device;
            float current_distance = sqrt((current_pos.x - state.mouse_pos_screen_adj.x)*(current_pos.x - state.mouse_pos_screen_adj.x) + (current_pos.y - state.mouse_pos_screen_adj.y)*(current_pos.y - state.mouse_pos_screen_adj.y));

            if (current_distance < closest_distance)
            {
                closest = i;
                closest_distance = current_distance;
            }
        }

        state.active_vertex = closest;
    }

    if (input_actions.start_drag_active_vertex)
    {
        state.is_dragging_vertex = true;
    }

    if (input_actions.stop_dragging_selected_vertex)
    {
        state.is_dragging_vertex = false;
    }

    if (input_actions.create_new_polygon)
    {
        state.active_user_poly++;
        state.list_of_user_polys.push_back(std::vector<Vertex>());
    }

    if (input_actions.change_active_vertex_color && state.active_vertex != -1)
    {
        Vertex& vertex = state.list_of_user_polys[state.active_user_poly][state.active_vertex];
        vertex.color = colors[state.active_color];

        state.active_color = (state.active_color + 1) % color_count;
    }

    // Persistent user actions?

    if (state.is_dragging_vertex)
    {
        Vertex& vertex = state.list_of_user_polys[state.active_user_poly][state.active_vertex];
        vertex.device = state.mouse_pos_screen_adj;
    }
}


Vec3f rnd_color()
{
    static int i = -1;
    i++;

    return colors[i % color_count];
}