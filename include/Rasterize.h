#pragma once
#include "Math.h"
#include "FrameBuffer.h"
#include <vector>
#include "Vertex.h"

void rasterize_point(const Vertex& v, int width, Buffer* buffer);
void rasterize_line(const Vertex& v0, const Vertex& v1, int width, Buffer* buffer);
void rasterize_polygon(const std::vector<Vertex>& vertices, Buffer* buffer);