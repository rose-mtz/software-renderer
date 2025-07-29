#include "Geometry.h"
#include <cassert>


// Cuts a polygon into two pieces
// Cut is horizontal and is at y
// top piece is above the cut line, bottom piece is bellow it
// Polygon MUST have some winding
void cut_polygon(const std::vector<Vertex>& polygon, float y, std::vector<Vertex>& top, std::vector<Vertex>& bottom)
{
    for (int i = 0; i < polygon.size(); i++)
    {
        const Vertex& cur = polygon[i];
        float cur_dy = y - cur.device.y; // relative to cut line

        if (cur_dy >= 0.0f)
        {
            bottom.push_back(cur);
        }
        if (cur_dy <= 0.0f)
        {
            top.push_back(cur);
        }

        const Vertex& next = polygon[(i + 1) % polygon.size()];
        float next_dy = y - next.device.y; // relative to cut line

        bool crossed_cut_line = (((cur_dy < 0.0f) && (next_dy > 0.0f)) || ((cur_dy > 0.0f) && (next_dy < 0.0f)));
        if (crossed_cut_line)
        {
            assert(next.device.y != cur.device.y);

            EdgeTracker edge = set_up_edge_tracker(cur, next, true);
            take_step(edge, cur_dy);

            // WARNING: triangle rasterizer expects exact value (no floating point rounding)
            edge.v.device.y = y;

            top.push_back(edge.v);
            bottom.push_back(edge.v);
        }
    }
}

Vec3f reflect_vector(const Vec3f& surface_normal, const Vec3f& vector)
{
    // CREDIT: https://math.stackexchange.com/questions/13261/how-to-get-a-reflection-vector
    return (vector - (surface_normal * 2.0f * (vector * surface_normal))).normalized();
}

Vec3f get_triangle_normal(const Vec3f& a, const Vec3f& b, const Vec3f& c)
{
    Vec3f v1 = b - a;
    Vec3f v2 = c - a;

    return (v1 ^ v2).normalized();
}