#pragma once
#include "Math.h"
#include <vector>

struct Buffer
{
    int width, height;
    Vec3f* pixels = nullptr;
};

void rasterize_point(Vec2f point, Vec3f color, int width, Buffer* buffer);
void rasterize_line(Vec2f point_1, Vec2f point_2, Vec3f color, int width, Buffer* buffer);
void rasterize_polygon(std::vector<Vec2f> polygon, Buffer* buffer);