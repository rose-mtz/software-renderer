#include <vector>
#include "Vertex.h"
#include "Vec.h"

struct Plane { float a, b, c, d; };

void cut_polygon(const std::vector<Vertex>& polygon, float y, std::vector<Vertex>& top, std::vector<Vertex>& bottom);
std::vector<Vertex> cull_polygon(std::vector<Vertex> polygon, Plane plane);

Vec3f reflect_vector(const Vec3f& surface_normal, const Vec3f& vector);
Vec3f get_triangle_normal(const Vec3f& a, const Vec3f& b, const Vec3f& c);
