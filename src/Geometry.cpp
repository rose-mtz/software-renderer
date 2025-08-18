#include "Geometry.h"
#include <cassert>


std::vector<Plane> get_frustum_planes(Frustum fru)
{
    std::vector<Plane> planes;

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

void cull_polygon(const std::vector<Vertex>& polygon, Plane plane, std::vector<Vertex>& in, std::vector<Vertex>& out, float epsilon)
{
    // TODO: maybe let user handle normalization, since sometimes user will already pass in normalized plane
    float one_over_length = 1.0f / Vec3f(plane.a, plane.b, plane.c).length();
    plane.a *= one_over_length;
    plane.b *= one_over_length;
    plane.c *= one_over_length;
    plane.d *= one_over_length;

    Vec3f norm (plane.a, plane.b, plane.c);
    float d = plane.d;

    for (int i = 0; i < polygon.size(); i++)
    {
        Vertex cur = polygon[i];
        float cur_delta = (norm * cur.cull) + d;

        bool is_cur_in = cur_delta > epsilon;
        bool is_cur_on = std::abs(cur_delta) <= epsilon;
        if (is_cur_on)
        {
            in.push_back(cur);
            out.push_back(cur);
        }
        else if (is_cur_in)
        {
            in.push_back(cur);
        }
        else
        {
            out.push_back(cur);
        }

        Vertex next = polygon[(i + 1) % polygon.size()];
        float next_delta = (norm * next.cull) + d;
        bool is_next_in = next_delta > epsilon;
        bool is_next_on = std::abs(next_delta) <= epsilon;

        if (!is_cur_on && !is_next_on && ((is_cur_in && !is_next_in) || (!is_cur_in && is_next_in)))
        {
            float total_length = (next.cull - cur.cull).length();
            Vec3f dir = (next.cull - cur.cull) * (1.0f/total_length);
            float length = std::abs(cur_delta / (dir * norm));

            Vertex interp = interpolate_vertex(cur, next, length/total_length); 
            in.push_back(interp);
            out.push_back(interp);
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