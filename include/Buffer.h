#pragma once
#include <limits>
#include "Vec.h"

const float MAX_DEPTH = std::numeric_limits<float>::lowest();

struct Buffer
{
    float* data;
    int width, height, fpp; // floats-per-element
};

// TODO: might be better to use float* uv, and float* offset, less clutter, more consistent
// TODO: make consts!!!
void set_element      (int x, int y, float* elm, Buffer* buf);
void get_element      (int x, int y, float* elm, Buffer* buf);
void sample_bilinear  (float u, float v, float* smpl, Buffer* buf);
void sample_nearest   (float u, float v, float* smpl, Buffer* buf);
void clear_buffer     (const float* clear, Buffer* buf);
void resize_buffer    (int width, int height, Buffer* buf);
void init_buffer      (int width, int height, int fpp, Buffer* buf);
void blit_buffer      (Buffer* src, Buffer* target, float x_offset, float y_offset, float width_percent, float height_percent);
void map_sample_point (float* point, Buffer* src, Buffer* target, float* mapped_point);

// I'm stuck at two options:
//  1) I could try switching to using Vec file here to clean stuff up
//  2) or I could just trying cleaning things up w/o it
