#include <cassert>
#include "Buffer.h"
#include "Util.h"

const int MAX_FPP = 4;

void set_element(int x, int y, float* elm, Buffer* buf)
{
    assert(x > -1 && x < buf->width);
    assert(y > -1 && y < buf->height);

    for (int i = 0; i < buf->fpp; i++)
    {
        buf->data[(x + (y * buf->width)) * buf->fpp + i] = elm[i];
    }
}

void get_element(int x, int y, float* elm, Buffer* buf)
{
    assert(x > -1 && x < buf->width);
    assert(y > -1 && y < buf->height);

    for (int i = 0; i < buf->fpp; i++)
    {
        elm[i] = buf->data[(x + (y * buf->width)) * buf->fpp + i];
    }
}

// uv will be clamped
void sample_bilinear(float u, float v, float* smpl, Buffer* buf)
{
    assert(u >= -0.5f && u <= 1.5f);
    assert(v >= -0.5f && v <= 1.5f);

    u = clampf(u, 0.0f, 1.0f);
    v = clampf(v, 0.0f, 1.0f);

    assert(buf->fpp <= MAX_FPP);
    float tl[MAX_FPP], tr[MAX_FPP], bl[MAX_FPP], br[MAX_FPP];

    // ASSUMPTION: no 1xn or 1xm or 1x1 buffers
    int x = clampi(u * buf->width, 1, buf->width - 1);
    int y = clampi(v * buf->height, 1, buf->height - 1);

    get_element(x - 1, y,     tl, buf);
    get_element(x,     y,     tr, buf);
    get_element(x - 1, y - 1, bl, buf);
    get_element(x,     y - 1, br, buf);

    for (int i = 0; i < buf->fpp; i++)
    {
        float t = lerpf(tl[i], tr[i], 0.5f);
        float b = lerpf(bl[i], br[i], 0.5f);
        smpl[i] = lerpf(t, b, 0.5f);
    }
}

// uv will be clamped
void sample_nearest(float u, float v, float* smpl, Buffer* buf)
{
    assert(u >= -0.5f && u <= 1.5f);
    assert(v >= -0.5f && v <= 1.5f);

    u = clampf(u, 0.0f, 1.0f);
    v = clampf(v, 0.0f, 1.0f);

    int x = clampi(u * buf->width, 0, buf->width - 1);
    int y = clampi(v * buf->height, 0, buf->height - 1);

    get_element(x, y, smpl, buf);
}

void clear_buffer(const float* clear, Buffer* buf)
{
    for (int i = 0; i < buf->width * buf->height; i++)
    {
        for (int j = 0; j < buf->fpp; j++)
        {
            buf->data[i * buf->fpp + j] = clear[j];
        }
    }
}

// buffer is left un-initialized
void resize_buffer(int width, int height, Buffer* buf)
{
    if (!buf->data) delete[] buf->data;

    buf->data = new float[width * height * buf->fpp];
    buf->width = width;
    buf->height = height;
}

void blit_buffer(Buffer* src, Buffer* target, float x_offset, float y_offset, float width_percent, float height_percent)
{
    assert(width_percent > 0.0f && height_percent > 0.0f);
    assert(target->fpp <= src->fpp);

    // TODO: maybe give user option for bilinear or closest
    // CORRECTNESS: it seems, theoretically, that if the src and targe resolutions
    //              difference is too high then bilinear sampling may be
    //              the wrong sampling strategy

    float temp[2] = { x_offset + target->width * width_percent, y_offset + target->height * height_percent };
    int bottom_left[2] = { maxf(x_offset, 0.0f), maxf(y_offset, 0.0f) };
    int top_right[2] = { minf(temp[0], target->width), minf(temp[1], target->height) };

    float x_scale = ((float) src->width) / ((float) target->width) * (1.0f / width_percent);
    float y_scale = ((float) src->height) / ((float) target->height) * (1.0f / height_percent);

    for (int y = bottom_left[1]; y < top_right[1]; y++)
    {
        for (int x = bottom_left[0]; x < top_right[0]; x++)
        {
            // REFACTOR: could refactor to reduce conversions, and maybe just do increments
            float sample_point[2] = {(x + 0.5f - x_offset) * x_scale / src->width, (y + 0.5f - y_offset) * y_scale / src->height}; // NOTE: FUCK! and uv normalization...
            float sample[MAX_FPP];
            sample_bilinear(sample_point[0], sample_point[1], sample, src);
            set_element(x, y, sample, target);
        }
    }
}

void init_buffer(int width, int height, int fpp, Buffer* buf)
{
    buf->fpp = fpp;
    resize_buffer(width, height, buf);
}

void map_sample_point(float* point, Buffer* src, Buffer* target, float* mapped_point)
{
    float x_scale = ((float) target->width) / ((float) src->width);
    float y_scale = ((float) target->height) / ((float) src->height);

    mapped_point[0] = point[0] * x_scale;
    mapped_point[1] = point[1] * y_scale;
}