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
#include "Buffer.h"
#include "Util.h"

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

struct FrameBuffer
{
    Buffer* color;
    Buffer* depth;
    int width, height;
};

struct ProgramState
{
    bool running = true;

    std::chrono::_V2::system_clock::time_point last_frame_start = std::chrono::high_resolution_clock::now();
    float dt = 0;

    FrameBuffer* render_buffer = nullptr;
    FrameBuffer* screen_res_buffer = nullptr;
    int resolution_scale_index = 3;

    Vec2f mouse_pos;
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

Buffer* tga_image_to_buffer(TGAImage& img);

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

    state.screen_res_buffer = new FrameBuffer();
    state.render_buffer = new FrameBuffer();
    state.render_buffer->color = new Buffer();
    state.render_buffer->depth = new Buffer();
    state.screen_res_buffer->color = new Buffer();
    state.screen_res_buffer->depth = new Buffer();
    state.screen_res_buffer->width = width;
    state.screen_res_buffer->height = height;
    state.render_buffer->width = width;
    state.render_buffer->height = height;
    init_buffer(width, height, 3, state.render_buffer->color);
    init_buffer(width, height, 1, state.render_buffer->depth);
    init_buffer(width, height, 3, state.screen_res_buffer->color);
    init_buffer(width, height, 1, state.screen_res_buffer->depth);

    state.camera.up = Vec3f(0.0f, 1.0f, 0.0f);
    state.camera.pos = Vec3f(0.0f, 0.0f, 5.0f);
    state.camera.aspect_ratio = ((float) width) / ((float) height);
    state.camera.near = 1.0f;
    state.camera.far = 25.0f;
    state.camera.dir = Vec3f(0.0f, 0.0f, -1.0f);
    state.camera.yaw = radians(180.0f);
    state.camera.pitch = radians(90.0f);

    TGAImage tga_image;
    tga_image.read_tga_file("img/Cubie_Face_Red.tga");

