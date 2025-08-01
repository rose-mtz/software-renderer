#include "FrameBuffer.h"
#include <cassert>

// returns true if out-of-bounds
bool is_out_of_bounds(const Vec2i& pixel, Buffer* buffer)
{
    return (pixel.x < 0 || pixel.x >= buffer->width) || (pixel.y < 0 || pixel.y >= buffer->height);
}

// Does not check out-of-bounds input
void set_pixel(int x, int y, Vec3f value, Buffer* buffer)
{
    assert(x >= 0 && x < buffer->width);
    assert(y >= 0 && y < buffer->height);

    buffer->pixels[y * buffer->width + x] = value;
}

Vec3f get_pixel(int x, int y, Buffer* buffer)
{
    assert(x >= 0 && x < buffer->width);
    assert(y >= 0 && y < buffer->height);

    return buffer->pixels[y * buffer->width + x];
}

int clampInt(int i, int min, int max)
{
    return i > max ? max : i < min ? min : i;
}

Vec3f get_pixel_clamped(int x, int y, Buffer* buffer)
{
    return get_pixel(clampInt(x, 0, buffer->width - 1), clampInt(y, 0, buffer->height - 1), buffer);
}

Vec3f bilinear_sample(Vec2f point, Buffer* buffer)
{
    assert(point.x >= 0.0f && point.x <= (float) buffer->width);
    assert(point.y >= 0.0f && point.y <= (float) buffer->height);

    Vec2i point_rounded (round(point.x), round(point.y));

    Vec3f top_left     = get_pixel_clamped(point_rounded.x - 1, point_rounded.y,     buffer);
    Vec3f top_right    = get_pixel_clamped(point_rounded.x,     point_rounded.y,     buffer);
    Vec3f bottom_left  = get_pixel_clamped(point_rounded.x - 1, point_rounded.y - 1, buffer);
    Vec3f bottom_right = get_pixel_clamped(point_rounded.x,     point_rounded.y - 1, buffer);

    Vec3f top_interpolation = lerpVec3f(top_left, top_right, 0.5f);
    Vec3f bottom_interpolation = lerpVec3f(bottom_left, bottom_right, 0.5f);
    Vec3f cross_interpolation = lerpVec3f(top_interpolation, bottom_interpolation, 0.5f);

    return cross_interpolation;
}

void resize_buffer(Buffer* buffer, int width, int height)
{
    if (!buffer->pixels) delete[] buffer->pixels;
    if (!buffer->depth) delete[] buffer->depth;

    buffer->pixels = new Vec3f[width * height];
    buffer->depth = new float[width * height];
    buffer->width = width;
    buffer->height = height;

    // Temporary
    for (int i = 0; i < buffer->width * buffer->height; i++)
    {
        buffer->depth[i] = -1000000.0f;
    }
}

void clear_buffer(Vec3f color, Buffer* buffer)
{
    for (int i = 0; i < buffer->width * buffer->height; i++)
    {
        buffer->pixels[i] = color;
        buffer->depth[i] = -1000000.0f;
    }
}

// Only blitz color
void blit_buffer(Buffer* src, Buffer* target)
{
    // TODO: let user pick offset to blitz on target
    //          and also % width & % height to blitz

    // CORRECTNESS: it seems, theoretically, that if the src and targe resolutions
    //              difference is too high then bilinear sampling may be
    //              the wrong sampling strategy

    float x_basis_scale = ((float) src->width) / ((float) target->width);
    float y_basis_scale = ((float) src->height) / ((float) target->height);

    for (int y = 0; y < target->height; y++)
    {
        for (int x = 0; x < target->width; x++)
        {
            Vec2f sample_point ((x + 0.5f) * x_basis_scale, (y + 0.5f) * y_basis_scale);
            Vec3f sample = bilinear_sample(sample_point, src);
            set_pixel(x,y, sample, target);
        }
    }
}

void set_fragment(const Fragment& frag, Buffer* buffer)
{
    if (!is_out_of_bounds(frag.pixel, buffer) && buffer->depth[frag.pixel.y * buffer->width + frag.pixel.x] < frag.depth)
    {
        // No depth check for now.
        buffer->pixels[frag.pixel.y * buffer->width + frag.pixel.x] = frag.color;
        buffer->depth[frag.pixel.y * buffer->width + frag.pixel.x] = frag.depth;
    }
}