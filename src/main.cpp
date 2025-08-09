#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include <cmath>
#include <vector>
#include "Window.h"
#include "Vec.h"
#include "Rasterize.h"
#include "Mesh.h"
#include "Mat.h"
#include "Camera.h"
#include "Object.h"
#include "Geometry.h"

struct Actions
{
    bool cycle_resolution = false;
    bool update_mouse_pos = false;
    bool exit_program = false;
    bool resize_window = false;
    bool change_camera_orientation = false;
    bool move_camera_forward = false;
    bool move_camera_back = false;
    bool move_camera_left = false;
    bool move_camera_right = false;
} input_actions;

struct ProgramState
{
    bool running = true;

    std::chrono::_V2::system_clock::time_point last_frame_start = std::chrono::high_resolution_clock::now();
    float dt = 0;

    Buffer* render_buffer = nullptr;
    Buffer* screen_res_buffer = nullptr;
    int resolution_scale_index = 3;

    Vec2f mouse_pos;
    Mesh* square = nullptr;
    Camera camera;
    std::vector<Object> objects;
} state;

const int RESOLUTION_SCALERS_COUNT = 6;
const float RESOLUTION_SCALERS[RESOLUTION_SCALERS_COUNT] = { 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
const Vec3f CLEAR_COLOR (0.0f, 0.0f, 0.0f);

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
        // handle_time();
        handle_events();
        update();
        draw();
    }
}

void init()
{
    int width = 640, height = 480;
    init_window(width, height);

    state.screen_res_buffer = new Buffer();
    resize_buffer(state.screen_res_buffer, width, height);
    state.render_buffer = new Buffer();
    resize_buffer(state.render_buffer, width, height);

    state.camera.up = Vec3f(0.0f, 1.0f, 0.0f);
    state.camera.pos = Vec3f(0.0f, 0.0f, 5.0f);
    state.camera.aspect_ratio = ((float) width) / ((float) height);
    state.camera.near = 1.0f;
    state.camera.far = 25.0f;
    state.camera.dir = Vec3f(0.0f, 0.0f, -1.0f);
    state.camera.yaw = radians(180.0f);
    state.camera.pitch = radians(90.0f);

    // TEMPORARY: cube ----------------------------------------------------------------------

    Object top_face, front_face, bottom_face, back_face, left_face, right_face;
    top_face.world_pos = Vec3f(0.0f, 0.5f, 0.0f);
    top_face.orientation = Vec3f(radians(-90), 0.0f, 0.0f);
    front_face.world_pos = Vec3f(0.0f, 0.0f, 0.5f);
    back_face.world_pos = Vec3f(0.0f, 0.0f, -0.5f);
    bottom_face.world_pos = Vec3f(0.0f, -0.5f, 0.0f);
    bottom_face.orientation = Vec3f(radians(90), 0.0f, 0.0f);
    left_face.world_pos = Vec3f(-0.5f, 0.0f, 0.0f);
    left_face.orientation = Vec3f(0.0f, radians(-90), 0.0f);
    right_face.world_pos = Vec3f(0.5f, 0.0f, 0.0f);
    right_face.orientation = Vec3f(0.0f, radians(90), 0.0f);

    state.square = new Mesh("obj/square.obj");
    top_face.mesh = state.square;
    bottom_face.mesh = state.square;
    left_face.mesh = state.square;
    right_face.mesh = state.square;
    front_face.mesh = state.square;
    back_face.mesh = state.square;

    top_face.color    = Vec3f(1.0f, 0.0f, 0.0f);
    bottom_face.color = Vec3f(0.0f, 1.0f, 0.0f);
    left_face.color   = Vec3f(0.0f, 0.0f, 1.0f);
    right_face.color  = Vec3f(1.0f, 1.0f, 1.0f);
    front_face.color  = Vec3f(1.0f, 1.0f, 0.0f);
    back_face.color   = Vec3f(1.0f, 0.0f, 1.0f);

    state.objects.push_back(top_face);
    state.objects.push_back(bottom_face);
    state.objects.push_back(left_face);
    state.objects.push_back(right_face);
    state.objects.push_back(front_face);
    state.objects.push_back(back_face);
}

