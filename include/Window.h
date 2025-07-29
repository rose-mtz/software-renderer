#include <SDL3/SDL.h>
#include "Vec.h"

struct Window
{
    SDL_Window* handle = nullptr;
    SDL_Surface* draw_surface = nullptr;
    int* pixels = nullptr;
    int width;
    int height;
} extern window;

void init_window(int width, int height);
void resize_window(int width, int height);
void blit_window(Vec3f* pixels);