    Object cube;
    cube.translation = Vec3f(0.0f, 0.0f, 0.0f);
    cube.yaw = radians(90.0f);
    cube.pitch = radians(15.0f);
    cube.roll = radians(-90.0f);
    cube.scale = Vec3f(2.0f, 1.0f, 1.0f);
    cube.mesh = new Mesh("obj/cube.obj");
    cube.texture = tga_image_to_buffer(tga_image);
    state.objects.push_back(cube);
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

void render_scene(FrameBuffer* frame_buffer)
{
    Mat4x4f camera = Mat4x4f::look_at(state.camera.pos, state.camera.dir, state.camera.up);
    Mat4x4f device = Mat4x4f::translation(Vec3f(frame_buffer->width/2.0f, frame_buffer->height/2.0f, 0.0f)) * Mat4x4f::scale(Vec3f(frame_buffer->width/state.camera.aspect_ratio, frame_buffer->height, 1.0f)); // ASSUMPTION: virtual screen height is 1, and width is aspect-ratio
    Mat4x4f world  = Mat4x4f::identity_matrix();

    for (int o = 0; o < state.objects.size(); o++)
    {
        Object& obj   = state.objects[o];
        Mat4x4f local = Mat4x4f::translation(obj.translation) * Mat4x4f::rotation_y(obj.yaw) * Mat4x4f::rotation_x(obj.pitch) * Mat4x4f::rotation_z(obj.roll) * Mat4x4f::scale(obj.scale);

        for (int f = 0; f < obj.mesh->faces.size(); f++)
        {
            std::vector<int> face = obj.mesh->faces[f];
            std::vector<Vertex> vertices;
            int vertex_count = face.size() / 2; // ASSUMPTION: 2 attributes per vertex (local pos, uv)
            for (int v = 0; v < vertex_count; v++)
            {
                Vec3f local_pos = obj.mesh->vertices[face[v * 2]];
                Vec2f uv = obj.mesh->uvs[face[v * 2 + 1]];

                Vertex vertex;
                vertex.world = world * local * Vec4f(local_pos, 1.0f);
                vertex.view  = camera * Vec4f(vertex.world, 1.0f);
                vertex.uv    = uv;
                vertex.cull  = vertex.view;

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
                vertex.cull = Vec3f(vertex.device.x, vertex.device.y, 0.0f); // So that rasterizer can cut up polygons into triangles
            }

            rasterize_polygon(vertices, frame_buffer->color, frame_buffer->depth, obj.texture);

            // for (int e = 0; e < vertices.size(); e++)
            // {
            //     Vertex v0 = vertices[e];
            //     Vertex v1 = vertices[(e + 1) % vertices.size()];

            //     v0.color = Vec3f(1.0f);
            //     v1.color = Vec3f(1.0f);

            //     rasterize_line(v0, v1, 5, frame_buffer->color, frame_buffer->depth);
            // }

            // for (int p = 0; p < vertices.size(); p++)
            // {
            //     Vertex point = vertices[p];
            //     point.color = Vec3f(1.0f, 1.0f, 1.0f);

            //     rasterize_point(point, 8, frame_buffer->color, frame_buffer->depth  );
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

    Vec3f BLUEISH (0.2f, 0.2f, clampf(0.5f * sin(SDL_GetTicks() * 0.0005f) + 0.5f, 0.0f, 1.0f));
    Vec3f YELLOW (0.85f, 0.9f, 0.0f);

    // Clear and render into render buffer
    clear_buffer(BLUEISH.raw, state.render_buffer->color);
    clear_buffer(&MAX_DEPTH, state.render_buffer->depth);
    render_scene(state.render_buffer);

    // Clear and blit onto screen res buffer
    Vec2f offset (state.screen_res_buffer->width * clampf(0.25f * sin(SDL_GetTicks() * 0.001f) + 0.25f, 0.0f, 1.0f), state.screen_res_buffer->height * clampf(0.25f * cos(SDL_GetTicks() * 0.001f) + 0.25f, 0.0f, 1.0f));
    clear_buffer(YELLOW.raw, state.screen_res_buffer->color);
    clear_buffer(&MAX_DEPTH, state.screen_res_buffer->depth);
    blit_buffer(state.render_buffer->color, state.screen_res_buffer->color, offset.x, offset.y, 0.5f, 0.5f);

    // Blit onto window
    blit_window(state.screen_res_buffer->color->data);

    // clear_buffer(BLUEISH.raw, state.render_buffer->color);
    // clear_buffer(&MAX_DEPTH, state.render_buffer->depth);
    // render_scene(state.render_buffer);
    // blit_window(state.render_buffer->color->data);
}

void update()
{
    state.objects[0].pitch += 0.01f;
    state.objects[0].yaw += 0.025f;
    state.objects[0].scale.x = sin(SDL_GetTicks() * 0.0025f) + 2.0f; 

    if (input_actions.exit_program)
    {
        state.running = false; // TODO: maybe have a exit() function
    }

    if (input_actions.resize_window)
    {
        // Update window
        resize_window(window.input.window.new_width, window.input.window.new_height);

        // Update screen resolution buffer
        state.screen_res_buffer->width = window.input.window.new_width;
        state.screen_res_buffer->height = window.input.window.new_height;
        resize_buffer(window.input.window.new_width, window.input.window.new_height, state.screen_res_buffer->color);
        resize_buffer(window.input.window.new_width, window.input.window.new_height, state.screen_res_buffer->depth);

        // Update render buffer
        state.render_buffer->width = window.input.window.new_width * RESOLUTION_SCALERS[state.resolution_scale_index];
        state.render_buffer->height = window.input.window.new_height * RESOLUTION_SCALERS[state.resolution_scale_index];
        resize_buffer(window.input.window.new_width * RESOLUTION_SCALERS[state.resolution_scale_index], window.input.window.new_height * RESOLUTION_SCALERS[state.resolution_scale_index], state.render_buffer->color);
        resize_buffer(window.input.window.new_width * RESOLUTION_SCALERS[state.resolution_scale_index], window.input.window.new_height * RESOLUTION_SCALERS[state.resolution_scale_index], state.render_buffer->depth);
        
        // Update camera aspect ratio
        state.camera.aspect_ratio = ((float) window.input.window.new_width) / ((float) window.input.window.new_height);
    }

    if (input_actions.update_mouse_pos)
    {
        Vec2f mouse_pos = Vec2f(window.input.mouse.pos.x, window.height - window.input.mouse.pos.y);
        map_sample_point(mouse_pos.raw, state.screen_res_buffer->color, state.render_buffer->color, state.mouse_pos.raw);
    }

    if (input_actions.cycle_resolution)
    {
        state.resolution_scale_index = (state.resolution_scale_index + 1) % RESOLUTION_SCALERS_COUNT;

        // Update render buffer
        state.render_buffer->width = window.width * RESOLUTION_SCALERS[state.resolution_scale_index];
        state.render_buffer->height = window.height * RESOLUTION_SCALERS[state.resolution_scale_index];
        resize_buffer(window.width * RESOLUTION_SCALERS[state.resolution_scale_index], window.height * RESOLUTION_SCALERS[state.resolution_scale_index], state.render_buffer->color);
        resize_buffer(window.width * RESOLUTION_SCALERS[state.resolution_scale_index], window.height * RESOLUTION_SCALERS[state.resolution_scale_index], state.render_buffer->depth);
    }

    if (input_actions.change_camera_orientation) // only pitch and yaw
    {
        float dx = window.input.mouse.delta.x;
        float dy = window.input.mouse.delta.y;
        float sensitivity_y = 0.0025f, sensitivity_x = 0.0025f;

        float MIN_PITCH = radians(5.0f);
        float MAX_PITCH = radians(175.0f);

        state.camera.yaw += dx * sensitivity_x;
        state.camera.pitch += (-dy) * sensitivity_y;
        state.camera.pitch = state.camera.pitch > MAX_PITCH ? MAX_PITCH : state.camera.pitch < MIN_PITCH ? MIN_PITCH : state.camera.pitch;

        Vec3f look_towards;
        look_towards.x = sin(state.camera.pitch) * sin(state.camera.yaw);
        look_towards.y = cos(state.camera.pitch);
        look_towards.z = sin(state.camera.pitch) * cos(state.camera.yaw);

        state.camera.dir = look_towards;
    }

    float movement_sensitivity = 0.01f;
    if (input_actions.move_camera_forward)
    {
        Mat3x3f camera_inv = Mat4x4f::look_at(state.camera.pos, state.camera.dir, state.camera.up).truncated().transposed();
        state.camera.pos = state.camera.pos + (camera_inv * Vec3f(0.0f, 0.0f, -movement_sensitivity));
    }
    if (input_actions.move_camera_back)
    {
        Mat3x3f camera_inv = Mat4x4f::look_at(state.camera.pos, state.camera.dir, state.camera.up).truncated().transposed();
        state.camera.pos = state.camera.pos + (camera_inv * Vec3f(0.0f, 0.0f, movement_sensitivity));
    }
    if (input_actions.move_camera_left)
    {
        Mat3x3f camera_inv = Mat4x4f::look_at(state.camera.pos, state.camera.dir, state.camera.up).truncated().transposed();
        state.camera.pos = state.camera.pos + (camera_inv * Vec3f(-movement_sensitivity, 0.0f, 0.0f));
    }
    if (input_actions.move_camera_right)
    {
        Mat3x3f camera_inv = Mat4x4f::look_at(state.camera.pos, state.camera.dir, state.camera.up).truncated().transposed();
        state.camera.pos = state.camera.pos + (camera_inv * Vec3f(movement_sensitivity, 0.0f, 0.0f));
    }
}

Buffer* tga_image_to_buffer(TGAImage& img)
{
    Buffer* buffer = new Buffer();
    init_buffer(img.get_width(), img.get_height(), 3, buffer);

    for (int y = 0; y < img.get_height(); y++)
    {
        for (int x = 0; x < img.get_width(); x++)
        {
            TGAColor tga_color = img.get(x, y);
            Vec3f rgb (tga_color.r / 255.0f, tga_color.g / 255.0f, tga_color.b / 255.0f);
            set_element(x, y, rgb.raw, buffer);
        }
    }

    return buffer;
}