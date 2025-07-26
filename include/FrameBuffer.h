#pragma once
#include "Math.h"

struct Buffer
{
    int width, height;
    Vec3f* pixels = nullptr;
};


bool  is_out_of_bounds(int x, int y, Buffer* buffer);
void  set_pixel(int x, int y, Vec3f value, Buffer* buffer);
Vec3f get_pixel(int x, int y, Buffer* buffer);
void  clear_buffer(Vec3f color, Buffer* buffer);
void  resize_buffer(Buffer* buffer, int width, int height);
void  blit_buffer(Buffer* src, Buffer* target);