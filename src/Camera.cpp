#include "Camera.h"

Frustum get_frustum(Camera cam)
{
    // NOTE: won't work for orthographic cameras

    Frustum frustum;

    float w, h;
    w = cam.aspect_ratio;
    h = 1.0f;

    // NOTE: for now frustum must be non-negative numbers

    frustum.n = cam.near;
    frustum.f = cam.far;
    frustum.l = w/2.0f;
    frustum.r = w/2.0f;
    frustum.t = h/2.0f;
    frustum.b = h/2.0f;

    return frustum;
}