#include "Rasterize.h"
#include <algorithm>
#include <cassert>


void rasterize_point(Vertex v, int radius, Buffer* buffer)
{
    // KNOWN ISSUE: 
    //      for small radius sizes (<5), the pixels may appear not so square, not circular
    //      for larger radiuses this is not a problem
    // PROPOSED SOLUTION: 
    //      try centering center of circle at center instead of at integer points
    //      or add aa

    /**
     * Process:
     * 
     * round vertex pos to nearest integer point
     * 
     * from bottom scanline to top scanline that circle spans
     *     find x intercepts points at current scanline w/ circle
     *     using current scanline and its x intercepts, rasterize the scanline
     */

    Vec2i center (round(v.device.x), round(v.device.y));

    int stop_scanline = center.y + radius;
    int cur_scanline = center.y - radius;

    while (cur_scanline < stop_scanline)
    {
        int y_intercept = cur_scanline;
        if (cur_scanline < center.y) 
        {
            // for bottom part of circle, we use scanline above current to sample circle intercepts
            y_intercept++; 
        }

        // Relative to circle center (essential move circle to origin)
        int y_rel = y_intercept - center.y;
        // Intercept y_rel with circle centered at origin
        float x_right_rel = sqrt(radius*radius - y_rel*y_rel);
        float x_left_rel = -x_right_rel;
        // Translates intercepts back
        float x_left = x_left_rel + center.x;
        float x_right = x_right_rel + center.x;

        // columns of scanline
        int cur_col = floor(x_left); // left intercept
        int stop_col = ceil(x_right); // right intercept

        while (cur_col < stop_col)
        {
            set_pixel(cur_col, cur_scanline, v.color, buffer);
            cur_col++;
        }

        cur_scanline++;
    }
}

void rasterize_line(Vertex v0, Vertex v1, int width, Buffer* buffer)
{
    Vec2f p0 = v0.device;
    Vec2f p1 = v1.device;
    float slope = (p1.y - p0.y) / (p1.x - p0.x);

    bool inverted_axis = slope > 1.0f || slope < -1.0f;
    if (inverted_axis)
    {
        slope = 1.0f/slope;

        // Swap x and y
        float temp = p0.x;
        p0.x = p0.y;
        p0.y = temp;
        temp = p1.x;
        p1.x = p1.y;
        p1.y = temp;
    }

    if (p0.x > p1.x) std::swap(p0, p1); // march in positive x direction (from p0 to p1)

    float dy = slope; // dy = slope * dx = slope * 1 = slope
    float y = p0.y;

    int col = floor(p0.x);
    int col_stop = ceil(p1.x);
    while (col < col_stop)
    {
        int row = floor(y);

        // Line thickness
        int line_count = 1 + ((width - 1) * 2);
        int cur_line = 0;
        while (cur_line < line_count)
        {
            int current_line_row = (row - (width - 1)) + cur_line;
            if (inverted_axis) set_pixel(current_line_row, col, v0.color, buffer);
            else set_pixel(col, current_line_row, v0.color, buffer);

            cur_line++;
        }

        y += dy;
        col++;
    }
}

// Cuts a polygon into two pieces
// Cut is horizontal and is at y
void cut_polygon(std::vector<Vertex>* polygon, float y, std::vector<Vertex>* top, std::vector<Vertex>* bottom)
{
    // IDEA: polygon MUST have some winding

    for (int i = 0; i < polygon->size(); i++)
    {
        Vertex cur = polygon->at(i);
        float cur_dy = y - cur.device.y;

        // Add current vertex to its respective list(s)
        if (cur_dy >= 0.0f)
        {
            bottom->push_back(cur);
        }
        if (cur_dy <= 0.0f)
        {
            top->push_back(cur);
        }

        Vertex next = polygon->at((i + 1) % polygon->size());
        float next_dy = y - next.device.y;

        // Check if we cross cut line on way to next vertex
        if (((cur_dy < 0.0f) && (next_dy > 0.0f)) || ((cur_dy > 0.0f) && (next_dy < 0.0f)))
        {
            // Interpolate
            float dx_dy = (next.device.x - cur.device.x) / (next.device.y - cur.device.y);
            float dx = cur_dy * dx_dy;
            Vec2f interp_device_coord (cur.device.x + dx, y);

            float dr_dy = (next.color.x - cur.color.x) / (next.device.y - cur.device.y);
            float dg_dy = (next.color.y - cur.color.y) / (next.device.y - cur.device.y);
            float db_dy = (next.color.z - cur.color.z) / (next.device.y - cur.device.y);

            float dr = cur_dy * dr_dy;
            float dg = cur_dy * dg_dy;
            float db = cur_dy * db_dy;

            Vertex interpolated; 
            interpolated.device = interp_device_coord;
            interpolated.color = Vec3f(cur.color.x + dr, cur.color.y + dg, cur.color.z + db);

            // TODO: do interpolation for rest of values!

            top->push_back(interpolated);
            bottom->push_back(interpolated);
        }
    }
}

