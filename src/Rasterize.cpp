#include "Rasterize.h"
#include <algorithm>
#include <cassert>

// TODO: stop using exact comparisons for floats

void rasterize_point(const Vertex& v, int radius, Buffer* buffer)
{
    /**
     * PROCESS:
     * 
     * for each scanline circle spans
     *      calculate scanline-circle interceptions
     *      rasterize between interception points
     * 
     * ISSUES:
     * 
     * Big points span many scanlines, but small points span 
     * only a few scanlines. The problem with this is that the number of scanlines determines 
     * how many times scanline-circle interceptions are calculated. The more interceptions calculated 
     * the more the rasterized point will look like a circle, but the less the more blocky it will look. 
     */

    float radius_squared = radius*radius;
    Vec2i center (round(v.device.x), round(v.device.y));
    int stop_scanline = center.y + radius;
    int start_scanline = center.y - radius;

    int cur_scanline = start_scanline;
    while (cur_scanline < stop_scanline)
    {
        int y_intercept = cur_scanline;
        // for bottom part of circle, we use scanline above current to sample circle intercepts
        if (cur_scanline < center.y) y_intercept++; 

        // Relative to circle center (essential move circle to origin)
        int   y_intercept_relative     = y_intercept - center.y;
        float right_intercept_relative = sqrt(radius_squared - y_intercept_relative*y_intercept_relative);
        float left_intercept_relative  = -right_intercept_relative;
        // Translates intercepts back
        float left_intercept  = left_intercept_relative  + center.x;
        float right_intercept = right_intercept_relative + center.x;

        int cur_column  = floor(left_intercept);
        int stop_column = ceil(right_intercept);

        while (cur_column < stop_column)
        {
            if (!is_out_of_bounds(cur_column, cur_scanline, buffer)) set_pixel(cur_column, cur_scanline, v.color, buffer);
            cur_column++;
        }

        cur_scanline++;
    }
}

void rasterize_line(const Vertex& v0, const Vertex& v1, int width, Buffer* buffer)
{
    /**
     * PROCESS:
     * 
     * if line slope is steep 
     *      invert axis
     * 
     * set up edge tracker
     * 
     * for each column line spans
     *      rasterize at column
     *      
     *      march one step forward on edge
     */

    Vertex start = v0;
    Vertex end   = v1;

    float slope;
    bool steep_slope = (fabs(end.device.y - start.device.y) > fabs(end.device.x - start.device.x));
    if (steep_slope)
    {
        // For steep slope cases, invert the axis (swap x and y)
        assert((end.device.y - start.device.y) != 0.0f);
        slope = (end.device.x - start.device.x) / (end.device.y - start.device.y);
        std::swap(start.device.x, start.device.y);
        std::swap(end.device.x, end.device.y);
    }
    else
    {
        assert((end.device.x - start.device.x) != 0.0f);
        slope =  (end.device.y - start.device.y) / (end.device.x - start.device.x);
    }

    // March line in increasing x-direction
    if (start.device.x > end.device.x) std::swap(start, end);

    EdgeTracker edge = set_up_edge_tracker(start, end, false);

    int cur_column = floor(start.device.x);
    int final_column = ceil(end.device.x);
    int line_thickness = 1 + ((width - 1) * 2);

    while (cur_column < final_column)
    {
        int scanline = floor(edge.y);

        for (int i = 0; i < line_thickness; i++)
        {
            int shift = i + (1 - width);
            int shifted_scanline = shift + scanline;

            if (steep_slope) 
            {
                if (!is_out_of_bounds(shifted_scanline, cur_column, buffer)) set_pixel(shifted_scanline, cur_column, clampedVec3f(Vec3f(edge.r, edge.g, edge.b), 0.0f, 1.0f), buffer);
            }
            else 
            {
                if (!is_out_of_bounds(cur_column, shifted_scanline, buffer)) set_pixel(cur_column, shifted_scanline, clampedVec3f(Vec3f(edge.r, edge.g, edge.b), 0.0f, 1.0f), buffer);
            }
        }

        take_step(edge);
        cur_column++;
    }
}

// Cuts a polygon into two pieces
// Cut is horizontal and is at y
// Polygon MUST have some winding
void cut_polygon(const std::vector<Vertex>& polygon, float y, std::vector<Vertex>& top, std::vector<Vertex>& bottom)
{
    for (int i = 0; i < polygon.size(); i++)
    {
        const Vertex& cur = polygon[i];
        float cur_dy = y - cur.device.y; // relative to cut line

        // Add current vertex to its respective list(s)
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

        // Check if we cross cut line on way to next vertex
        if (((cur_dy < 0.0f) && (next_dy > 0.0f)) || ((cur_dy > 0.0f) && (next_dy < 0.0f)))
        {
            assert(next.device.y != cur.device.y);

            EdgeTracker edge = set_up_edge_tracker(cur, next, true);
            take_step(edge, cur_dy);
            Vertex interp_vertex = get_vertex(edge);
            // Triangle rasterizer expects exact value (no floating point rounding)
            interp_vertex.device.y = y;

            top.push_back(interp_vertex);
            bottom.push_back(interp_vertex);
        }
    }
}

