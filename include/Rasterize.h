#pragma once
#include "Vec.h"
#include "FrameBuffer.h"
#include <vector>
#include "Vertex.h"

void rasterize_point(const Vertex& v, int width, Buffer* buffer);
void rasterize_line(const Vertex& v0, const Vertex& v1, int width, Buffer* buffer);
void rasterize_polygon(const std::vector<Vertex>& vertices, Buffer* buffer);

// NOTE: currently, rasterizer expect in-bounds device coordinates
//          if they are out-of-bounds, and the primitive to render
//          is large, then it will still try to rasterizer every pixel it covers
//          this can really slow down performance, halt it even