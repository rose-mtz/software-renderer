#include <SDL3/SDL.h>
#include <iostream>
#include "Window.h"

Vec4f* color_buffer = nullptr;

void resize_color_buffer();
void clear_color_buffer(Vec4f color);

int main()
{
    init_window(1280, 720);    
    resize_color_buffer();

    bool running = true;
    while (running)
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
                resize_window(event.window.data1, event.window.data2);   
                resize_color_buffer();
            }
        }

        clear_color_buffer(Vec4f(1.0f, 0.0f, 1.0f, 1.0f));
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