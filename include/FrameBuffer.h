#pragma once
#include "Math.h"

struct Buffer
{
    int width, height;
    Vec3f* pixels = nullptr;
};

struct Fragment
{
    Vec2i pixel;
    Vec3f color;
    float opacity;
    float depth;
};


void  clear_buffer(Vec3f color, Buffer* buffer);
void  resize_buffer(Buffer* buffer, int width, int height);
void  blit_buffer(Buffer* src, Buffer* target);
void  set_fragment(const Fragment& frag, Buffer* buffer);