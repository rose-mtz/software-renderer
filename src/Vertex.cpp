#include "Vertex.h"
#include <cassert>

// v0 will be set the initial value of point on edge
// to start on
EdgeTracker set_up_edge_tracker(const Vertex& v0, const Vertex& v1, bool step_in_y_direction)
{
    float delta;
    if (step_in_y_direction)
    {
        delta = v1.device.y - v0.device.y;
    }
    else
    {
        delta = v1.device.x - v0.device.x;
    }
    assert(delta != 0.0f); // delta must not be zero
    float one_over_delta = 1.0f / delta;

    EdgeTracker edge;

    // Note: likely that dx/dy * dy != 1.0f due to floating point rounding (close to 1.0f, but not exact) [same goes for if its dy/dx * dx]
    //       up to user to use integers in that case for exact integer math
    edge.v_inc.device.x = (v1.device.x - v0.device.x) * one_over_delta;
    edge.v_inc.device.y = (v1.device.y - v0.device.y) * one_over_delta;
    edge.v_inc.depth    = (v1.depth    - v0.depth   ) * one_over_delta;

    edge.v_inc.color.x = (v1.color.x - v0.color.x) * one_over_delta;
    edge.v_inc.color.y = (v1.color.y - v0.color.y) * one_over_delta;
    edge.v_inc.color.z = (v1.color.z - v0.color.z) * one_over_delta;

    edge.v = v0;

    return edge;
}

Vertex interpolate_vertex(Vertex v0, Vertex v1, float t)
{
    assert(t >= 0.0f && t <= 1.0f);

    Vertex interp;
    interp.world  = v0.world  * (1.0f - t) + v1.world  * t;
    interp.view   = v0.view   * (1.0f - t) + v1.view   * t;
    interp.device = v0.device * (1.0f - t) + v1.device * t;
    interp.color  = v0.color  * (1.0f - t) + v1.color  * t;
    interp.depth  = v0.depth  * (1.0f - t) + v1.depth  * t;
    interp.cull   = v0.cull   * (1.0f - t) + v1.cull   * t;

    return interp;
}

void take_step(EdgeTracker& edge, float step)
{
    edge.v.device.x += edge.v_inc.device.x * step;
    edge.v.device.y += edge.v_inc.device.y * step;
    edge.v.depth    += edge.v_inc.depth    * step;
    edge.v.color.x  += edge.v_inc.color.x  * step;
    edge.v.color.y  += edge.v_inc.color.y  * step;
    edge.v.color.z  += edge.v_inc.color.z  * step;
};
