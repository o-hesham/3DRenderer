#include <math.h>
#include "Mesh/triangle.h"
#include "Graphics/display.h"

#include "Lighting/light.h"
#include "swap.h"

///////////////////////////////////////////////////////////////////////////////
// Return the normal vector of a triangle face
///////////////////////////////////////////////////////////////////////////////
vec3_t get_triangle_normal(vec4_t vertices[3])
{
    // Get individual vectors from A, B, and C vertices to compute normal
    vec3_t vector_a = vec3_from_vec4(vertices[0]); /*   A */
    vec3_t vector_b = vec3_from_vec4(vertices[1]); /*  / \ */
    vec3_t vector_c = vec3_from_vec4(vertices[2]); /* C---B */

    // Get the vector subtraction of B-A and C-A
    vec3_t vector_ab = vec3_sub(vector_b, vector_a);
    vec3_t vector_ac = vec3_sub(vector_c, vector_a);

    // Compute face normal using cross product
    vec3_t normal = vec3_cross(vector_ab, vector_ac);

    // Normalize the face normal vector
    vec3_normalize(&normal);

    return normal;
}

///////////////////////////////////////////////////////////////////////////////
// Checks if a triangle edge is top-left
///////////////////////////////////////////////////////////////////////////////
bool is_top_left(vec2_t *start, vec2_t *end)
{
    vec2_t edge = {end->x - start->x, end->y - start->y};
    bool is_top_edge = edge.y == 0 && edge.x > 0;
    bool is_left_edge = edge.y < 0;
    return is_left_edge || is_top_edge;
}

///////////////////////////////////////////////////////////////////////////////
// Performs the 2D edge-cross between 2 vertices and a point
///////////////////////////////////////////////////////////////////////////////
float edge_cross(vec2_t *a, vec2_t *b, vec2_t *p)
{
    vec2_t ab = {b->x - a->x, b->y - a->y};
    vec2_t ap = {p->x - a->x, p->y - a->y};

    return ab.x * ap.y - ab.y * ap.x;
}

///////////////////////////////////////////////////////////////////////////////////////
// Function to calculate the barycentric weights for point p
///////////////////////////////////////////////////////////////////////////////////////
/*
/         (B)
/         /|\
/        / | \
/       /  |  \
/      /  (P)  \
/     /  /   \  \
/    / /       \ \
/   //           \\
/  (A)------------(C)
*/
///////////////////////////////////////////////////////////////////////////////
vec3_t barycentric_weights(vec2_t a, vec2_t b, vec2_t c, vec2_t p)
{
    float area = edge_cross(&a, &b, &c);

    float w0 = edge_cross(&b, &c, &p); // Area of triangle bcp
    float w1 = edge_cross(&c, &a, &p); // Area of triangle cap
    float w2 = edge_cross(&a, &b, &p); // Area of triangle abp

    vec3_t weights = {
        w0 / area,  // α
        w1 / area,  // β
        w2 / area}; // γ

    return weights;
}

