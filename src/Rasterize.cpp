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

    // Each increase in width adds 1 layer of 'thickness' (i.e two lines on each side)
    int line_count = 1 + (width - 1) * 2;

    float m = (v1.device.y - v0.device.y) / (v1.device.x - v0.device.x);
    float inc_x;
    float inc_y;
    float steep_slope = m > 1.0f || m < -1.0f;
 
    if (!steep_slope)
    {
        // March in increasing x-direction
        if (v0.device.x > v1.device.x) 
        {
            std::swap(v0, v1);
        }   

        inc_x = 1.0f;
        inc_y = m;
    }
    else
    {
        // March in increasing y-direction
        if (v0.device.y > v1.device.y) 
        {
            std::swap(v0, v1);
        }

        inc_y = 1.0f;
        inc_x = 1.0f/m;   
    }

    Vec2f cur_point = v0.device;
    while (true) // March line
    {
        int rounded_x = std::round(cur_point.x);
        int rounded_y = std::round(cur_point.y);

        for (int i = 0; i < line_count; i++) // thickness
        {
            if (!steep_slope)
            {
                int current_line_y = (rounded_y - (width - 1)) + i;
                if (current_line_y < 0 || current_line_y >= buffer->height) continue; // out of bounds fragment

                int index = rounded_x + current_line_y * buffer->width;
                buffer->pixels[index] = v0.color; // temporary, needs interpolation
            }
            else
            {
                int current_line_x = (rounded_x - (width - 1)) + i;
                if (current_line_x < 0 || current_line_x >= buffer->width) continue; // out of bounds fragment

                int index = current_line_x + rounded_y * buffer->width;
                buffer->pixels[index] = v0.color; // temporary, needs interpolation
            }
        }

        cur_point.x += inc_x;
        cur_point.y += inc_y;

        // March ends when past point_2
        if ((steep_slope && cur_point.y > v1.device.y) || (!steep_slope && cur_point.x > v1.device.x)) // IDEA: BUG: point_2.x/y = right/top edge of screen
        {
            break;
        }
    }
}

// Trapezoid MUST have flat top & bottoms
void rasterize_trapezoid(Vertex v0, Vertex v1, Vertex v2, Vertex v3, Buffer* buffer)
{  
    // IDEA: I think its safe to assume culling for backfaceing traps has already occurred
    // IDEA: raster policy: don't rasterize at or past top edge
    // IDEA: raster policy: don't rasterize at or past right edge

    // Sort by y (bubble sort)
    if (v0.device.y > v1.device.y) std::swap(v0,v1);
    if (v1.device.y > v2.device.y) std::swap(v1,v2);
    if (v2.device.y > v3.device.y) std::swap(v2,v3);
    if (v0.device.y > v1.device.y) std::swap(v0,v1);
    if (v1.device.y > v2.device.y) std::swap(v1,v2);
    if (v0.device.y > v1.device.y) std::swap(v0,v1);

    // Sort by x
    if (v0.device.x > v1.device.x) std::swap(v0,v1);
    if (v2.device.x > v3.device.x) std::swap(v2,v3);

    //  p1 is bottom left
    //  p2 is bottom right
    //  p3 is top left
    //  p4 is top right

    float dx_dy_l = (v2.device.x - v0.device.x) / (v2.device.y - v0.device.y);
    float dx_dy_r = (v3.device.x - v1.device.x) / (v3.device.y - v1.device.y);

    float delta_y = 1.0f;

    float delta_h_l = ceil(v0.device.y) - v0.device.y;
    float delta_w_l = delta_h_l * dx_dy_l;
    Vec2f cur_p_l (v0.device.x + delta_w_l, v0.device.y + delta_h_l);

    float delta_h_r = ceil(v1.device.y) - v1.device.y;
    float delta_w_r = delta_h_r * dx_dy_r;
    Vec2f cur_p_r (v1.device.x + delta_w_r, v1.device.y + delta_h_r);

    while (cur_p_l.y < v2.device.y) // March up sides of trapezoid
    {
        int pixel_x = cur_p_l.x; // floor
        int pixel_y = cur_p_l.y; // floor, but should already be integer (decimal part is all zeros)
        int index = pixel_x + pixel_y * buffer->width;

        while (pixel_x < cur_p_r.x) // March left to right across trapezoid
        {
            buffer->pixels[index] = v0.color; // temporary, needs interpolation

            pixel_x++;
            index++;
        }

        cur_p_l.x += dx_dy_l;
        cur_p_l.y += delta_y;
        
        cur_p_r.x += dx_dy_r;
        cur_p_r.y += delta_y;
    }
}

