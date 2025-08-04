#include "Vec.h"

struct Camera
{
    Vec3f pos;
    Vec3f dir;
    Vec3f up;

    float aspect_ratio; // width/height
    float near;

    float pitch, yaw;
};