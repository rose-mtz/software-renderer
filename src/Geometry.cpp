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

std::vector<Plane> get_frustum_planes(Frustum fru)
{
    std::vector<Plane> planes;

    // WARNING: I have assumed this is how the normals of hte planes are made
    //          I NEVER checked it, so CHECK IT

    Plane top    {  0.0f,       -1.0f/fru.t, -1.0f/fru.n,  0.0f  };
    Plane bottom {  0.0f,        1.0f/fru.b, -1.0f/fru.n,  0.0f  };
    Plane left   {  1.0f/fru.l,  0.0f,       -1.0f/fru.n,  0.0f  };
    Plane right  { -1.0f/fru.r,  0.0f,       -1.0f/fru.n,  0.0f  };
    Plane far    {  0.0f,        0.0f,        1.0f,        fru.f };
    Plane near   {  0.0f,        0.0f,       -1.0f,       -fru.n };

    planes.push_back(top);
    planes.push_back(bottom);
    planes.push_back(left);
    planes.push_back(right);
    planes.push_back(far);
    planes.push_back(near);

    return planes;
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

std::vector<Vertex> cull_polygon(std::vector<Vertex> polygon, Plane plane)
{
    // NOTE: culling is happening in view space
    //          need to figure out way to not care about what space
    //          culling happens

    float one_over_length = 1.0f / Vec3f(plane.a, plane.b, plane.c).length();
    plane.a *= one_over_length;
    plane.b *= one_over_length;
    plane.c *= one_over_length;
    plane.d *= one_over_length; // NOTE: likely an issue!!!

    Vec3f norm (plane.a, plane.b, plane.c);
    float d = plane.d;

    std::vector<Vertex> in;
    for (int i = 0; i < polygon.size(); i++)
    {
        Vertex cur = polygon[i];
        float cur_delta = norm * cur.view + d;

        float fudge = 0.001f;

        bool is_cur_in = cur_delta > fudge;
        bool is_cur_on = std::abs(cur_delta) <= fudge;
        if (is_cur_in || is_cur_on)
        {
            in.push_back(cur);
        }

        Vertex next = polygon[(i + 1) % polygon.size()];
        float next_delta = norm * next.view + d;
        bool is_next_in = next_delta > fudge;
        bool is_next_on = std::abs(cur_delta) <= fudge;

        if (!is_cur_on && !is_next_on && ((is_cur_in && !is_next_in) || (!is_cur_in && is_next_in)))
        {
            float total_length = (next.view - cur.view).length();
            Vec3f dir = (next.view - cur.view) * (1.0f/total_length);
            float length = std::abs(cur_delta / (dir * norm));

            in.push_back(interpolate_vertex(cur, next, length/total_length));
        }
    }

    return in;
}