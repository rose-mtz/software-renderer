#include "Rasterize.h"
#include <algorithm>

void rasterize_point(Vec2f point, Vec3f color, int width, Buffer* buffer)
{
    // Assumes point is in device coords, and is inside bounds.
    // Ignoring depth, and AA for now
    
    // Width can be 2x2, 4x4, 6x6, ...
    width = (width) * 2;

    // Center of point
    int center_x = point.x; // round down
    int center_y = point.y; // round down

    // Bottom left of point
    int bottom_left_x  = center_x - width/2;
    int bottom_left_y  = center_y - width/2;

    for (int i = bottom_left_y; i < bottom_left_y + width; i++)
    {
        if (i < 0 || i >= buffer->height) continue; // out of bounds fragment

        for (int j = bottom_left_x; j < bottom_left_x + width; j++)
        {
            if (j < 0 || j >= buffer->width) continue; // out of bounds fragment

            int index = i * buffer->width + j;
            buffer->pixels[index] = color;
        }
    }
}

void rasterize_line(Vec2f point_1, Vec2f point_2, Vec3f color, int width, Buffer* buffer)
{
    // Each increase in width adds 1 layer of 'thickness' (i.e two lines on each side)
    int line_count = 1 + (width - 1) * 2;

    float m = (point_2.y - point_1.y) / (point_2.x - point_1.x);
    float inc_x;
    float inc_y;
    float steep_slope = m > 1.0f || m < -1.0f;
 
    if (!steep_slope)
    {
        // March in increasing x-direction
        if (point_1.x > point_2.x) 
        {
            Vec2f temp = point_1;
            point_1 = point_2;
            point_2 = temp;
        }   

        inc_x = 1.0f;
        inc_y = m;
    }
    else
    {
        // March in increasing y-direction
        if (point_1.y > point_2.y) 
        {
            Vec2f temp = point_1;
            point_1 = point_2;
            point_2 = temp;
        }

        inc_y = 1.0f;
        inc_x = 1.0f/m;   
    }

    Vec2f cur_point = point_1;
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
                buffer->pixels[index] = color;
            }
            else
            {
                int current_line_x = (rounded_x - (width - 1)) + i;
                if (current_line_x < 0 || current_line_x >= buffer->width) continue; // out of bounds fragment

                int index = current_line_x + rounded_y * buffer->width;
                buffer->pixels[index] = color;
            }
        }

        cur_point.x += inc_x;
        cur_point.y += inc_y;

        // March ends when past point_2
        if ((steep_slope && cur_point.y > point_2.y) || (!steep_slope && cur_point.x > point_2.x)) // IDEA: BUG: point_2.x/y = right/top edge of screen
        {
            break;
        }
    }
}

