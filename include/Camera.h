#include "Vec.h"

struct Camera
{
    Vec3f pos;
    Vec3f look_at;
    Vec3f up;

    float aspect_ratio; // width/height
    float near;
};