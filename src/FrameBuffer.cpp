#include "FrameBuffer.h"
#include <cassert>

// returns false if out-of-bounds
bool set_pixel(int x, int y, Vec3f value, Buffer* buffer)
{
    if (x < 0 || x >= buffer->width) return false;
    if (y < 0 || y >= buffer->height) return false;

    buffer->pixels[y * buffer->width + x] = value;
    return true;
}

Vec3f get_pixel(int x, int y, Buffer* buffer)
{
    assert(x >= 0 && x < buffer->width);
    assert(y >= 0 && y < buffer->height);

    return buffer->pixels[y * buffer->width + x];
}

void resize_buffer(Buffer* buffer, int width, int height)
{
    if (!buffer->pixels) delete[] buffer->pixels;

    buffer->pixels = new Vec3f[width * height];
    buffer->width = width;
    buffer->height = height;
}

void clear_buffer(Vec3f color, Buffer* buffer)
{
    for (int i = 0; i < buffer->width * buffer->height; i++)
    {
        buffer->pixels[i] = color;
    }
}

void blit_buffer(Buffer* src, Buffer* target) // , float percent_width, float precent_height, Vec2f offset)
{
    // 100% width
    // 100% height
    // offset is (0,0)

    float x_basis_scale = ((float) src->width) / ((float) target->width);
    float y_basis_scale = ((float) src->height) / ((float) target->height);

    for (int y = 0; y < target->height; y++)
    {
        for (int x = 0; x < target->width; x++)
        {
            int x_src = (x + 0.5f) * x_basis_scale;
            int y_src = (y + 0.5f) * y_basis_scale;

            Vec3f sample = get_pixel(x_src, y_src, src);
            set_pixel(x,y, sample, target);
        }
    }
}