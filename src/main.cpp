#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <vector>
#include "Window.h"
#include "Vec.h"
#include "Rasterize.h"
#include "Model.h"
#include "Mat.h"

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
    Vec2f mouse_pos;
    std::vector<std::vector<Vertex>> list_of_user_polys = std::vector<std::vector<Vertex>>(1);
    float theta = 0.0f;

    Model* model = nullptr;
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

    state.model = new Model("obj/square.obj");
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
    Mat4x4f local_to_world = Mat4x4f::scale(Vec3f(5.0f));
    state.theta += state.dt;

    Vec3f camera_pos (3.0f * sin(state.theta * 0.001f), 0.0f, 3.0f * cos(state.theta * 0.001f));
    Vec3f camera_look_at (0.0f);
    Vec3f camera_up (0.0f, 1.0f, 0.0f);
    Mat4x4f world_to_camera = Mat4x4f::look_at(camera_pos, camera_look_at, camera_up);

    struct VirtualScreen
    {
        float width;
        float height;
        float near = 5.0f; // assume 1 for now
    } virtual_screen;

    // Should be integer math?
    // Only integer size virtual screen?
    float aspect_ratio = state.low_res_buffer->width / state.low_res_buffer->height;
    virtual_screen.height = 8.0f;
    virtual_screen.width = virtual_screen.height * aspect_ratio;

    // Assumes camera is orthonormal basis
    Vec3f scale_factor (state.low_res_buffer->width/virtual_screen.width/2.0f, state.low_res_buffer->height/virtual_screen.height/2.0f, 0.0f);
    Mat4x4f camera_to_device = Mat4x4f::translation(Vec3f(state.low_res_buffer->width/2.0f, state.low_res_buffer->height/2.0f, 0.0f)) * Mat4x4f::scale(scale_factor);
    Vec3f face_color (1.0f, 0.0f, 0.0f);
    std::vector<Vertex> polygon;

    std::vector<int> face = state.model->get_face(0); // 1 face
    for (int i = 0; i < face.size(); i++)
    {
        Vec3f local_pos = state.model->get_local_position(face[i]);
        Vec3f world_pos = local_to_world * Vec4f(local_pos, 1.0f);
        Vec3f view_pos = world_to_camera * Vec4f(world_pos, 1.0f);
        Vec3f projected_pos = Vec3f(view_pos.x / fabs(view_pos.z), view_pos.y / fabs(view_pos.z), view_pos.z);
        Vec3f device_pos = camera_to_device * Vec4f(projected_pos, 1.0f);

        Vertex vert;
        vert.device = Vec2f(device_pos.x, device_pos.y);
        vert.depth = device_pos.z;
        vert.color = face_color;

        polygon.push_back(vert);
    }

    rasterize_polygon(polygon, frame_buffer);

    // Vec3f POINT_COLOR (0.0f, 1.0f, 0.0f);
    // Vec3f LINE_COLOR  (1.0f, 0.0f, 0.0f);

    // for (int i = 0; i < state.list_of_user_polys.size(); i++)
    // {
    //     std::vector<Vertex>& poly = state.list_of_user_polys[i];

    //     if (poly.size() > 2)
    //     {
    //         rasterize_polygon(poly, frame_buffer);
    //     }

    //     if (poly.size() > 1)
    //     {
    //         for (int j = 0; j < poly.size(); j++)
    //         {
    //             const Vertex& start = poly[j];
    //             const Vertex& end = poly[(j + 1) % poly.size()];
    //             rasterize_line(start, end, 2, frame_buffer);
    //         }
    //     }

    //     for (int j = 0; j < poly.size(); j++)
    //     {
    //         Vertex v = poly[j];
    //         int POINT_SIZE = 4;
            
    //         rasterize_point(v, POINT_SIZE, frame_buffer);
    //     }
    // }
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
        state.mouse_pos.x = window.input.mouse.pos.x * x_basis_scale;
        state.mouse_pos.y = y_pos_flipped * y_basis_scale;
    }

    if (input_actions.add_vertex)
    {
        Vertex vertex;
        vertex.color = rnd_color();
        vertex.device = Vec2f(state.mouse_pos.x, state.mouse_pos.y);
        state.list_of_user_polys[state.active_user_poly].push_back(vertex);
    }

    if (input_actions.select_vertex)
    {
        // Find closest vertex of active polygon

        int closest = 0; // for now I'll assume atleast 1 vertex
        Vec2f closest_pos = state.list_of_user_polys[state.active_user_poly][closest].device;
        float closest_distance = sqrt((closest_pos.x - state.mouse_pos.x)*(closest_pos.x - state.mouse_pos.x) + (closest_pos.y - state.mouse_pos.y)*(closest_pos.y - state.mouse_pos.y));
        for (int i = 1; i < state.list_of_user_polys[state.active_user_poly].size(); i++)
        {
            Vec2f current_pos = state.list_of_user_polys[state.active_user_poly][i].device;
            float current_distance = sqrt((current_pos.x - state.mouse_pos.x)*(current_pos.x - state.mouse_pos.x) + (current_pos.y - state.mouse_pos.y)*(current_pos.y - state.mouse_pos.y));

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
        vertex.device = state.mouse_pos;
    }
}


Vec3f rnd_color()
{
    static int i = -1;
    i++;

    return colors[i % color_count];
}