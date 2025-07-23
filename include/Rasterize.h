#pragma once
#include "Math.h"
#include "FrameBuffer.h"
#include <vector>

struct Vertex
{
    Vec2f device;
    float depth;
    Vec3f color;
};

void rasterize_point(Vertex v, int width, Buffer* buffer);
void rasterize_line(Vertex v0, Vertex v1, int width, Buffer* buffer);
void rasterize_polygon(const std::vector<Vertex>& vertices, Buffer* buffer);