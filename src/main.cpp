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
#include "Camera.h"

struct Actions
{
    bool cycle_resolution = false;
    bool update_mouse_pos = false;
    bool exit_program = false;
    bool resize_window = false;
} input_actions;

struct ProgramState
{
    bool running = true;

    std::chrono::_V2::system_clock::time_point last_frame_start = std::chrono::high_resolution_clock::now();
    float dt = 0;

    Buffer* render_buffer = nullptr;
    Buffer* screen_res_buffer = nullptr;
    Vec2f mouse_pos;
    Model* model = nullptr;
    Camera camera;

    int resolution_scale_index = 3;
} state;

const int RESOLUTION_SCALERS_COUNT = 6;
const float RESOLUTION_SCALERS[RESOLUTION_SCALERS_COUNT] = { 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };

void update();
void draw();
void handle_events();
void handle_time();
void init();

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
    state.render_buffer = new Buffer();

    int width = 640, height = 480;
    init_window(width, height);
    resize_buffer(state.screen_res_buffer, width, height);
    resize_buffer(state.render_buffer, width, height);

    state.model = new Model("obj/square.obj");

    state.camera.up = Vec3f(0.0f, 1.0f, 0.0f);
    state.camera.look_at = Vec3f(0.0f, 0.0f, 0.0f);
    state.camera.pos = Vec3f(0.0f, 0.0f, 5.0f);
    state.camera.aspect_ratio = ((float) width) / ((float) height);
    state.camera.near = 1.0f;
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

    input_actions.cycle_resolution = (window.input.keys[KEY_ENTER].is_down && !window.input.keys[KEY_ENTER].prev_state);
    input_actions.update_mouse_pos = (window.input.mouse.did_move);
    input_actions.exit_program = (window.input.quit);
    input_actions.resize_window = (window.input.window.did_resize);
}

void render_scene(Buffer* frame_buffer)
{
    Vec3f scale_factor (frame_buffer->width/state.camera.aspect_ratio/2.0f, frame_buffer->height/2.0f, 0.0f);
    Mat4x4f local_to_world = Mat4x4f::scale(Vec3f(5.0f)) * Mat4x4f::rotation_y(SDL_GetTicks() * 0.001);
    Mat4x4f world_to_camera = Mat4x4f::look_at(state.camera.pos, state.camera.look_at, state.camera.up);
    Mat4x4f projected_to_device = Mat4x4f::translation(Vec3f(frame_buffer->width/2.0f, frame_buffer->height/2.0f, 0.0f)) * Mat4x4f::scale(scale_factor);

    std::vector<Vertex> polygon;
    Vec3f face_color (1.0f, 0.0f, 0.0f);
    std::vector<int> face = state.model->get_face(0); // 1 face
    for (int i = 0; i < face.size(); i++)
    {
        Vec3f local_pos = state.model->get_local_position(face[i]);
        Vec3f world_pos = local_to_world * Vec4f(local_pos, 1.0f);
        Vec3f view_pos  = world_to_camera * Vec4f(world_pos, 1.0f);
        Vec3f projected_pos = Vec3f((view_pos.x / fabs(view_pos.z)) * state.camera.near, (view_pos.y / fabs(view_pos.z)) * state.camera.near, view_pos.z);
        Vec3f device_pos = projected_to_device * Vec4f(projected_pos, 1.0f);

        Vertex vert;
        vert.device = Vec2f(device_pos.x, device_pos.y);
        vert.depth = device_pos.z;
        vert.color = face_color;

        polygon.push_back(vert);
    }

    rasterize_polygon(polygon, frame_buffer);
}

void draw()
{
    Vec3f CLEAR_COLOR (0.0f, 0.0f, 0.0f);

    if ((state.render_buffer->width == state.screen_res_buffer->width) && (state.render_buffer->height == state.screen_res_buffer->height))
    {
        clear_buffer(CLEAR_COLOR, state.screen_res_buffer);
        render_scene(state.screen_res_buffer);
    }
    else
    {
        // NOTE: a high res render buffer actually won't make image more aliased due sampling method being closest 
        //          and not something like bi-linear blend, that is something to be done soon
        clear_buffer(CLEAR_COLOR, state.screen_res_buffer);
        clear_buffer(CLEAR_COLOR, state.render_buffer);
        render_scene(state.render_buffer);
        blit_buffer(state.render_buffer, state.screen_res_buffer);
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
        resize_buffer(state.render_buffer, window.input.window.new_width * RESOLUTION_SCALERS[state.resolution_scale_index], window.input.window.new_height * RESOLUTION_SCALERS[state.resolution_scale_index]);
        state.camera.aspect_ratio = ((float) window.input.window.new_width) / ((float) window.input.window.new_height);
    }

    if (input_actions.update_mouse_pos)
    {
        float x_basis_scale = ((float) state.render_buffer->width)  / state.screen_res_buffer->width;
        float y_basis_scale = ((float) state.render_buffer->height) / state.screen_res_buffer->height;
        float y_pos_flipped = (window.height - window.input.mouse.pos.y);
        state.mouse_pos.x = window.input.mouse.pos.x * x_basis_scale;
        state.mouse_pos.y = y_pos_flipped * y_basis_scale;
    }

    if (input_actions.cycle_resolution)
    {
        state.resolution_scale_index = (state.resolution_scale_index + 1) % RESOLUTION_SCALERS_COUNT;
        resize_buffer(state.render_buffer, state.screen_res_buffer->width * RESOLUTION_SCALERS[state.resolution_scale_index], state.screen_res_buffer->height * RESOLUTION_SCALERS[state.resolution_scale_index]);
    }
}