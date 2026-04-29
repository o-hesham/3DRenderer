#pragma once

#include <SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include "Math/vector.h"

#define FPS 180
#define FRAME_TARGET_TIME (1000.0 / FPS)

enum cull_method
{
    CULL_NONE,
    CULL_BACKFACE
};

enum render_method
{
    RENDER_WIRE,
    RENDER_WIRE_VERTEX,
    RENDER_FILL_TRIANGLE,      // Flat shading
    RENDER_FILL_TRIANGLE_WIRE, // Flat shading
    RENDER_GOURAUD,
    RENDER_GOURAUD_WIRE,
    RENDER_TEXTURED,
    RENDER_TEXTURED_WIRE
};

extern enum cull_method cull_method;
extern enum render_method render_method;

extern SDL_Window *window;
extern SDL_Renderer *renderer;

extern int window_width;
extern int window_height;

bool initialize_window(void);
void draw_grid(void);
void draw_pixel(int x, int y, uint32_t color);
void draw_line(float x0, float y0, float x1, float y1, uint32_t color);
void draw_rect(int x, int y, int width, int height, uint32_t color);
void render_color_buffer(void);
void clear_color_buffer(uint32_t color);
void clear_z_buffer();

float get_zbuffer_at(int x, int y);
void update_zbuffer_at(int x, int y, float value);

void destroy_window(void);