void rasterize_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Buffer* buffer)
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
    
    // Raster policy: don't rasterize at or past top edge
    // Raster policy: don't rasterize at or past right edge

    // BUG: 3 colinear vertices is causing issues
    //      How to handle this?
    //      Only seen on horizontal colinear vertices, not sure about vertical
    // PROBLEM: doing exact checks like == with floats is likely bad idea
    //          need to figure out something else
    // IDEA: should I make my rasterizer more robust, even at the cost of perf?
    //       if tiny perf consequences then sure, I can always remove them later, if needed 

    struct TriangleVertexLabels { const Vertex *apex, *left, *right; } labels;
    // Set the apex
    if      (v0.device.y == v1.device.y) labels = { &v2, &v0, &v1 };
    else if (v0.device.y == v2.device.y) labels = { &v1, &v0, &v2 };
    else                                 labels = { &v0, &v1, &v2 };
    // Swap left right to correct order
    if (labels.left->device.x > labels.right->device.x) std::swap(labels.left, labels.right);

    // INPUT DATA SANITY CHECK: flat top/bottom triangle
    assert(labels.left->device.y == labels.right->device.y);
    // INPUT DATA SANITY CHECK: non-colinear points
    assert(labels.apex->device.y != labels.left->device.y);
    assert(labels.right->device.x != labels.left->device.x);

    EdgeTracker left;
    EdgeTracker right;

    int start_scanline;
    bool is_apex_above_other_vertices = labels.apex->device.y > labels.left->device.y;
    float delta_y;
    if (is_apex_above_other_vertices)
    {
        delta_y  = (ceil(labels.left->device.y) - labels.left->device.y); // same for both edges
        left = set_up_edge_tracker(*labels.left, *labels.apex, true);
        right = set_up_edge_tracker(*labels.right, *labels.apex, true);
        start_scanline = ceil(labels.left->device.y);
    }
    else
    {
        delta_y  = (ceil(labels.apex->device.y) - labels.apex->device.y);
        left = set_up_edge_tracker(*labels.apex, *labels.left, true);
        right = set_up_edge_tracker(*labels.apex, *labels.right, true);        
        start_scanline = ceil(labels.apex->device.y);
    }

    // Initial step to get on scanline
    take_step(left, delta_y);
    take_step(right, delta_y);
    int cur_scanline = start_scanline;

    EdgeTracker scanline_edge = set_up_edge_tracker(*labels.left, *labels.right, false);
    int stop_scanline = is_apex_above_other_vertices ? ceil(labels.apex->device.y) : ceil(labels.left->device.y);

    while (cur_scanline < stop_scanline)
    {
        float delta_x = (floor(left.x) - left.x);
        // TEMPORARY
        scanline_edge.z = left.z + delta_x * scanline_edge.z_inc;
        scanline_edge.r = left.r + delta_x * scanline_edge.r_inc;
        scanline_edge.g = left.g + delta_x * scanline_edge.g_inc;
        scanline_edge.b = left.b + delta_x * scanline_edge.b_inc;
        Vec2i cur_pixel (floor(left.x), cur_scanline);

        int right_stop = floor(right.x);
        while (cur_pixel.x < right_stop)
        {
            set_pixel(cur_pixel.x, cur_pixel.y, clampedVec3f(Vec3f(scanline_edge.r, scanline_edge.g, scanline_edge.b), 0.0f, 1.0f), buffer);

            cur_pixel.x++;
            take_step(scanline_edge);
        }

        take_step(left);
        take_step(right); // OPTIMIZE: really only need the right.x value, don't need to interpolate everything else
        cur_scanline++;
    }
}

// Trapezoid MUST have flat top & bottoms
void rasterize_trapezoid(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3, Buffer* buffer)
{
    rasterize_triangle(v0, v1, v2, buffer);
    rasterize_triangle(v2, v3, v0, buffer);
}

// Polygon is assumed 'flat' (in all dimension)
// Polygon must have some winding
void rasterize_polygon(const std::vector<Vertex>& vertices, Buffer* buffer)
{
    // Allocate vectors once
    static std::vector<Vertex> cur_polygon (15);
    static std::vector<Vertex> top (15);
    static std::vector<Vertex> bottom (15);
    cur_polygon.clear();
    top.clear();
    bottom.clear();

    // TODO: switch to insertion sort (apparently its faster on smaller lists)
    std::vector<float> sorted_heights (vertices.size());
    for (int i = 0; i < sorted_heights.size(); i++) sorted_heights[i] = vertices[i].device.y;
    std::sort(sorted_heights.begin(), sorted_heights.end());

    for (int i = 0; i < vertices.size(); i++) cur_polygon.push_back(vertices[i]);

    for (int i = 0; i < sorted_heights.size(); i++)
    {
        cut_polygon(cur_polygon, sorted_heights[i], top, bottom);

        // Rasterize bottom piece
        if (bottom.size() == 4)
        {
            rasterize_trapezoid(bottom[0], bottom[1], bottom[2], bottom[3], buffer);
        }
        else if (bottom.size() == 3)
        {
            rasterize_triangle(bottom[0], bottom[1], bottom[2], buffer);
        }

        std::swap(cur_polygon, top);
        top.clear();
        bottom.clear();
    }
}