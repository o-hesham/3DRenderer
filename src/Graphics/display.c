#include "Graphics/display.h"
#include <stdio.h>
#include <math.h>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

static uint32_t *color_buffer = NULL;
static float *z_buffer = NULL;
static SDL_Texture *color_buffer_texture = NULL;

int window_width = 800;
int window_height = 600;

enum cull_method cull_method = CULL_NONE;
enum render_method render_method = RENDER_WIRE;

bool initialize_window(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        fprintf(stderr, "Error initializing SDL.\n");
        return false;
    }

    // Use SDL to query what is the fullscreen max. width and height
    SDL_DisplayMode display_mode;
    if (SDL_GetCurrentDisplayMode(0, &display_mode) != 0)
    {
        fprintf(stderr, "Error getting display mode: %s\n", SDL_GetError());
        return false;
    }

    int fullscreen_width = display_mode.w;
    int fullscreen_height = display_mode.h;

    window_width = fullscreen_width / 3;
    window_height = fullscreen_height / 3;

    // Create a SDL Window
    window = SDL_CreateWindow("Software Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, fullscreen_width, fullscreen_height, 0);
    if (!window)
    {
        fprintf(stderr, "Error creating SDL window.\n");
        return false;
    }

    // Create a SDL Renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        fprintf(stderr, "Error creating SDL renderer.\n");
        return false;
    }

    // Allocate the required memory in bytes to hold the color buffer and the z-buffer
    color_buffer = malloc(sizeof(uint32_t) * window_width * window_height);
    if (!color_buffer)
    {
        fprintf(stderr, "Error allocating memory.\n");
        return false;
    }
    z_buffer = malloc(sizeof(float) * window_width * window_height);
    if (!z_buffer)
    {
        fprintf(stderr, "Error allocating memory.\n");
        return false;
    }

    // Creating a SDL texture that is used to display the color buffer
    color_buffer_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);
    if (!color_buffer_texture)
    {
        fprintf(stderr, "Error creating SDL Texture.\n");
        return false;
    }

    // Lock the mouse and hide the cursor
    SDL_SetRelativeMouseMode(1);

    return true;
}

void draw_grid(void)
{
    for (int y = 0; y < window_height; y++)
    {
        for (int x = 0; x < window_width; x++)
        {
            if (x % 10 == 0 && y % 10 == 0)
            {
                color_buffer[(window_width * y) + x] = 0xFF444444;
            }
        }
    }
}

void draw_pixel(int x, int y, uint32_t color)
{
    if (x >= 0 && x < window_width && y >= 0 && y < window_height)
    {
        color_buffer[(window_width * y) + x] = color;
    }
}

void draw_line(float x0, float y0, float x1, float y1, uint32_t color) // DDA Algorithm
{
    float delta_x = (x1 - x0);
    float delta_y = (y1 - y0);

    float longest_side = (fabs(delta_x) >= fabs(delta_y)) ? fabs(delta_x) : fabs(delta_y);

    if (longest_side == 0)
    {
        draw_pixel(x0, y0, color);
        return;
    }

    float x_inc = delta_x / longest_side;
    float y_inc = delta_y / longest_side;

    float current_x = x0;
    float current_y = y0;
    for (int i = 0; i <= longest_side; i++)
    {
        draw_pixel(round(current_x), round(current_y), color);
        current_x += x_inc;
        current_y += y_inc;
    }
}

void draw_rect(int x, int y, int width, int height, uint32_t color)
{
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            int current_x = x + i;
            int current_y = y + j;
            draw_pixel(current_x, current_y, color);
        }
    }
}

void render_color_buffer(void)
{
    SDL_UpdateTexture(color_buffer_texture, NULL, color_buffer, (int)(window_width * sizeof(uint32_t)));

    SDL_RenderCopy(renderer, color_buffer_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void clear_color_buffer(uint32_t color)
{
    int buffer_size = window_width * window_height;

    for (int i = 0; i < buffer_size; i++)
        color_buffer[i] = color;
}

void clear_z_buffer()
{
    int buffer_size = window_width * window_height;

    for (int i = 0; i < buffer_size; i++)
        z_buffer[i] = 1.0;
}

float get_zbuffer_at(int x, int y)
{
    if (x >= 0 && x < window_width && y >= 0 && y < window_height)
    {
        return z_buffer[(window_width * y) + x];
    }
    else
        return 1.0;
}

void update_zbuffer_at(int x, int y, float value)
{
    if (x >= 0 && x < window_width && y >= 0 && y < window_height)
    {
        z_buffer[(window_width * y) + x] = value;
    }
}

void destroy_window(void)
{
    free(color_buffer);
    free(z_buffer);
    SDL_DestroyTexture(color_buffer_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}