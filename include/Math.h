#pragma once

struct Vec4f
{
    float x, y, z, w;

    Vec4f() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vec4f(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

struct Vec3f
{
    float x, y, z;

    Vec3f() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
};