void rasterize_triangle(Vertex v0, Vertex v1, Vertex v2, Buffer* buffer)
{
    Vertex apex_v, left_v, right_v;
    if (v0.device.y == v1.device.y)
    {
        apex_v = v2;

        if (v0.device.x < v1.device.x)
        {
            left_v = v0;
            right_v = v1;
        }
        else
        {
            left_v = v1;
            right_v = v0;
        }
    }
    else if (v0.device.y == v2.device.y)
    {
        apex_v = v1;

        if (v0.device.x < v2.device.x)
        {
            left_v = v0;
            right_v = v2;
        }
        else
        {
            left_v = v2;
            right_v = v0;
        }
    }
    else // v1.device.y == v2.device.y 
    {
        apex_v = v0;

        if (v1.device.x < v2.device.x)
        {
            left_v = v1;
            right_v = v2;
        }
        else
        {
            left_v = v2;
            right_v = v1;
        }
    }

    // Flat bottom/top triangle, and non-colinear points
    assert(left_v.device.y == right_v.device.y);
    assert(apex_v.device.y != left_v.device.y);

    int stop_scanline = apex_v.device.y > left_v.device.y ? ceil(apex_v.device.y) : ceil(left_v.device.y);

    struct Endpoint
    {
        float x_inc, y_inc, z_inc, r_inc, g_inc, b_inc;
        float x, y, z, r, g, b;
    };

    // *_inc is d*/dy, the derivitive (slope)
    // expect for y_inc, which is the deltaY

    Endpoint left;
    left.x_inc = (apex_v.device.x - left_v.device.x) / (apex_v.device.y - left_v.device.y);
    left.y_inc = 1.0f;
    left.z_inc = (apex_v.depth   - left_v.depth)   / (apex_v.device.y - left_v.device.y);
    left.r_inc = (apex_v.color.x - left_v.color.x) / (apex_v.device.y - left_v.device.y);
    left.g_inc = (apex_v.color.y - left_v.color.y) / (apex_v.device.y - left_v.device.y);
    left.b_inc = (apex_v.color.z - left_v.color.z) / (apex_v.device.y - left_v.device.y);

    if (apex_v.device.y < left_v.device.y)
    {
        left.x = apex_v.device.x + (ceil(apex_v.device.y) - apex_v.device.y) * left.x_inc;
        left.y = apex_v.device.y + (ceil(apex_v.device.y) - apex_v.device.y);
        left.z = apex_v.depth    + (ceil(apex_v.device.y) - apex_v.device.y) * left.z_inc;
        left.r = apex_v.color.x  + (ceil(apex_v.device.y) - apex_v.device.y) * left.r_inc;
        left.g = apex_v.color.y  + (ceil(apex_v.device.y) - apex_v.device.y) * left.g_inc;
        left.b = apex_v.color.z  + (ceil(apex_v.device.y) - apex_v.device.y) * left.b_inc;
    }
    else
    {
        left.x = left_v.device.x + (ceil(left_v.device.y) - left_v.device.y) * left.x_inc;
        left.y = left_v.device.y + (ceil(left_v.device.y) - left_v.device.y);
        left.z = left_v.depth    + (ceil(left_v.device.y) - left_v.device.y) * left.z_inc;
        left.r = left_v.color.x  + (ceil(left_v.device.y) - left_v.device.y) * left.r_inc;
        left.g = left_v.color.y  + (ceil(left_v.device.y) - left_v.device.y) * left.g_inc;
        left.b = left_v.color.z  + (ceil(left_v.device.y) - left_v.device.y) * left.b_inc;
    }

    Endpoint right;
    right.x_inc = (apex_v.device.x - right_v.device.x) / (apex_v.device.y - right_v.device.y);
    right.y_inc = 1.0f;
    right.z_inc = (apex_v.depth   - right_v.depth)   / (apex_v.device.y - right_v.device.y);
    right.r_inc = (apex_v.color.x - right_v.color.x) / (apex_v.device.y - right_v.device.y);
    right.g_inc = (apex_v.color.y - right_v.color.y) / (apex_v.device.y - right_v.device.y);
    right.b_inc = (apex_v.color.z - right_v.color.z) / (apex_v.device.y - right_v.device.y);

    if (apex_v.device.y < right_v.device.y)
    {
        right.x = apex_v.device.x + (ceil(apex_v.device.y) - apex_v.device.y) * right.x_inc;
        right.y = apex_v.device.y + (ceil(apex_v.device.y) - apex_v.device.y);
        right.z = apex_v.depth    + (ceil(apex_v.device.y) - apex_v.device.y) * right.z_inc;
        right.r = apex_v.color.x  + (ceil(apex_v.device.y) - apex_v.device.y) * right.r_inc;
        right.g = apex_v.color.y  + (ceil(apex_v.device.y) - apex_v.device.y) * right.g_inc;
        right.b = apex_v.color.z  + (ceil(apex_v.device.y) - apex_v.device.y) * right.b_inc;
    }
    else
    {
        right.x = right_v.device.x + (ceil(right_v.device.y) - right_v.device.y) * right.x_inc;
        right.y = right_v.device.y + (ceil(right_v.device.y) - right_v.device.y);
        right.z = right_v.depth    + (ceil(right_v.device.y) - right_v.device.y) * right.z_inc;
        right.r = right_v.color.x  + (ceil(right_v.device.y) - right_v.device.y) * right.r_inc;
        right.g = right_v.color.y  + (ceil(right_v.device.y) - right_v.device.y) * right.g_inc;
        right.b = right_v.color.z  + (ceil(right_v.device.y) - right_v.device.y) * right.b_inc;
    }

    assert(left.y  == floor(left.y));  // y should be at integer
    assert(right.y == floor(right.y)); // y should be at integer

    Endpoint scanline;
    scanline.x_inc = 1.0f;
    scanline.y_inc = 0.0f;

    scanline.z_inc = (right_v.depth   - left_v.depth)   / (right_v.device.x - left_v.device.x);
    scanline.r_inc = (right_v.color.x - left_v.color.x) / (right_v.device.x - left_v.device.x);
    scanline.g_inc = (right_v.color.y - left_v.color.y) / (right_v.device.x - left_v.device.x);
    scanline.b_inc = (right_v.color.z - left_v.color.z) / (right_v.device.x - left_v.device.x);

    while (left.y < stop_scanline)
    {
        scanline.x = floor(left.x);
        scanline.y = left.y;
        scanline.z = left.z + (floor(left.x) - left.x) * scanline.z_inc;
        scanline.r = left.r + (floor(left.x) - left.x) * scanline.r_inc;
        scanline.g = left.g + (floor(left.x) - left.x) * scanline.g_inc;
        scanline.b = left.b + (floor(left.x) - left.x) * scanline.b_inc;

        int right_stop = floor(right.x);
        while (scanline.x < right_stop) 
        {
            Vec2i pixel (scanline.x, scanline.y);
            Vec3f color = clampedVec3f(Vec3f(scanline.r, scanline.g, scanline.b), 0.0f, 1.0f);
            set_pixel(pixel.x, pixel.y, color, buffer);

            scanline.x += scanline.x_inc;
            scanline.y += scanline.y_inc;
            scanline.z += scanline.z_inc;
            scanline.r += scanline.r_inc;
            scanline.g += scanline.g_inc;
            scanline.b += scanline.b_inc;
        }

        left.x += left.x_inc;
        left.y += left.y_inc;
        left.z += left.z_inc;
        left.r += left.r_inc;
        left.g += left.g_inc;
        left.b += left.b_inc;

        right.x += right.x_inc;
        right.y += right.y_inc;
        right.z += right.z_inc;
        right.r += right.r_inc;
        right.g += right.g_inc;
        right.b += right.b_inc;
    }
}

