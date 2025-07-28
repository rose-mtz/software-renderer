#include "Rasterize.h"
#include "Geometry.h"
#include <algorithm>
#include <cassert>


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
        int scanline = floor(edge.v.device.y);

        for (int i = 0; i < line_thickness; i++)
        {
            int shift = i + (1 - width);
            int shifted_scanline = shift + scanline;

            if (steep_slope) 
            {
                if (!is_out_of_bounds(shifted_scanline, cur_column, buffer)) set_pixel(shifted_scanline, cur_column, clampedVec3f(edge.v.color, 0.0f, 1.0f), buffer);
            }
            else 
            {
                if (!is_out_of_bounds(cur_column, shifted_scanline, buffer)) set_pixel(cur_column, shifted_scanline, clampedVec3f(edge.v.color, 0.0f, 1.0f), buffer);
            }
        }

        take_step(edge);
        cur_column++;
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
    if      (v0.device.y == v1.device.y) labels = { &v2, &v0, &v1 };    // WARNING: exact float checks is not good idea, but for now I know that results can be exact
    else if (v0.device.y == v2.device.y) labels = { &v1, &v0, &v2 };    //          due to setting exact values before passing to this function
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
        float delta_x = (floor(left.v.device.x) - left.v.device.x);
        scanline_edge.v = left.v;
        take_step(scanline_edge, delta_x);

        Vec2i cur_pixel (floor(left.v.device.x), cur_scanline);

        int right_stop = floor(right.v.device.x);
        while (cur_pixel.x < right_stop)
        {
            set_pixel(cur_pixel.x, cur_pixel.y, clampedVec3f(scanline_edge.v.color, 0.0f, 1.0f), buffer);

            cur_pixel.x++;
            take_step(scanline_edge);
        }

        take_step(left);
        right.v.device.x += right.v_inc.device.x; // only need to calculate x value
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