#include <math.h>
#include "Math/vector.h"

///////////////////////////////////////////////////////////////////////
// Implementations of Vector 2D functions
///////////////////////////////////////////////////////////////////////
vec2_t vec2_new(float x, float y)
{
    return (vec2_t){x, y};
}

float vec2_length(vec2_t v)
{
    return sqrt(v.x * v.x + v.y * v.y);
}

void vec2_normalize(vec2_t *v)
{
    float length = vec2_length(*v);

    v->x /= length;
    v->y /= length;
}

vec2_t vec2_add(vec2_t a, vec2_t b)
{
    vec2_t result =
        {.x = a.x + b.x,
         .y = a.y + b.y};

    return result;
}

vec2_t vec2_sub(vec2_t a, vec2_t b)
{
    vec2_t result =
        {.x = a.x - b.x,
         .y = a.y - b.y};

    return result;
}

vec2_t vec2_mult(vec2_t a, float n)
{
    vec2_t result =
        {.x = a.x * n,
         .y = a.y * n};

    return result;
}

vec2_t vec2_div(vec2_t a, float n)
{
    if (n == 0.0f)
    {
        vec2_t result =
            {.x = 0.0f,
             .y = 0.0f};

        return result;
    }

    vec2_t result =
        {.x = a.x / n,
         .y = a.y / n};

    return result;
}

float vec2_dot(vec2_t a, vec2_t b)
{
    return (a.x * b.x) + (a.y * b.y);
}

///////////////////////////////////////////////////////////////////////
// Implementations of Vector 3D functions
///////////////////////////////////////////////////////////////////////
vec3_t vec3_new(float x, float y, float z)
{
    return (vec3_t){x, y, z};
}

vec3_t vec3_clone(vec3_t *v)
{
    return (vec3_t){v->x, v->y, v->z};
}

float vec3_length(vec3_t v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

void vec3_normalize(vec3_t *v)
{
    float length = vec3_length(*v);

    v->x /= length;
    v->y /= length;
    v->z /= length;
}

vec3_t vec3_add(vec3_t a, vec3_t b)
{
    vec3_t result =
        {.x = a.x + b.x,
         .y = a.y + b.y,
         .z = a.z + b.z};

    return result;
}

vec3_t vec3_sub(vec3_t a, vec3_t b)
{
    vec3_t result =
        {.x = a.x - b.x,
         .y = a.y - b.y,
         .z = a.z - b.z};

    return result;
}

vec3_t vec3_mult(vec3_t a, float n)
{
    vec3_t result =
        {.x = a.x * n,
         .y = a.y * n,
         .z = a.z * n};

    return result;
}

vec3_t vec3_div(vec3_t a, float n)
{
    if (n == 0.0f)
    {
        vec3_t result =
            {.x = 0.0f,
             .y = 0.0f,
             .z = 0.0f};

        return result;
    }

    vec3_t result =
        {.x = a.x / n,
         .y = a.y / n,
         .z = a.z / n};

    return result;
}

vec3_t vec3_cross(vec3_t a, vec3_t b)
{
    vec3_t result =
        {.x = a.y * b.z - a.z * b.y,
         .y = a.z * b.x - a.x * b.z,
         .z = a.x * b.y - a.y * b.x};

    return result;
}

float vec3_dot(vec3_t a, vec3_t b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

vec3_t vec3_rotate_x(vec3_t v, float angle)
{
    vec3_t rotated_vector = {
        .x = v.x,
        .y = v.y * cos(angle) - v.z * sin(angle),
        .z = v.y * sin(angle) + v.z * cos(angle)};

    return rotated_vector;
}

vec3_t vec3_rotate_y(vec3_t v, float angle)
{
    vec3_t rotated_vector = {
        .x = v.x * cos(angle) - v.z * sin(angle),
        .y = v.y,
        .z = v.x * sin(angle) + v.z * cos(angle)};

    return rotated_vector;
}

vec3_t vec3_rotate_z(vec3_t v, float angle)
{
    vec3_t rotated_vector = {
        .x = v.x * cos(angle) - v.y * sin(angle),
        .y = v.x * sin(angle) + v.y * cos(angle),
        .z = v.z};

    return rotated_vector;
}

///////////////////////////////////////////////////////////////////////
// Implementations of Vector conversion functions
///////////////////////////////////////////////////////////////////////
vec4_t vec4_from_vec3(vec3_t v)
{
    vec4_t result = {v.x, v.y, v.z, 1.0};
    return result;
}

vec3_t vec3_from_vec4(vec4_t v)
{
    vec3_t result = {v.x, v.y, v.z};
    return result;
}

vec2_t vec2_from_vec4(vec4_t v)
{
    vec2_t result = {v.x, v.y};
    return result;
}