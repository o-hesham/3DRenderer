#pragma once

#include <stdint.h>
#include "Math/vector.h"
#include "Rendering/texture.h"
#include "upng/upng.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct
{
    int a;
    int b;
    int c;
    int a_n;     // normal index for vertex a
    int b_n;     // normal index for vertex b
    int c_n;     // normal index for vertex c
    tex2_t a_uv; // texture index for vertex a
    tex2_t b_uv; // texture index for vertex b
    tex2_t c_uv; // texture index for vertex c
    uint32_t color;
} face_t;

typedef struct
{
    vec4_t points[3];
    tex2_t texcoords[3];
    upng_t *texture;
    uint32_t color;      // base color (for Gouraud interpolation)
    uint32_t flat_color; // pre-computed flat shaded color
    float intensity[3];  // Light intensity at each vertex (for Gouraud)
} triangle_t;

vec3_t get_triangle_normal(vec4_t vertices[3]);
vec3_t barycentric_weights(vec2_t a, vec2_t b, vec2_t c, vec2_t p);
void draw_triangle_wire(vec2_t *v0, vec2_t *v1, vec2_t *v2, uint32_t color);
void draw_texel(int x, int y,
                vec4_t point_a, vec4_t point_b, vec4_t point_c,
                float u0_over_w, float v0_over_w, float u1_over_w, float v1_over_w, float u2_over_w, float v2_over_w,
                float inv_w0, float inv_w1, float inv_w2,
                upng_t *texture);
void draw_triangle_pixel(int x, int y,
                         vec4_t point_a, vec4_t point_b, vec4_t point_c,
                         float inv_w0, float inv_w1, float inv_w2,
                         uint32_t color);
void draw_filled_triangle(vec4_t *v0, vec4_t *v1, vec4_t *v2, uint32_t color);
void draw_filled_triangle_gouraud(triangle_t triangle);
void draw_textured_triangle(vec4_t *vrtx0, float u0, float v0,
                            vec4_t *vrtx1, float u1, float v1,
                            vec4_t *vrtx2, float u2, float v2,
                            upng_t *texture);
