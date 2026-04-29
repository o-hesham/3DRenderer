#include <stdio.h>
#include <stdlib.h>
#include "Graphics/display.h"
#include "array.h"
#include "Math/vector.h"
#include "Math/matrix.h"
#include "Mesh/mesh.h"
#include "Lighting/light.h"
#include "upng/upng.h"
#include "Graphics/camera.h"
#include "Rendering/clipping.h"
#include <math.h>

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// TODO: OPTIMIZE PERFORMANCE
// TODO: ADD SHADING TO TEXTURES
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Global variables for execution status and game loop
///////////////////////////////////////////////////////////////////////
bool is_running = false;
Uint32 previous_frame_time = 0;
float delta_time = 0;

///////////////////////////////////////////////////////////////////////
// Array of triangles that should be rendered frame by frame
///////////////////////////////////////////////////////////////////////
#define MAX_TRIANGLES 10000
triangle_t triangles_to_render[MAX_TRIANGLES];
int num_triangles_to_render = 0;

///////////////////////////////////////////////////////////////////////////////
// Declaration of our global transformation matrices
///////////////////////////////////////////////////////////////////////////////
mat4_t world_matrix;
mat4_t proj_matrix;
mat4_t view_matrix;

///////////////////////////////////////////////////////////////////////
// Setup function to initialize variables and game objects
///////////////////////////////////////////////////////////////////////
void setup(void)
{
    // Initialize render mode and triangle culling method
    render_method = RENDER_TEXTURED;
    cull_method = CULL_BACKFACE;

    // Initialize the scene light direction
    init_light(vec3_new(0, 0, 1));

    // Initialize the perspective projection matrix
    float aspectx = (float)window_width / (float)window_height;
    float aspecty = (float)window_height / (float)window_width;
    float fovy = M_PI / 3.0; // 60
    float fovx = atan(tan(fovy / 2) * aspectx) * 2;
    float znear = 1.0;
    float zfar = 50.0;
    proj_matrix = mat4_make_perspective(fovy, aspecty, znear, zfar);

    // Initialize frustum planes with a point and a normal
    init_frustum_planes(fovx, fovy, znear, zfar);

    load_mesh("./assets/models/spaceship.obj", "./assets/textures/spaceship.png", vec3_new(1, 1, 1), vec3_new(0, 0, 5), vec3_new(0, M_PI / 2, 0));

    // load_mesh("./assets/models/runway.obj", "./assets/textures/runway.png", vec3_new(1, 1, 1), vec3_new(0, -1.5, 23), vec3_new(0, 0, 0));
    // load_mesh("./assets/models/f22.obj", "./assets/textures/f22.png", vec3_new(1, 1, 1), vec3_new(0, -1.3, 5), vec3_new(0, -M_PI / 2, 0));
    // load_mesh("./assets/models/efa.obj", "./assets/textures/efa.png", vec3_new(1, 1, 1), vec3_new(-2, -1.3, 9), vec3_new(0, -M_PI / 2, 0));
    // load_mesh("./assets/models/f117.obj", "./assets/textures/f117.png", vec3_new(1, 1, 1), vec3_new(2, -1.3, 9), vec3_new(0, -M_PI / 2, 0));
}

