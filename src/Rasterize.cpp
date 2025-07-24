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

void rasterize_triangle(Vertex& v0, Vertex& v1, Vertex& v2, Buffer* buffer)
// void rasterize_triangle(std::vector<Vertex> vertices, Buffer* buffer)
{
    /**
     * PROCESS:
     * 
     * label triangle vertices
     *      apex endpoint, left endpoint, right endpoint
     * 
     * set up left edge tracker
     * set up right edge tracker
     * 
     * for each scanline triangle spans
     *      set up current scanline
     *      rasterize scanline from left to right
     * 
     *      take step forward on left edge tracker
     *      take step forward on right edge tracker
     */

    // INPUT DATA SANITY CHECK: 3 vertex
    // assert(vertices.size() == 3);

    struct TriangleVertexLabels { Vertex *apex, *left, *right; } labels;
    if (v0.device.y == v1.device.y)
    {
        labels.apex = &v2;
        labels.left  = &v0;
        labels.right = &v1;
    }
    else if (v0.device.y == v2.device.y)
    {
        labels.apex = &v1;
        labels.left  = &v0;
        labels.right = &v2;
    }
    else
    {
        labels.apex = &v0;
        labels.left = &v1;
        labels.right = &v2;
    }
    if (labels.left->device.x > labels.right->device.x) std::swap(labels.left, labels.right);

    // INPUT DATA SANITY CHECK: flat top/bottom triangle
    assert(labels.left->device.y == labels.right->device.y);

    bool is_apex_above_other_vertices = labels.apex->device.y > labels.left->device.y;

    struct EdgeTracker
    {
        float x_inc, y_inc, z_inc, r_inc, g_inc, b_inc;
        float x, y, z, r, g, b;
        int x_int, y_int; // use instead of float x, y when known integer math
    };

    // *_inc is d*/dy, the derivitive (slope)
    // expect for y_inc, which is the deltaY

    float one_over_delta_y = 1.0f / (labels.apex->device.y - labels.left->device.y); // same for both edges (left.y == right.y)

    EdgeTracker left;
    left.y_inc = 1.0f;
    left.x_inc = (labels.apex->device.x - labels.left->device.x) * one_over_delta_y;
    left.z_inc = (labels.apex->depth    - labels.left->depth   ) * one_over_delta_y;
    left.r_inc = (labels.apex->color.x  - labels.left->color.x ) * one_over_delta_y;
    left.g_inc = (labels.apex->color.y  - labels.left->color.y ) * one_over_delta_y;
    left.b_inc = (labels.apex->color.z  - labels.left->color.z ) * one_over_delta_y;

    EdgeTracker right;
    right.y_inc = 1.0f;
    right.x_inc = (labels.apex->device.x - labels.right->device.x) * one_over_delta_y;
    right.z_inc = (labels.apex->depth    - labels.right->depth   ) * one_over_delta_y;
    right.r_inc = (labels.apex->color.x  - labels.right->color.x ) * one_over_delta_y;
    right.g_inc = (labels.apex->color.y  - labels.right->color.y ) * one_over_delta_y;
    right.b_inc = (labels.apex->color.z  - labels.right->color.z ) * one_over_delta_y;


    if (is_apex_above_other_vertices)
    {
        float delta_y = (ceil(labels.left->device.y) - labels.left->device.y); // same for both edges (left.y == right.y)

        left.x = labels.left->device.x + (delta_y * left.x_inc);
        left.y = labels.left->device.y + (delta_y             );
        left.z = labels.left->depth    + (delta_y * left.z_inc);
        left.r = labels.left->color.x  + (delta_y * left.r_inc);
        left.g = labels.left->color.y  + (delta_y * left.g_inc);
        left.b = labels.left->color.z  + (delta_y * left.b_inc);

        right.x = labels.right->device.x + (delta_y * right.x_inc);
        right.y = labels.right->device.y + (delta_y              );
        right.z = labels.right->depth    + (delta_y * right.z_inc);
        right.r = labels.right->color.x  + (delta_y * right.r_inc);
        right.g = labels.right->color.y  + (delta_y * right.g_inc);
        right.b = labels.right->color.z  + (delta_y * right.b_inc);
    }
    else
    {
        float delta_y = (ceil(labels.apex->device.y) - labels.apex->device.y);

        left.x = labels.apex->device.x + (delta_y * left.x_inc);
        left.y = labels.apex->device.y + (delta_y             );
        left.z = labels.apex->depth    + (delta_y * left.z_inc);
        left.r = labels.apex->color.x  + (delta_y * left.r_inc);
        left.g = labels.apex->color.y  + (delta_y * left.g_inc);
        left.b = labels.apex->color.z  + (delta_y * left.b_inc);

        right.x = labels.apex->device.x + (delta_y * right.x_inc);
        right.y = labels.apex->device.y + (delta_y              );
        right.z = labels.apex->depth    + (delta_y * right.z_inc);
        right.r = labels.apex->color.x  + (delta_y * right.r_inc);
        right.g = labels.apex->color.y  + (delta_y * right.g_inc);
        right.b = labels.apex->color.z  + (delta_y * right.b_inc);
    }

    assert(left.y  == floor(left.y));  // y should be at integer
    assert(right.y == floor(right.y)); // y should be at integer

    float one_over_delta_x = 1.0f / (labels.right->device.x - labels.left->device.x);

    EdgeTracker scanline;
    scanline.x_inc = 1.0f;
    scanline.y_inc = 0.0f;
    scanline.z_inc = (labels.right->depth   - labels.left->depth  ) * one_over_delta_x;
    scanline.r_inc = (labels.right->color.x - labels.left->color.x) * one_over_delta_x;
    scanline.g_inc = (labels.right->color.y - labels.left->color.y) * one_over_delta_x;
    scanline.b_inc = (labels.right->color.z - labels.left->color.z) * one_over_delta_x;

    int stop_scanline = is_apex_above_other_vertices ? ceil(labels.apex->device.y) : ceil(labels.left->device.y);
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
void rasterize_trapezoid(Vertex& v0, Vertex& v1, Vertex& v2, Vertex& v3, Buffer* buffer)
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