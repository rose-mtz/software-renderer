#pragma once
#include "Vec.h"
#include "Buffer.h"
#include <vector>
#include "Vertex.h"
#include "tgaimage.h"

struct Fragment
{
    Vec2i pixel;
    Vec3f color;
    float opacity;
    float depth;
};

void rasterize_point(const Vertex& v, int width, Buffer* color_buffer, Buffer* depth_buffer, std::vector<Fragment>& fragments);
void rasterize_line(const Vertex& v0, const Vertex& v1, int width, Buffer* color_buffer, Buffer* depth_buffer, std::vector<Fragment>& fragments);
void rasterize_polygon(const std::vector<Vertex>& vertices, Buffer* color_buffer, Buffer* depth_buffer, Buffer* texture, std::vector<Fragment>& fragments);

// NOTE: currently, rasterizer expect in-bounds device coordinates
//          if they are out-of-bounds, and the primitive to render
//          is large, then it will still try to rasterizer every pixel it covers
//          this can really slow down performance, halt it even