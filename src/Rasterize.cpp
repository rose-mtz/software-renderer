#include "Util.h"
#include "Rasterize.h"
#include "Geometry.h"
#include <algorithm>
#include <cassert>

struct Fragment
{
    Vec2i pixel;
    Vec3f color;
    float opacity;
    float depth;
};

void set_fragment(Fragment& frag, Buffer* color_buffer, Buffer* depth_buffer)
{
    assert(frag.color.x != 0.0f || frag.color.y != 0.0f || frag.color.z != 0.0f);
    bool is_out_of_bounds = (frag.pixel.x < 0 || frag.pixel.x >= color_buffer->width) || (frag.pixel.y < 0 || frag.pixel.y >= color_buffer->height);
    float depth; get_element(frag.pixel.x, frag.pixel.y, &depth, depth_buffer);
    bool is_hidden = depth > frag.depth;

    if (!is_out_of_bounds && !is_hidden)
    {
        set_element(frag.pixel.x, frag.pixel.y, frag.color.raw, color_buffer);
        set_element(frag.pixel.x, frag.pixel.y, &frag.depth, depth_buffer);
    }
}

// For cutting polygons
// For checking if two device.y's are the same
const float EPSILON = 0.001f;

void rasterize_point(const Vertex& v, int radius, Buffer* color_buffer, Buffer* depth_buffer)
{
    // BUG: radius is device-resolution dependent
    //      high-res buffers will have small points
    //      low-res buffers will have big points

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
            Fragment frag;
            frag.color = v.color;
            frag.pixel = Vec2i(cur_column, cur_scanline);
            frag.depth = v.depth;
            set_fragment(frag, color_buffer, depth_buffer);

            cur_column++;
        }

        cur_scanline++;
    }
}

void rasterize_line(const Vertex& v0, const Vertex& v1, int width, Buffer* color_buffer, Buffer* depth_buffer)
{
    // BUG: width is device-resolution dependent
    //      high-res buffers will have small lines
    //      low-res buffers will have big lines

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

            Fragment frag;
            frag.color = clampedVec3f(edge.v.color, 0.0f, 1.0f);
            frag.depth = edge.v.depth;
            if (steep_slope) 
            {
                frag.pixel = Vec2i(shifted_scanline, cur_column);
            }
            else
            {
                frag.pixel = Vec2i(cur_column, shifted_scanline);
            }
            set_fragment(frag, color_buffer, depth_buffer);
        }

        take_step(edge);
        cur_column++;
    }
}

void rasterize_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Buffer* color_buffer, Buffer* depth_buffer, Buffer* texture)
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

    // NOTE: triangle rasterization process can sample points outside of the triangle
    //       this can lead to unexpected values for the interpolated vertex
    //       like out-of-bounds uvs, thus this must be dealt
    
    if (std::abs((v1.device - v0.device) ^ (v2.device - v0.device))/2.0f < 1.0f) return;

    struct TriangleVertexLabels { const Vertex *apex, *left, *right; } labels;
    // Set the apex
    if      (std::abs(v0.device.y - v1.device.y) < EPSILON) labels = { &v2, &v0, &v1 };
    else if (std::abs(v0.device.y - v2.device.y) < EPSILON) labels = { &v1, &v0, &v2 };
    else                                 labels = { &v0, &v1, &v2 };
    // Swap left right to correct order
    if (labels.left->device.x > labels.right->device.x) std::swap(labels.left, labels.right);

    // INPUT DATA SANITY CHECK: flat top/bottom triangle
    assert(std::abs(labels.left->device.y - labels.right->device.y) < EPSILON);

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
            Fragment frag;
            sample_bilinear(clampf(scanline_edge.v.uv.x, 0.0f, 1.0f), clampf(scanline_edge.v.uv.y, 0.0f, 1.0f), frag.color.raw, texture);
            frag.pixel = Vec2i(cur_pixel.x, cur_pixel.y);
            frag.depth = scanline_edge.v.depth;
            set_fragment(frag, color_buffer, depth_buffer);

            cur_pixel.x++;
            take_step(scanline_edge);
        }

        take_step(left);
        right.v.device.x += right.v_inc.device.x; // only need to calculate x value
        cur_scanline++;
    }
}

// Polygon is assumed 'flat' (in all dimension)
// Polygon must have some winding
void rasterize_polygon(const std::vector<Vertex>& vertices, Buffer* color_buffer, Buffer* depth_buffer, Buffer* texture)
{
    // ROBUSTNESS: degenerate polygon check?

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
        Plane plane { 0.0f, 1.0f, 0.0f, -sorted_heights[i] };
        cull_polygon(cur_polygon, plane, top, bottom, EPSILON);

        if (bottom.size() == 4)
        {
            rasterize_triangle(bottom[0], bottom[1], bottom[2], color_buffer, depth_buffer, texture);
            rasterize_triangle(bottom[2], bottom[3], bottom[0], color_buffer, depth_buffer, texture);
        }
        else if (bottom.size() == 3)
        {
            rasterize_triangle(bottom[0], bottom[1], bottom[2], color_buffer, depth_buffer, texture);
        }

        std::swap(cur_polygon, top);
        top.clear();
        bottom.clear();
    }
}
