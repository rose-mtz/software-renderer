#include "Util.h"
#include <cassert>

float clampf(float f, float min, float max)
{
    assert(min <= max);
    return f < min ? min : f > max ? max : f;
}

float clampi(int i, int min, int max)
{
    assert(min <= max);
    return i < min ? min : i > max ? max : i;
}

float lerpf(float a, float b, float t)
{
    assert(t >= 0.0f && t <= 1.0f);
    return a * (1.0f - t) + b * t;
}

float maxf(float a, float b)
{
    return a > b ? a : b;
}

float minf(float a, float b)
{
    return a < b ? a : b;
}