// Trapezoid MUST have flat top & bottoms
void rasterize_trapezoid(Vertex v0, Vertex v1, Vertex v2, Vertex v3, Buffer* buffer)
{  
    // IDEA: I think its safe to assume culling for backfaceing traps has already occurred
    // IDEA: raster policy: don't rasterize at or past top edge
    // IDEA: raster policy: don't rasterize at or past right edge

    rasterize_triangle(v0, v1, v2, buffer);
    rasterize_triangle(v2, v3, v0, buffer);
}

void rasterize_polygon(const std::vector<Vertex>& vertices, Buffer* buffer)
{
    // Allocate vectors once
    static std::vector<Vertex>* cur_polygon = new std::vector<Vertex>(15);
    static std::vector<Vertex>* top = new std::vector<Vertex>(15);
    static std::vector<Vertex>* bottom = new std::vector<Vertex>(15);
    cur_polygon->clear();
    top->clear();
    bottom->clear();

    // Process: cut polygon into rasterize-able triangle or trapezoid
    // IDEA: polygon is flat, and convex
    // IDEA: polygon must have some winding

    // IDEA: switch to insertion sort (apparently its faster on smaller lists)
    std::vector<float> sorted_heights (vertices.size());
    for (int i = 0; i < sorted_heights.size(); i++) sorted_heights[i] = vertices[i].device.y;
    std::sort(sorted_heights.begin(), sorted_heights.end());

    for (int i = 0; i < vertices.size(); i++) cur_polygon->push_back(vertices[i]);

    for (int i = 0; i < sorted_heights.size(); i++)
    {
        cut_polygon(cur_polygon, sorted_heights[i], top, bottom);

        // Rasterize bottom piece
        if (bottom->size() == 4)
        {
            rasterize_trapezoid(bottom->at(0), bottom->at(1), bottom->at(2), bottom->at(3), buffer);
        }
        else if (bottom->size() == 3)
        {
            rasterize_triangle(bottom->at(0), bottom->at(1), bottom->at(2), buffer);
        }

        std::swap(cur_polygon, top);
        top->clear();
        bottom->clear();
    }
}