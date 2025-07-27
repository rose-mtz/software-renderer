#pragma once
#include "Math.h"

struct Vertex
{
    Vec2f device;
    float depth;
    Vec3f color;
};

struct EdgeTracker
{
    // TODO: just use a vertex as members vertex and vertex_inc
    float r,g,b;
    float x,y,z;

    float r_inc, g_inc, b_inc;
    float x_inc, y_inc, z_inc;
};

EdgeTracker set_up_edge_tracker(const Vertex& start, const Vertex& end, bool step_in_y_direction);
void take_step(EdgeTracker& edge, float step = 1.0f);
Vertex get_vertex(const EdgeTracker& edge);