// Cuts a polygon into two pieces
// polygon above cut line is 0
// polygon bellow cut line is 1
std::vector<std::vector<Vertex>> cut_polygon(std::vector<Vertex> polygon, float y)
{
    // IDEA: polygon MUST have some winding

    std::vector<std::vector<Vertex>> to_return (2);
    std::vector<Vertex>& top = to_return[0];
    std::vector<Vertex>& bottom = to_return[1];

    for (int i = 0; i < polygon.size(); i++)
    {
        Vertex cur = polygon[i];
        float cur_dy = y - cur.device.y;

        // Add current vertex to its respective list(s)
        if (cur_dy >= 0.0f)
        {
            bottom.push_back(cur);
        }
        if (cur_dy <= 0.0f)
        {
            top.push_back(cur);
        }

        Vertex next = polygon[(i + 1) % polygon.size()];
        float next_dy = y - next.device.y;

        // Check if we cross cut line on way to next vertex
        if (((cur_dy < 0.0f) && (next_dy > 0.0f)) || ((cur_dy > 0.0f) && (next_dy < 0.0f)))
        {
            // Interpolate
            float dx_dy = (next.device.x - cur.device.x) / (next.device.y - cur.device.y);
            float dx = cur_dy * dx_dy;
            Vec2f interp_device_coord (cur.device.x + dx, y);

            Vertex interpolated; 
            interpolated.device = interp_device_coord;
            interpolated.color = cur.color; // temporary, needs interpolation

            top.push_back(interpolated);
            bottom.push_back(interpolated);
        }
    }

    return to_return;
}

void rasterize_triangle(Vertex v0, Vertex v1, Vertex v2, Buffer* buffer)
{
    // for now I'll just pass it over to rasterize trapezoid,
    // so triangles must have a flat top/bottom

    // IDEA: I need two top points, and two bottoms points for rasterizing a trapezoid
    if (v0.device.y == v1.device.y)
    {
        rasterize_trapezoid(v0, v1, v2, v2, buffer);
    }
    else if (v0.device.y == v2.device.y)
    {
        rasterize_trapezoid(v0, v1, v2, v1, buffer);
    }
    else
    {
        rasterize_trapezoid(v0, v1, v2, v0, buffer);
    }
}

void rasterize_polygon(std::vector<Vertex> vertices, Buffer* buffer)
{
    // Process: cut polygon into rasterize-able triangle or trapezoid
    // IDEA: polygon is flat, and convex
    // IDEA: polygon must have some winding

    // IDEA: switch to insertion sort 
    std::vector<float> sorted_heights (vertices.size());
    for (int i = 0; i < sorted_heights.size(); i++) sorted_heights[i] = vertices[i].device.y;
    std::sort(sorted_heights.begin(), sorted_heights.end());

    std::vector<Vertex> cur_polygon = vertices;
    for (int i = 0; i < sorted_heights.size(); i++)
    {
        std::vector<std::vector<Vertex>> split_poly = cut_polygon(cur_polygon, sorted_heights[i]);

        // Rasterize bottom piece (if its triangle or trapezoid)
        if (split_poly[1].size() == 4)
        {
            rasterize_trapezoid(split_poly[1][0], split_poly[1][1], split_poly[1][2], split_poly[1][3], buffer);
        }
        else if (split_poly[1].size() == 3)
        {
            rasterize_triangle(split_poly[1][0], split_poly[1][1], split_poly[1][2], buffer);
        }

        // Keep top piece of polygon
        cur_polygon = split_poly[0];
    }
}