///////////////////////////////////////////////////////////////////////////////
// Function to draw a solid pixel at position (x,y) using depth interpolation
///////////////////////////////////////////////////////////////////////////////
void draw_triangle_pixel(int x, int y,
                         vec4_t point_a, vec4_t point_b, vec4_t point_c,
                         float inv_w0, float inv_w1, float inv_w2,
                         uint32_t color)
{
    vec2_t p = {x, y};
    vec2_t a = vec2_from_vec4(point_a);
    vec2_t b = vec2_from_vec4(point_b);
    vec2_t c = vec2_from_vec4(point_c);

    vec3_t weights = barycentric_weights(a, b, c, p);

    float alpha = weights.x;
    float beta = weights.y;
    float gamma = weights.z;

    // Interpolate the value of 1/w for the current pixel
    float interpolated_inv_w = alpha * inv_w0 + beta * inv_w1 + gamma * inv_w2;

    // Adjust 1/w so the pixel that are closer to the camera have smaller values
    interpolated_inv_w = 1.0 - interpolated_inv_w;

    // Only draw the pixel if the depth value is less than the one previously stored in the z-buffer
    if (interpolated_inv_w < get_zbuffer_at(x, y))
    {
        // Draw a pixel at position (x,y) with a solid color
        draw_pixel(x, y, color);

        // Update the z-buffer value with 1/w of this current pixel
        update_zbuffer_at(x, y, interpolated_inv_w);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Draw a triangle using three raw line calls
///////////////////////////////////////////////////////////////////////////////
void draw_triangle_wire(vec2_t *v0, vec2_t *v1, vec2_t *v2, uint32_t color)
{
    draw_line(v0->x, v0->y, v1->x, v1->y, color);
    draw_line(v1->x, v1->y, v2->x, v2->y, color);
    draw_line(v2->x, v2->y, v0->x, v0->y, color);
}

///////////////////////////////////////////////////////////////////////////////
// Function to draw the textured pixel at position x and y using interpolation
///////////////////////////////////////////////////////////////////////////////
void draw_texel(int x, int y,
                vec4_t point_a, vec4_t point_b, vec4_t point_c,
                float u0_over_w, float v0_over_w,
                float u1_over_w, float v1_over_w,
                float u2_over_w, float v2_over_w,
                float inv_w0, float inv_w1, float inv_w2,
                upng_t *texture)
{
    vec2_t p = {x, y};
    vec2_t a = vec2_from_vec4(point_a);
    vec2_t b = vec2_from_vec4(point_b);
    vec2_t c = vec2_from_vec4(point_c);

    vec3_t weights = barycentric_weights(a, b, c, p);

    float alpha = weights.x;
    float beta = weights.y;
    float gamma = weights.z;

    // Interpolate u/w, v/w and 1/w using pre-calculated values
    float interpolated_u_over_w = alpha * u0_over_w + beta * u1_over_w + gamma * u2_over_w;
    float interpolated_v_over_w = alpha * v0_over_w + beta * v1_over_w + gamma * v2_over_w;
    float interpolated_inv_w = alpha * inv_w0 + beta * inv_w1 + gamma * inv_w2;

    // Divide back both interpolated values by 1/w
    float interpolated_u = interpolated_u_over_w / interpolated_inv_w;
    float interpolated_v = interpolated_v_over_w / interpolated_inv_w;

    // Get the mesh texture width and height dimensions
    int texture_width = upng_get_width(texture);
    int texture_height = upng_get_height(texture);

    // Map UV coordinate to the full texture width & height
    int tex_x = abs((int)(interpolated_u * texture_width)) % texture_width;
    int tex_y = abs((int)(interpolated_v * texture_height)) % texture_height;

    // Adjust 1/w so the pixel that are closer to the camera have smaller values
    interpolated_inv_w = 1.0 - interpolated_inv_w;

    // Only draw the pixel if the depth value is less than the one previously stored in the z-buffer
    if (interpolated_inv_w < get_zbuffer_at(x, y))
    {
        // Get the buffer of colors from the texture
        uint32_t *texture_buffer = (uint32_t *)upng_get_buffer(texture);

        // Draw a pixel at position (x,y) with the color that comes from the mapped texture
        draw_pixel(x, y, texture_buffer[(texture_width * tex_y) + tex_x]);

        // Update the z-buffer value with 1/w of this current pixel
        update_zbuffer_at(x, y, interpolated_inv_w);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Draw a flat-shaded fill triangle
///////////////////////////////////////////////////////////////////////////////
/*
/          (x0,y0)
/            / \
/           /   \
/          /     \
/         /       \
/        /         \
/   (x1,y1)------(Mx,My)
/       \_           \
/          \_         \
/             \_       \
/                \_     \
/                   \    \
/                     \_  \
/                        \_\
/                           \
/                         (x2,y2)
*/
///////////////////////////////////////////////////////////////////////////////
void draw_filled_triangle(vec4_t *v0, vec4_t *v1, vec4_t *v2, uint32_t color)
{
    // Finds the bounding box with all candidate pixels
    int x_min = floor(MIN(MIN(v0->x, v1->x), v2->x));
    int y_min = floor(MIN(MIN(v0->y, v1->y), v2->y));
    int x_max = ceil(MAX(MAX(v0->x, v1->x), v2->x));
    int y_max = ceil(MAX(MAX(v0->y, v1->y), v2->y));

    // Screen 2D points from vertices v0, v1, and v2
    vec2_t sv0 = {v0->x, v0->y};
    vec2_t sv1 = {v1->x, v1->y};
    vec2_t sv2 = {v2->x, v2->y};

    // Compute the area of the entire triangle/parallelogram
    float area = edge_cross(&sv0, &sv1, &sv2);

    // Back-face culling using the signed-area
    if (area <= 0)
        return;

    float inv_w0 = 1.0f / v0->w;
    float inv_w1 = 1.0f / v1->w;
    float inv_w2 = 1.0f / v2->w;

    // Loop through bounding box
    for (int y = y_min; y <= y_max; y++)
    {
        for (int x = x_min; x <= x_max; x++)
        {
            vec2_t p = {(float)x + 0.5f, (float)y + 0.5f};
            vec3_t w = barycentric_weights(sv0, sv1, sv2, p);

            // Check if pixel is inside using top-left rule
            bool inside =
                (w.x > 0 || (w.x == 0 && is_top_left(&sv1, &sv2))) &&
                (w.y > 0 || (w.y == 0 && is_top_left(&sv2, &sv0))) &&
                (w.z > 0 || (w.z == 0 && is_top_left(&sv0, &sv1)));

            if (inside)
            {
                // Interpolate the value of 1/w for the current pixel
                float interpolated_inv_w = w.x * inv_w0 + w.y * inv_w1 + w.z * inv_w2;

                // Adjust 1/w so the pixels that are closer to the camera have smaller values
                interpolated_inv_w = 1.0 - interpolated_inv_w;

                // Only draw the pixel if the depth value is less than the one previously stored in the z-buffer
                if (interpolated_inv_w < get_zbuffer_at(x, y))
                {
                    // Draw a pixel at position (x,y) with a solid color
                    draw_pixel(x, y, color);

                    // Update the z-buffer value with the 1/w of this current pixel
                    update_zbuffer_at(x, y, interpolated_inv_w);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Draw a filled triangle and apply Gouraud shading
///////////////////////////////////////////////////////////////////////////////
void draw_filled_triangle_gouraud(triangle_t triangle)
{
    vec2_t a = {triangle.points[0].x, triangle.points[0].y};
    vec2_t b = {triangle.points[1].x, triangle.points[1].y};
    vec2_t c = {triangle.points[2].x, triangle.points[2].y};

    // Find the bounding box of the triangle
    int x_min = (int)floor(fmin(fmin(a.x, b.x), c.x));
    int y_min = (int)floor(fmin(fmin(a.y, b.y), c.y));
    int x_max = (int)ceil(fmax(fmax(a.x, b.x), c.x));
    int y_max = (int)ceil(fmax(fmax(a.y, b.y), c.y));

    // Clamp to screen bounds
    if (x_min < 0)
        x_min = 0;
    if (y_min < 0)
        y_min = 0;
    if (x_max >= window_width)
        x_max = window_width - 1;
    if (y_max >= window_height)
        y_max = window_height - 1;

    // Pre-calculate reciprocals once per triangle to avoid divisions per pixel
    float inv_w0 = 1.0f / triangle.points[0].w;
    float inv_w1 = 1.0f / triangle.points[1].w;
    float inv_w2 = 1.0f / triangle.points[2].w;
    float i0_over_w = triangle.intensity[0] * inv_w0;
    float i1_over_w = triangle.intensity[1] * inv_w1;
    float i2_over_w = triangle.intensity[2] * inv_w2;

    // Loop every pixel in the bounding box
    for (int y = y_min; y <= y_max; y++)
    {
        for (int x = x_min; x <= x_max; x++)
        {
            vec2_t p = {(float)x, (float)y};
            vec3_t weights = barycentric_weights(a, b, c, p);

            float alpha = weights.x;
            float beta = weights.y;
            float gamma = weights.z;

            // Check if pixel is inside the triangle (all weights >= 0)
            if (alpha >= 0 && beta >= 0 && gamma >= 0)
            {
                // Interpolate the value of 1/w for the current pixel
                float interpolated_inv_w = alpha * inv_w0 + beta * inv_w1 + gamma * inv_w2;

                if (interpolated_inv_w <= 0.0f)
                {
                    continue;
                }

                // Adjust 1/w so the pixel that are closer to the camera have smaller values
                float depth = 1.0f - interpolated_inv_w;

                if (depth < get_zbuffer_at(x, y))
                {
                    // Perspective-correct interpolation for per-vertex intensity.
                    float interpolated_i_over_w = alpha * i0_over_w + beta * i1_over_w + gamma * i2_over_w;
                    float pixel_intensity = interpolated_i_over_w / interpolated_inv_w;

                    // Clamp intensity
                    if (pixel_intensity < 0.0f)
                    {
                        pixel_intensity = 0.0f;
                    }
                    if (pixel_intensity > 1.0f)
                    {
                        pixel_intensity = 1.0f;
                    }

                    // Apply interpolated intensity to the base color
                    uint32_t pixel_color = light_apply_intensity(triangle.color, pixel_intensity);
                    draw_pixel(x, y, pixel_color);

                    update_zbuffer_at(x, y, depth);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Draw a textured triangle based on a texture array of colors
///////////////////////////////////////////////////////////////////////////////
/*
/        v1
/        /\
/       /  \
/      /    \
/     /      \
/   v0        \
/     \_       \
/        \_     \
/           \_   \
/              \_ \
/                 \\
/                   \
/                    v2
*/
///////////////////////////////////////////////////////////////////////////////
void draw_textured_triangle(vec4_t *vrtx0, float u0, float v0,
                            vec4_t *vrtx1, float u1, float v1,
                            vec4_t *vrtx2, float u2, float v2,
                            upng_t *texture)
{

    // Flip the V component to account for inverted UV-coordinates (V grows downwards)
    v0 = 1.0 - v0;
    v1 = 1.0 - v1;
    v2 = 1.0 - v2;

    // Finds the bounding box with all candidate pixels
    int x_min = floor(MIN(MIN(vrtx0->x, vrtx1->x), vrtx2->x));
    int y_min = floor(MIN(MIN(vrtx0->y, vrtx1->y), vrtx2->y));
    int x_max = ceil(MAX(MAX(vrtx0->x, vrtx1->x), vrtx2->x));
    int y_max = ceil(MAX(MAX(vrtx0->y, vrtx1->y), vrtx2->y));

    // Screen 2D points from vertices v0, v1, and v2
    vec2_t sv0 = {vrtx0->x, vrtx0->y};
    vec2_t sv1 = {vrtx1->x, vrtx1->y};
    vec2_t sv2 = {vrtx2->x, vrtx2->y};

    // Compute the area of the entire triangle/parallelogram
    float area = edge_cross(&sv0, &sv1, &sv2);

    // Back-face culling using the signed-area
    if (area <= 0)
        return;

    float inv_w0 = 1.0f / vrtx0->w;
    float inv_w1 = 1.0f / vrtx1->w;
    float inv_w2 = 1.0f / vrtx2->w;

    for (int y = y_min; y <= y_max; y++)
    {
        for (int x = x_min; x <= x_max; x++)
        {
            vec2_t p = {(float)x + 0.5f, (float)y + 0.5f};
            vec3_t w = barycentric_weights(sv0, sv1, sv2, p);

            bool inside =
                (w.x > 0 || (w.x == 0 && is_top_left(&sv1, &sv2))) &&
                (w.y > 0 || (w.y == 0 && is_top_left(&sv2, &sv0))) &&
                (w.z > 0 || (w.z == 0 && is_top_left(&sv0, &sv1)));

            if (inside)
            {
                // Perform the interpolation of all U/w and V/w values using barycentric weights and a factor of 1/w
                float interpolated_u = (u0 * inv_w0 * w.x + u1 * inv_w1 * w.y + u2 * inv_w2 * w.z);
                float interpolated_v = (v0 * inv_w0 * w.x + v1 * inv_w1 * w.y + v2 * inv_w2 * w.z);

                // Also interpolate the value of 1/w for the current pixel
                float interpolated_inv_w = w.x * inv_w0 + w.y * inv_w1 + w.z * inv_w2;

                // Now we can divide back both interpolated values by 1/w
                interpolated_u /= interpolated_inv_w;
                interpolated_v /= interpolated_inv_w;

                // Get the mesh texture width and height dimensions
                int tex_width = upng_get_width(texture);
                int tex_height = upng_get_height(texture);

                // Map the UV coordinate to the full texture width and height
                int tex_x = abs((int)(interpolated_u * tex_width)) % tex_width;
                int tex_y = abs((int)(interpolated_v * tex_height)) % tex_height;

                // Adjust 1/w so the pixels that are closer to the camera have smaller values
                interpolated_inv_w = 1.0 - interpolated_inv_w;

                // Only draw the pixel if the depth value is less than the one previously stored in the z-buffer
                if (interpolated_inv_w < get_zbuffer_at(x, y))
                {
                    // Draw a pixel at position (x,y) with the color that comes from the mapped texture
                    uint32_t *tex_buffer = (uint32_t *)upng_get_buffer(texture);
                    draw_pixel(x, y, tex_buffer[tex_y * tex_width + tex_x]);

                    // Update the z-buffer value with the 1/w of this current pixel
                    update_zbuffer_at(x, y, interpolated_inv_w);
                }
            }
        }
    }
}