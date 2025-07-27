#include "Vertex.h"
#include <cassert>

EdgeTracker set_up_edge_tracker(const Vertex& start, const Vertex& end, bool step_in_y_direction)
{
    // NOTE: its up to user to set up x_integer, and y_integer

    EdgeTracker edge;

    float delta;
    if (step_in_y_direction)
    {
        delta = end.device.y - start.device.y;
    }
    else
    {
        delta = end.device.x - start.device.x;
    }
    assert(delta != 0.0f); // delta must not be zero
    float one_over_delta = 1.0f / delta;

    // Note: likely that dx/dy * dy != 1.0f due to floating point rounding (close to 1.0f, but not exact) [same goes for dy/dx * dx]
    //       up to user to use integers in that case for exact integer math
    edge.x_inc = (end.device.x - start.device.x) * one_over_delta;
    edge.y_inc = (end.device.y - start.device.y) * one_over_delta;
    edge.z_inc = (end.depth    - start.depth) * one_over_delta;

    edge.r_inc = (end.color.x - start.color.x) * one_over_delta;
    edge.g_inc = (end.color.y - start.color.y) * one_over_delta;
    edge.b_inc = (end.color.z - start.color.z) * one_over_delta;

    edge.r = start.color.x;
    edge.g = start.color.y;
    edge.b = start.color.z;
    edge.x = start.device.x;
    edge.y = start.device.y;
    edge.z = start.depth;

    return edge;
}

void take_step(EdgeTracker& edge, float step)
{
    // NOTE: taking a positive step forward does not necessarily mean taking a step towards end, could be taking 
    //          step away from it 

    edge.x += edge.x_inc * step;
    edge.y += edge.y_inc * step;
    edge.z += edge.z_inc * step;
    edge.r += edge.r_inc * step;
    edge.g += edge.g_inc * step;
    edge.b += edge.b_inc * step;
};

Vertex get_vertex(const EdgeTracker& edge)
{
    // NOTE: its up to user to correct vertex.device coordinates with
    //          x_integer and y_integer if they use them
    //          its likely default values will be slightly off due to float rounding

    Vertex vertex;
    vertex.color = Vec3f(edge.r, edge.g, edge.b);
    vertex.device = Vec2f(edge.x, edge.y);
    vertex.depth = edge.z;

    return vertex;
}