void handle_time()
{
    // QUESTION: is this in wall clock or CPU ticks (time program is running, doesn't include blocked IO time)
    // COMMENT: i do not understand this time code

    auto current_frame_start = std::chrono::high_resolution_clock::now();
    state.dt = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(current_frame_start - state.last_frame_start).count();
    state.last_frame_start = current_frame_start;
    std::cout << state.dt << std::endl;
}

void handle_events()
{
    poll_events();

    // Map user input to commands
    input_actions.cycle_resolution = (window.input.keys[KEY_ENTER].is_down && !window.input.keys[KEY_ENTER].prev_state);
    input_actions.update_mouse_pos = (window.input.mouse.did_move);
    input_actions.exit_program = (window.input.quit);
    input_actions.resize_window = (window.input.window.did_resize);
    input_actions.change_camera_orientation = (window.input.mouse.did_move && window.input.mouse.left.is_down);
    input_actions.move_camera_forward = window.input.keys[KEY_UP].is_down;
    input_actions.move_camera_back = window.input.keys[KEY_DOWN].is_down;
    input_actions.move_camera_left = window.input.keys[KEY_LEFT].is_down;
    input_actions.move_camera_right = window.input.keys[KEY_RIGHT].is_down;
}

void render_scene(Buffer* frame_buffer)
{
    Mat4x4f camera = Mat4x4f::look_at(state.camera.pos, state.camera.dir + state.camera.pos, state.camera.up);
    Mat4x4f device = Mat4x4f::translation(Vec3f(frame_buffer->width/2.0f, frame_buffer->height/2.0f, 0.0f)) * Mat4x4f::scale(Vec3f(frame_buffer->width/state.camera.aspect_ratio, frame_buffer->height, 1.0f)); // NOTE: virtual screen height is 1
    Mat4x4f world  = Mat4x4f::scale(Vec3f(1.0f));

    for (int o = 0; o < state.objects.size(); o++)
    {
        Object& obj   = state.objects[o];
        Mat4x4f local = Mat4x4f::translation(obj.world_pos) * Mat4x4f::rotation_x(obj.orientation.x) * Mat4x4f::rotation_y(obj.orientation.y) * Mat4x4f::rotation_z(obj.orientation.z);

        for (int f = 0; f < obj.mesh->get_face_count(); f++)
        {
            std::vector<int> face = obj.mesh->get_face(f); // TEMPORARY: face just has indicies to vertices' local positions
            std::vector<Vertex> vertices;
            for (int v = 0; v < face.size(); v++)
            {
                Vec3f local_pos  = obj.mesh->get_local_position(face[v]);
                Vec3f world_pos  = world * local * Vec4f(local_pos, 1.0f);
                Vec3f view_pos   = camera * Vec4f(world_pos, 1.0f);

                Vertex vertex;
                vertex.color = obj.color; // TEMPORARY
                vertex.world = world_pos;
                vertex.view = view_pos;
                vertex.cull = view_pos;

                vertices.push_back(vertex);
            }

            std::vector<Plane> frustum_planes = get_frustum_planes(get_frustum(state.camera));
            for (int p = 0; p < frustum_planes.size(); p++)
            {
                std::vector<Vertex> in, out;
                cull_polygon(vertices, frustum_planes[p], in, out);
                vertices = in;
            }

            for (int v = 0; v < vertices.size(); v++)
            {
                Vertex& vertex = vertices[v];

                Vec3f projected_pos = Vec3f((vertex.view.x / fabs(vertex.view.z)) * state.camera.near, (vertex.view.y / fabs(vertex.view.z)) * state.camera.near, vertex.view.z);
                Vec3f device_pos = device * Vec4f(projected_pos, 1.0f);

                vertex.device = device_pos.xy();
                vertex.depth = device_pos.z;
                vertex.cull = Vec3f(vertex.device.x, vertex.device.y, 0.0f);
            }

            rasterize_polygon(vertices, frame_buffer);

            // for (int e = 0; e < vertices.size(); e++)
            // {
            //     Vertex v0 = vertices[e];
            //     Vertex v1 = vertices[(e + 1) % vertices.size()];

            //     v0.color = Vec3f(0.0f);
            //     v1.color = Vec3f(0.0f);

            //     rasterize_line(v0, v1, 5, frame_buffer);
            // }

            // for (int p = 0; p < vertices.size(); p++)
            // {
            //     Vertex point = vertices[p];
            //     point.color = Vec3f(1.0f, 1.0f, 1.0f);

            //     rasterize_point(point, 8, frame_buffer);
            // }
        }
    }
}

