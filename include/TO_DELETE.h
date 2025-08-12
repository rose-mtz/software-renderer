// #pragma once
// #include <limits>
// #include "Vec.h"

// struct Buffer
// {
//     int width, height;
//     Vec3f* pixels = nullptr;
//     float* depth = nullptr;
// };

// struct Fragment
// {
//     Vec2i pixel;
//     Vec3f color;
//     float opacity;
//     float depth;
// };


// void  clear_buffer(Vec3f color, Buffer* buffer);
// void  resize_buffer(Buffer* buffer, int width, int height);
// void  blit_buffer(Buffer* src, Buffer* target, Vec2f offset, float width_percent, float height_percent);
// void  set_fragment(const Fragment& frag, Buffer* buffer);

// Vec2f map_point(const Vec2f& point, Buffer* src, Buffer* target);

// const float MAX_DEPTH = std::numeric_limits<float>::lowest();