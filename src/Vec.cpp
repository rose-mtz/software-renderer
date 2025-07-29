#include "Vec.h"

Vec3f clampedVec3f(const Vec3f& v, float min, float max)
{
    return Vec3f(
        (v.x > max) ? max : (v.x < min) ? min : v.x,
        (v.y > max) ? max : (v.y < min) ? min : v.y,
        (v.z > max) ? max : (v.z < min) ? min : v.z
    );
}

Vec2f clampedVec2f(const Vec2f& v, float min, float max)
{
    return Vec2f(
        (v.x > max) ? max : (v.x < min) ? min : v.x,
        (v.y > max) ? max : (v.y < min) ? min : v.y
    );
}