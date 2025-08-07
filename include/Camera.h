#pragma once

#include "Vec.h"

struct Camera
{
    Vec3f pos;
    Vec3f dir;
    Vec3f up;

    float aspect_ratio; // width/height
    float near;
    float far;

    float pitch, yaw;
};

struct Frustum { float l, r, t, b, n, f; };
Frustum get_frustum(Camera cam);