// Trapezoid MUST have flat top & bottoms
void rasterize_trapezoid(Vec2f p1, Vec2f p2, Vec2f p3, Vec2f p4, Vec3f color, Buffer* buffer)
{  
    // IDEA: I think its safe to assume culling for backfaceing traps has already occurred
    // IDEA: raster policy: don't rasterize at or past top edge
    // IDEA: raster policy: don't rasterize at or past right edge

    // Sort by y (bubble sort)
    if (p1.y > p2.y) std::swap(p1,p2);
    if (p2.y > p3.y) std::swap(p2,p3);
    if (p3.y > p4.y) std::swap(p3,p4);
    if (p1.y > p2.y) std::swap(p1,p2);
    if (p2.y > p3.y) std::swap(p2,p3);
    if (p1.y > p2.y) std::swap(p1,p2);

    // Sort by x
    if (p1.x > p2.x) std::swap(p1,p2);
    if (p3.x > p4.x) std::swap(p3,p4);

    //  p1 is bottom left
    //  p2 is bottom right
    //  p3 is top left
    //  p4 is top right

    float dx_dy_l = (p3.x - p1.x) / (p3.y - p1.y);
    float dx_dy_r = (p4.x - p2.x) / (p4.y - p2.y);

    float delta_y = 1.0f;

    float delta_h_l = ceil(p1.y) - p1.y;
    float delta_w_l = delta_h_l * dx_dy_l;
    Vec2f cur_p_l (p1.x + delta_w_l, p1.y + delta_h_l);

    float delta_h_r = ceil(p2.y) - p2.y;
    float delta_w_r = delta_h_r * dx_dy_r;
    Vec2f cur_p_r (p2.x + delta_w_r, p2.y + delta_h_r);

    while (cur_p_l.y < p3.y) // March up sides of trapezoid
    {
        int pixel_x = cur_p_l.x; // floor
        int pixel_y = cur_p_l.y; // floor, but should already be integer (decimal part is all zeros)
        int index = pixel_x + pixel_y * buffer->width;

        while (pixel_x < cur_p_r.x) // March left to right across trapezoid
        {
            buffer->pixels[index] = color;

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
std::vector<std::vector<Vec2f>> cut_polygon(std::vector<Vec2f> polygon, float y)
{
    // IDEA: polygon MUST have some winding

    std::vector<std::vector<Vec2f>> to_return (2);
    std::vector<Vec2f>& top = to_return[0];
    std::vector<Vec2f>& bottom = to_return[1];

    for (int i = 0; i < polygon.size(); i++)
    {
        Vec2f cur = polygon[i];
        float cur_dy = y - cur.y;

        // Add current vertex to its respective list(s)
        if (cur_dy >= 0.0f)
        {
            bottom.push_back(cur);
        }
        if (cur_dy <= 0.0f)
        {
            top.push_back(cur);
        }

        Vec2f next = polygon[(i + 1) % polygon.size()];
        float next_dy = y - next.y;

        // Check if we cross cut line on way to next vertex
        if (((cur_dy < 0.0f) && (next_dy > 0.0f)) || ((cur_dy > 0.0f) && (next_dy < 0.0f)))
        {
            // Interpolate
            float dx_dy = (next.x - cur.x) / (next.y - cur.y);
            float dx = cur_dy * dx_dy;
            Vec2f interpolated (cur.x + dx, y);

            top.push_back(interpolated);
            bottom.push_back(interpolated);
        }
    }

    return to_return;
}

void rasterize_triangle(Vec2f p1, Vec2f p2, Vec2f p3, Vec3f color, Buffer* buffer)
{
    // for now I'll just pass it over to rasterize trapezoid,
    // so triangles must have a flat top/bottom

    // IDEA: I need two top points, and two bottoms points for rasterizing a trapezoid
    if (p1.y == p2.y)
    {
        rasterize_trapezoid(p1, p2, p3, p3, color, buffer);
    }
    else if (p1.y == p3.y)
    {
        rasterize_trapezoid(p1, p2, p3, p2, color, buffer);
    }
    else
    {
        rasterize_trapezoid(p1, p2, p3, p1, color, buffer);
    }
}

void rasterize_polygon(std::vector<Vec2f> polygon, Buffer* buffer)
{
    // Process: cut polygon into rasterizable triangle or trapezoid
    // IDEA: polygon is flat, and convex
    // IDEA: polygon must have some winding

    // IDEA: switch to insertion sort 
    std::vector<float> sorted_heights (polygon.size());
    for (int i = 0; i < sorted_heights.size(); i++) sorted_heights[i] = polygon[i].y;
    std::sort(sorted_heights.begin(), sorted_heights.end());

    std::vector<Vec2f> cur_polygon = polygon;
    for (int i = 0; i < sorted_heights.size(); i++)
    {
        std::vector<std::vector<Vec2f>> split_poly = cut_polygon(cur_polygon, sorted_heights[i]);

        // Rasterize bottom piece (if its triangle or trapezoid)
        if (split_poly[1].size() == 4)
        {
            rasterize_trapezoid(split_poly[1][0], split_poly[1][1], split_poly[1][2], split_poly[1][3], Vec3f(1.0f, 1.0f, 1.0f), buffer);
        }
        else if (split_poly[1].size() == 3)
        {
            rasterize_triangle(split_poly[1][0], split_poly[1][1], split_poly[1][2], Vec3f(1.0f, 1.0f, 1.0f), buffer);
        }

        // Keep top piece of polygon
        cur_polygon = split_poly[0];
    }
}