void draw()
{
    // if ((state.render_buffer->width == state.screen_res_buffer->width) && (state.render_buffer->height == state.screen_res_buffer->height))
    // {
    //     clear_buffer(CLEAR_COLOR, state.screen_res_buffer);
    //     render_scene(state.screen_res_buffer);
    // }
    // else
    // {
    //     clear_buffer(CLEAR_COLOR, state.render_buffer);
    //     render_scene(state.render_buffer);
    //     blit_buffer(state.render_buffer, state.screen_res_buffer);
    // }

    clear_buffer(CLEAR_COLOR, state.render_buffer);
    render_scene(state.render_buffer);
    clear_buffer(Vec3f(0.5f, 0.20f, 0.0f), state.screen_res_buffer);
    blit_buffer(state.render_buffer, state.screen_res_buffer, Vec2f(state.screen_res_buffer->width * 0.1f, state.screen_res_buffer->height * 0.1f), 0.8f, 0.8f);
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
        state.mouse_pos = map_point(Vec2f(window.input.mouse.pos.x, window.height - window.input.mouse.pos.y), state.screen_res_buffer, state.render_buffer);
    }

    if (input_actions.cycle_resolution)
    {
        state.resolution_scale_index = (state.resolution_scale_index + 1) % RESOLUTION_SCALERS_COUNT;
        resize_buffer(state.render_buffer, state.screen_res_buffer->width * RESOLUTION_SCALERS[state.resolution_scale_index], state.screen_res_buffer->height * RESOLUTION_SCALERS[state.resolution_scale_index]);
    }

    if (input_actions.change_camera_orientation) // only pitch and yaw
    {
        float dx = window.input.mouse.delta.x;
        float dy = window.input.mouse.delta.y;
        float sensitivity_y = 0.0025f, sensitivity_x = 0.0025f;

        float MIN_PITCH = radians(10.0f);
        float MAX_PITCH = radians(170.0f);

        state.camera.yaw += dx * sensitivity_x;
        state.camera.pitch += (-dy) * sensitivity_y;
        state.camera.pitch = state.camera.pitch > MAX_PITCH ? MAX_PITCH : state.camera.pitch < MIN_PITCH ? MIN_PITCH : state.camera.pitch;

        Vec3f look_at;
        look_at.x = sin(state.camera.pitch) * sin(state.camera.yaw);
        look_at.y = cos(state.camera.pitch);
        look_at.z = sin(state.camera.pitch) * cos(state.camera.yaw);

        state.camera.dir = look_at;
    }

    float movement_sensitivity = 0.01f;
    if (input_actions.move_camera_forward)
    {
        Mat3x3f camera_inv = Mat4x4f::look_at(state.camera.pos, state.camera.dir + state.camera.pos, state.camera.up).truncated().transposed();
        state.camera.pos = state.camera.pos + (camera_inv * Vec3f(0.0f, 0.0f, -movement_sensitivity));
    }
    if (input_actions.move_camera_back)
    {
        Mat3x3f camera_inv = Mat4x4f::look_at(state.camera.pos, state.camera.dir + state.camera.pos, state.camera.up).truncated().transposed();
        state.camera.pos = state.camera.pos + (camera_inv * Vec3f(0.0f, 0.0f, movement_sensitivity));
    }
    if (input_actions.move_camera_left)
    {
        Mat3x3f camera_inv = Mat4x4f::look_at(state.camera.pos, state.camera.dir + state.camera.pos, state.camera.up).truncated().transposed();
        state.camera.pos = state.camera.pos + (camera_inv * Vec3f(-movement_sensitivity, 0.0f, 0.0f));
    }
    if (input_actions.move_camera_right)
    {
        Mat3x3f camera_inv = Mat4x4f::look_at(state.camera.pos, state.camera.dir + state.camera.pos, state.camera.up).truncated().transposed();
        state.camera.pos = state.camera.pos + (camera_inv * Vec3f(movement_sensitivity, 0.0f, 0.0f));
    }
}