///////////////////////////////////////////////////////////////////////////////
// Poll system events and handle keyboard input
///////////////////////////////////////////////////////////////////////////////
void process_input(void)
{

    const Uint8 *keystate = SDL_GetKeyboardState(NULL);
    if (keystate[SDL_SCANCODE_W])
    {
        update_camera_forward_velocity(vec3_mult(get_camera_direction(), 5.0 * delta_time));
        update_camera_position(vec3_add(get_camera_position(), get_camera_forward_velocity()));
    }
    if (keystate[SDL_SCANCODE_S])
    {
        update_camera_forward_velocity(vec3_mult(get_camera_direction(), 5.0 * delta_time));
        update_camera_position(vec3_sub(get_camera_position(), get_camera_forward_velocity()));
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            is_running = false;
            break;
        case SDL_MOUSEMOTION:
            float sensitivity = 0.1f;
            float yaw = event.motion.xrel * sensitivity;
            float pitch = event.motion.yrel * sensitivity;

            rotate_camera_yaw(yaw * delta_time);
            rotate_camera_pitch(pitch * delta_time);
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                is_running = false;
            }
            if (event.key.keysym.sym == SDLK_1)
                render_method = RENDER_WIRE_VERTEX;
            if (event.key.keysym.sym == SDLK_2)
                render_method = RENDER_WIRE;
            if (event.key.keysym.sym == SDLK_3)
                render_method = RENDER_FILL_TRIANGLE;
            if (event.key.keysym.sym == SDLK_4)
                render_method = RENDER_FILL_TRIANGLE_WIRE;
            if (event.key.keysym.sym == SDLK_5)
                render_method = RENDER_GOURAUD;
            if (event.key.keysym.sym == SDLK_6)
                render_method = RENDER_GOURAUD_WIRE;
            if (event.key.keysym.sym == SDLK_7)
                render_method = RENDER_TEXTURED;
            if (event.key.keysym.sym == SDLK_8)
                render_method = RENDER_TEXTURED_WIRE;
            if (event.key.keysym.sym == SDLK_c)
                cull_method = CULL_BACKFACE;
            if (event.key.keysym.sym == SDLK_v)
                cull_method = CULL_NONE;
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Process the graphics pipeline stages for all the mesh triangles
///////////////////////////////////////////////////////////////////////////////
// +-------------+
// | Model space |  <-- original mesh vertices
// +-------------+
// |   +-------------+
// `-> | World space |  <-- multiply by world matrix
//     +-------------+
//     |   +--------------+
//     `-> | Camera space |  <-- multiply by view matrix
//         +--------------+
//         |    +------------+
//         `--> |  Clipping  |  <-- clip against the six frustum planes
//              +------------+
//              |    +------------+
//              `--> | Projection |  <-- multiply by projection matrix
//                   +------------+
//                   |    +-------------+
//                   `--> | Image space |  <-- apply perspective divide
//                        +-------------+
//                        |    +--------------+
//                        `--> | Screen space |  <-- ready to render
//                             +--------------+
///////////////////////////////////////////////////////////////////////////////
void process_graphics_pipeline_stages(mesh_t *mesh)
{
    // Create a translation, rotation & scale matrix that will be used to multiply the mesh vertices
    mat4_t scale_matrix = mat4_make_scale(mesh->scale.x, mesh->scale.y, mesh->scale.z);
    mat4_t translation_matrix = mat4_make_translation(mesh->translation.x, mesh->translation.y, mesh->translation.z);
    mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh->rotation.x);
    mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh->rotation.y);
    mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh->rotation.z);

    // Create a World Matrix combining scale, rotation, and translation matrices
    world_matrix = mat4_identity();
    world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
    world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
    world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
    world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
    world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

    // Update camera look at target to create view matrix
    vec3_t target = get_camera_lookat_target();
    vec3_t up_direction = {0, 1, 0};
    view_matrix = mat4_look_at(get_camera_position(), target, up_direction);

    // Loop all triangle faces of our mesh
    int num_faces = array_length(mesh->faces);
    for (int i = 0; i < num_faces; i++)
    {
        face_t mesh_face = mesh->faces[i];

        vec3_t face_vertices[3];
        face_vertices[0] = mesh->vertices[mesh_face.a];
        face_vertices[1] = mesh->vertices[mesh_face.b];
        face_vertices[2] = mesh->vertices[mesh_face.c];

        vec4_t transformed_vertices[3];
        // Loop all three vertices of the current face and apply transformations
        for (int j = 0; j < 3; j++)
        {
            vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

            // Multiply the world matrix by the original vector
            transformed_vertex = mat4_mul_vec4(world_matrix, transformed_vertex);

            // Multiply the view matrix by the original vector to transform the scene to camera space
            transformed_vertex = mat4_mul_vec4(view_matrix, transformed_vertex);

            // Save transformed vertex in the array of transformed vertices
            transformed_vertices[j] = transformed_vertex;
        }

        // Calculate the triangle face normal
        vec3_t face_normal = get_triangle_normal(transformed_vertices);

        // Backface culling test to see if the current face should be projected
        if (cull_method == CULL_BACKFACE)
        {
            // Find the vector between a point in the triangle and the camera origin
            vec3_t camera_ray = vec3_sub(vec3_new(0, 0, 0), vec3_from_vec4(transformed_vertices[0]));

            // Calculate how alligned the camera ray is with the face normal using dot product
            float dot_normal_camera = vec3_dot(face_normal, camera_ray);

            if (dot_normal_camera <= 0.0f)
            {
                continue;
            }
        }

        /////////////////////////////////////////////////////////////////////
        // Compute per-vertex lighting for Gouraud shading
        /////////////////////////////////////////////////////////////////////
        float intensities[3];
        int normal_indices[3] = {mesh_face.a_n, mesh_face.b_n, mesh_face.c_n};

        bool has_vertex_normal = (mesh->normals != NULL && normal_indices[0] >= 0 && normal_indices[1] >= 0 && normal_indices[2] >= 0);

        if (has_vertex_normal)
        {
            for (int j = 0; j < 3; j++)
            {
                vec3_t vertex_normal = mesh->normals[normal_indices[j]];

                // Transform normal by world matrix
                vec4_t n4 = {vertex_normal.x, vertex_normal.y, vertex_normal.z, 0};
                n4 = mat4_mul_vec4(world_matrix, n4);
                n4 = mat4_mul_vec4(view_matrix, n4);

                vec3_t transformed_normal = {n4.x, n4.y, n4.z};
                vec3_normalize(&transformed_normal);

                float intensity = -vec3_dot(transformed_normal, get_light_direction());
                if (intensity < 0)
                    intensity = 0;
                if (intensity > 1)
                    intensity = 1;

                intensities[j] = intensity;
            }
        }
        else
        {
            // No vertex normals: render with constant intensity
            intensities[0] = intensities[1] = intensities[2] = 1.0f;
        }

        // Create a polygon from the original transformed triangle to be clipped
        polygon_t polygon = polygon_from_triangle(vec3_from_vec4(transformed_vertices[0]),
                                                  vec3_from_vec4(transformed_vertices[1]),
                                                  vec3_from_vec4(transformed_vertices[2]),
                                                  mesh_face.a_uv,
                                                  mesh_face.b_uv,
                                                  mesh_face.c_uv,
                                                  intensities[0],
                                                  intensities[1],
                                                  intensities[2]);

        // Clip the polygon and return a new polygon with potetntial new vertices
        clip_polygon(&polygon);

        // Break the clipped polygon apart back into individual triangles
        triangle_t triangles_after_clipping[MAX_NUM_POLY_TRIANGLES];
        int num_triangles_after_clipping = 0;

        triangles_from_polygon(&polygon, triangles_after_clipping, &num_triangles_after_clipping);

        // Loops all the assembled triangles after clipping
        for (int t = 0; t < num_triangles_after_clipping; t++)
        {
            triangle_t triangle_after_clipping = triangles_after_clipping[t];

            vec4_t projected_points[3];
            // Loop all three vertices to perform projection
            for (int j = 0; j < 3; j++)
            {
                // Project the current vertex
                projected_points[j] = mat4_mul_vec4_project(proj_matrix, triangle_after_clipping.points[j]);

                // Scale into the view
                projected_points[j].x *= (window_width / 2.0);
                projected_points[j].y *= (window_height / 2.0);

                // Invert the x & y values to account for flipped screen x & y coordinates
                // projected_points[j].x *= -1;
                projected_points[j].y *= -1;

                // Translate the projected points to the middle of the screen
                projected_points[j].x += (window_width / 2.0);
                projected_points[j].y += (window_height / 2.0);
            }

            // Flat shading: compute one color for the whole face
            float light_intensity_factor = -vec3_dot(face_normal, get_light_direction());
            uint32_t triangle_color = light_apply_intensity(mesh_face.color, light_intensity_factor);

            triangle_t triangle_to_render = {
                .points = {{projected_points[0].x, projected_points[0].y, projected_points[0].z, projected_points[0].w},
                           {projected_points[1].x, projected_points[1].y, projected_points[1].z, projected_points[1].w},
                           {projected_points[2].x, projected_points[2].y, projected_points[2].z, projected_points[2].w}},
                .texcoords = {
                    {triangle_after_clipping.texcoords[0].u, triangle_after_clipping.texcoords[0].v},
                    {triangle_after_clipping.texcoords[1].u, triangle_after_clipping.texcoords[1].v},
                    {triangle_after_clipping.texcoords[2].u, triangle_after_clipping.texcoords[2].v}},
                .texture = mesh->texture,
                .color = mesh_face.color,     // base color (for Gouraud interpolation)
                .flat_color = triangle_color, // pre-computed flat shaded color
                .intensity = {triangle_after_clipping.intensity[0], triangle_after_clipping.intensity[1], triangle_after_clipping.intensity[2]}};

            // Save the projected triangle in the array of triangles to render
            if (num_triangles_to_render < MAX_TRIANGLES)
            {
                triangles_to_render[num_triangles_to_render++] = triangle_to_render;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////
// Update function frame by frame with a fixed time step
///////////////////////////////////////////////////////////////////////
void update(void)
{
    Uint32 current_time = SDL_GetTicks();
    //  Wait some time until target frame time is reached
    Uint32 frame_time_so_far = current_time - previous_frame_time;
    Uint32 time_to_wait = FRAME_TARGET_TIME - (frame_time_so_far);

    // Only delay the execution if we are running too fast
    if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME)
    {
        SDL_Delay(time_to_wait);
    }

    current_time = SDL_GetTicks();

    // Get a delta time factor converted to seconds to be used to update our game objects
    delta_time = (current_time - previous_frame_time) / 1000.0f;

    previous_frame_time = current_time;

    // Initialize the counter of triangles to render for the current frame
    num_triangles_to_render = 0;

    // Loop all the meshes of our scene
    for (int mesh_index = 0; mesh_index < get_num_meshes(); mesh_index++)
    {
        mesh_t *mesh = get_mesh(mesh_index);

        mesh->rotation.y += 0.006;

        // Process the graphics pipeline stages for every mesh of our 3D scene
        process_graphics_pipeline_stages(mesh);
    }
}

///////////////////////////////////////////////////////////////////////
// Render functions to draw objects on the display
///////////////////////////////////////////////////////////////////////
void render(void)
{
    // Clear all the arrays to get ready for the next frame
    clear_color_buffer(0xFF000000);
    clear_z_buffer();

    draw_grid();

    // Loop all projected triangles and render them
    for (int i = 0; i < num_triangles_to_render; i++)
    {
        triangle_t triangle = triangles_to_render[i];

        // Draw flat-shaded filled triangle
        if (render_method == RENDER_FILL_TRIANGLE || render_method == RENDER_FILL_TRIANGLE_WIRE)
        {
            vec4_t v0 = {triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w};
            vec4_t v1 = {triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w};
            vec4_t v2 = {triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w};

            draw_filled_triangle(&v0, &v1, &v2, triangle.flat_color);
        }

        // Draw Gouraud-shaded filled triangle
        if (render_method == RENDER_GOURAUD || render_method == RENDER_GOURAUD_WIRE)
        {
            draw_filled_triangle_gouraud(triangle);
        }

        // Draw textured triangle
        if (render_method == RENDER_TEXTURED || render_method == RENDER_TEXTURED_WIRE)
        {
            vec4_t v0 = {triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w};
            vec4_t v1 = {triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w};
            vec4_t v2 = {triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w};

            draw_textured_triangle(&v0, triangle.texcoords[0].u, triangle.texcoords[0].v,
                                   &v1, triangle.texcoords[1].u, triangle.texcoords[1].v,
                                   &v2, triangle.texcoords[2].u, triangle.texcoords[2].v,
                                   triangle.texture);
        }

        // Draw triangle wireframe
        if (render_method == RENDER_WIRE || render_method == RENDER_WIRE_VERTEX || render_method == RENDER_FILL_TRIANGLE_WIRE || render_method == RENDER_GOURAUD_WIRE || render_method == RENDER_TEXTURED_WIRE)
        {
            vec2_t v0 = {triangle.points[0].x, triangle.points[0].y};
            vec2_t v1 = {triangle.points[1].x, triangle.points[1].y};
            vec2_t v2 = {triangle.points[2].x, triangle.points[2].y};

            draw_triangle_wire(&v0, &v1, &v2, 0xFFFFFFFF);
        }

        // Draw triangle vertex points
        if (render_method == RENDER_WIRE_VERTEX)
        {
            draw_rect(triangle.points[0].x - 3, triangle.points[0].y - 3, 6, 6, 0xFFFF0000); // vertex A
            draw_rect(triangle.points[1].x - 3, triangle.points[1].y - 3, 6, 6, 0xFFFF0000); // vertex B
            draw_rect(triangle.points[2].x - 3, triangle.points[2].y - 3, 6, 6, 0xFFFF0000); // vertex C
        }
    }

    render_color_buffer();
}

///////////////////////////////////////////////////////////////////////
// Free the memory that was dynamically allocated by the program
///////////////////////////////////////////////////////////////////////
void free_resources(void)
{
    free_meshes();
    destroy_window();
}

///////////////////////////////////////////////////////////////////////////////
// Main function
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    (void)argc, (void)argv;

    is_running = initialize_window();

    setup();

    while (is_running)
    {
        process_input();
        update();
        render();
    }

    free_resources();

    return 0;
}