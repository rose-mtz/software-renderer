#include <SDL3/SDL.h>
#include "Vec.h"

enum KEY_CODES { KEY_A = 0, KEY_SPACE, KEY_ENTER, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_COUNT };

struct KeyState
{
    bool is_down = false;
    bool prev_state = false;
};

struct ButtonState
{
    bool is_down = false;
    bool prev_state = false;
};

struct MouseEvent
{
    Vec2f pos;
    bool did_move = false;

    Vec2f delta; // delta from previous position
    
    ButtonState left;
    ButtonState right;
};

struct WindowEvent
{
    bool did_resize = false;
    int new_width = false;
    int new_height = false;
};

struct UserInput
{
    KeyState keys[KEY_COUNT];
    MouseEvent mouse;
    WindowEvent window;
    bool quit = false;
};

struct Window
{
    SDL_Window* handle = nullptr;
    SDL_Surface* draw_surface = nullptr;
    int* pixels = nullptr;
    int width;
    int height;
    UserInput input;
} extern window;

void init_window(int width, int height);
void resize_window(int width, int height);
void blit_window(Vec3f* pixels);
void poll_events();