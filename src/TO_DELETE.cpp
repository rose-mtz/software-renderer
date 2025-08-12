// #include "FrameBuffer.h"
// #include <cassert>

// // Maps a point from one buffer to another
// Vec2f map_point(const Vec2f& point, Buffer* src, Buffer* target)
// {
//     float x_scale = ((float) target->width) / ((float) src->width);
//     float y_scale = ((float) target->height) / ((float) src->height);

//     return Vec2f(point.x * x_scale, point.y * y_scale);
// }

// // returns true if out-of-bounds
// bool is_out_of_bounds(const Vec2i& pixel, Buffer* buffer)
// {
//     return (pixel.x < 0 || pixel.x >= buffer->width) || (pixel.y < 0 || pixel.y >= buffer->height);
// }

// // true if is closer
// bool is_hidden(const Vec2i& pixel, float depth, Buffer* buffer)
// {
//     assert(pixel.x >= 0 && pixel.x < buffer->width);
//     assert(pixel.y >= 0 && pixel.y < buffer->height);

//     return buffer->depth[pixel.y * buffer->width + pixel.x] > depth;
// }

// // Does not check out-of-bounds input
// void set_pixel(int x, int y, Vec3f value, Buffer* buffer)
// {
//     assert(x >= 0 && x < buffer->width);
//     assert(y >= 0 && y < buffer->height);

//     buffer->pixels[y * buffer->width + x] = value;
// }

// Vec3f get_pixel(int x, int y, Buffer* buffer)
// {
//     assert(x >= 0 && x < buffer->width);
//     assert(y >= 0 && y < buffer->height);

//     return buffer->pixels[y * buffer->width + x];
// }

// int clampInt(int i, int min, int max)
// {
//     return i > max ? max : i < min ? min : i;
// }

// Vec3f get_pixel_clamped(int x, int y, Buffer* buffer)
// {
//     return get_pixel(clampInt(x, 0, buffer->width - 1), clampInt(y, 0, buffer->height - 1), buffer);
// }

// Vec3f bilinear_sample(Vec2f point, Buffer* buffer)
// {
//     assert(point.x >= 0.0f && point.x <= (float) buffer->width);
//     assert(point.y >= 0.0f && point.y <= (float) buffer->height);

//     Vec2i point_rounded (round(point.x), round(point.y));

//     Vec3f top_left     = get_pixel_clamped(point_rounded.x - 1, point_rounded.y,     buffer);
//     Vec3f top_right    = get_pixel_clamped(point_rounded.x,     point_rounded.y,     buffer);
//     Vec3f bottom_left  = get_pixel_clamped(point_rounded.x - 1, point_rounded.y - 1, buffer);
//     Vec3f bottom_right = get_pixel_clamped(point_rounded.x,     point_rounded.y - 1, buffer);

//     Vec3f top_interpolation    = lerpVec3f(top_left, top_right, 0.5f);
//     Vec3f bottom_interpolation = lerpVec3f(bottom_left, bottom_right, 0.5f);
//     Vec3f cross_interpolation  = lerpVec3f(top_interpolation, bottom_interpolation, 0.5f);

//     return cross_interpolation;
// }

// void resize_buffer(Buffer* buffer, int width, int height)
// {
//     if (!buffer->pixels) delete[] buffer->pixels;
//     if (!buffer->depth) delete[] buffer->depth;

//     buffer->pixels = new Vec3f[width * height];
//     buffer->depth = new float[width * height];
//     buffer->width = width;
//     buffer->height = height;

//     for (int i = 0; i < buffer->width * buffer->height; i++)
//     {
//         buffer->depth[i] = MAX_DEPTH;
//     }
// }

// // Clears color and depth
// void clear_buffer(Vec3f color, Buffer* buffer)
// {
//     for (int i = 0; i < buffer->width * buffer->height; i++)
//     {
//         buffer->pixels[i] = color;
//         buffer->depth[i] = MAX_DEPTH;
//     }
// }

// float max(float a, float b)
// {
//     return a > b ? a : b;
// }

// float min(float a, float b)
// {
//     return a < b ? a : b;
// }

// // Only blitz color
// void blit_buffer(Buffer* src, Buffer* target, Vec2f offset, float width_percent, float height_percent)
// {
//     assert(width_percent > 0.0f && height_percent > 0.0f);

//     // CORRECTNESS: it seems, theoretically, that if the src and targe resolutions
//     //              difference is too high then bilinear sampling may be
//     //              the wrong sampling strategy

//     Vec2f temp = offset + Vec2f(target->width * width_percent, target->height * height_percent);
//     Vec2i bottom_left (max(offset.x, 0.0f), max(offset.y, 0.0f));
//     Vec2i top_right (min(temp.x, target->width), min(temp.y, target->height));

//     float x_scale = ((float) src->width) / ((float) target->width) * (1.0f / width_percent);
//     float y_scale = ((float) src->height) / ((float) target->height) * (1.0f / height_percent);

//     for (int y = bottom_left.y; y < top_right.y; y++)
//     {
//         for (int x = bottom_left.x; x < top_right.x; x++)
//         {
//             // REFACTOR: could refactor to reduce conversions, and maybe just do increments
//             Vec2f sample_point ((x + 0.5f - offset.x) * x_scale, (y + 0.5f - offset.y) * y_scale); // ROBUSTNESS: could sample point be outside of bounds of src buffer?
//             Vec3f sample = bilinear_sample(sample_point, src); // TODO: maybe give user option for bilinear or closest
//             set_pixel(x, y, sample, target);
//         }
//     }
// }

// void set_fragment(const Fragment& frag, Buffer* buffer)
// {
//     if (!is_out_of_bounds(frag.pixel, buffer) && !is_hidden(frag.pixel, frag.depth, buffer))
//     {
//         buffer->pixels[frag.pixel.y * buffer->width + frag.pixel.x] = frag.color;
//         buffer->depth[frag.pixel.y * buffer->width + frag.pixel.x] = frag.depth;
//     }
// }