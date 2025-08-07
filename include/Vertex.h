#pragma once
#include "Vec.h"

struct Vertex
{
    Vec2f device;
    float depth;
    Vec3f color;
    Vec3f world;
    Vec3f view;
};

struct EdgeTracker
{
    Vertex v;
    Vertex v_inc;
};

EdgeTracker set_up_edge_tracker(const Vertex& start, const Vertex& end, bool step_in_y_direction);
void take_step(EdgeTracker& edge, float step = 1.0f);
Vertex interpolate_vertex(Vertex v0, Vertex v1, float t);