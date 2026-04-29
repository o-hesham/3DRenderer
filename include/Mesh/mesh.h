#pragma once

#include "Math/vector.h"
#include "Mesh/triangle.h"
#include "upng/upng.h"

///////////////////////////////////////////////////////////////////////
// Define a struct for a dynamic size meshes
///////////////////////////////////////////////////////////////////////
typedef struct
{
    vec3_t *vertices;   // dynamic array of vertices
    vec3_t *normals;    // dynamic array of vertex normals (from vn lines)
    face_t *faces;      // dynamic array of faces
    upng_t *texture;    // mesh PNG texture pointer
    vec3_t rotation;    // rotation with x,y,z values
    vec3_t scale;       // scale with x,y,z values
    vec3_t translation; // translate with x,y,z values
} mesh_t;

void load_mesh(char *obj_filepath, char *png_filepath, vec3_t scale, vec3_t translation, vec3_t rotation);
void load_mesh_obj_data(mesh_t *mesh, char *obj_filepath);
void load_mesh_png_data(mesh_t *mesh, char *png_filepath);

int get_num_meshes(void);
mesh_t *get_mesh(int index);

void free_meshes(void);