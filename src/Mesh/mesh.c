#include <stdio.h>
#include <string.h>
#include "Mesh/mesh.h"
#include "array.h"

#define MAX_NUM_MESHES 10
static mesh_t meshes[MAX_NUM_MESHES];
static int mesh_count = 0;

void load_mesh(char *obj_filepath, char *png_filepath, vec3_t scale, vec3_t translation, vec3_t rotation)
{
    load_mesh_obj_data(&meshes[mesh_count], obj_filepath);
    load_mesh_png_data(&meshes[mesh_count], png_filepath);

    meshes[mesh_count].scale = scale;
    meshes[mesh_count].translation = translation;
    meshes[mesh_count].rotation = rotation;

    mesh_count++;
}

void load_mesh_obj_data(mesh_t *mesh, char *obj_filepath)
{
    FILE *file = fopen(obj_filepath, "r");
    if (!file)
    {
        fprintf(stderr, "Error opening file: %s\n", obj_filepath);
        return;
    }

    char line[1024];
    tex2_t *texcoords = NULL;

    while (fgets(line, 1024, file))
    {
        // Vertex information: "v 0.123 0.456 0.789"
        if (strncmp(line, "v ", 2) == 0)
        {
            vec3_t vertex;
            sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);

            array_push(mesh->vertices, vertex);
        }
        // Parse vertex normals: "vn 0.123 0.456 0.789"
        else if (strncmp(line, "vn ", 3) == 0)
        {
            vec3_t normal;
            sscanf(line, "vn %f %f %f", &normal.x, &normal.y, &normal.z);
            array_push(mesh->normals, normal);
        }
        // Texture coordinate information "vt 0.123 0.456"
        else if (strncmp(line, "vt ", 3) == 0)
        {
            tex2_t texcoord;
            sscanf(line, "vt %f %f", &texcoord.u, &texcoord.v);
            array_push(texcoords, texcoord);
        }
        // For "f v/vt/vn v/vt/vn v/vt/vn"
        else if (strncmp(line, "f ", 2) == 0)
        {
            int vertex_indices[4] = {0, 0, 0, 0};
            int texcoord_indices[4] = {0, 0, 0, 0};
            int normal_indices[4] = {0, 0, 0, 0};

            int matched = sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
                                 &vertex_indices[0], &texcoord_indices[0], &normal_indices[0],
                                 &vertex_indices[1], &texcoord_indices[1], &normal_indices[1],
                                 &vertex_indices[2], &texcoord_indices[2], &normal_indices[2],
                                 &vertex_indices[3], &texcoord_indices[3], &normal_indices[3]);
            if (matched != 9 && matched != 12)
            {
                // Try "f v1//vn1 v2//vn2 v3//vn3 v4//vn4" format (quad)
                matched = sscanf(line, "f %d//%d %d//%d %d//%d %d//%d",
                                 &vertex_indices[0], &normal_indices[0],
                                 &vertex_indices[1], &normal_indices[1],
                                 &vertex_indices[2], &normal_indices[2],
                                 &vertex_indices[3], &normal_indices[3]);
                if (matched != 8)
                {
                    // Try "f v1//vn1 v2//vn2 v3//vn3" format (triangle)
                    matched = sscanf(line, "f %d//%d %d//%d %d//%d",
                                     &vertex_indices[0], &normal_indices[0],
                                     &vertex_indices[1], &normal_indices[1],
                                     &vertex_indices[2], &normal_indices[2]);
                }
            }

            // Verify matched so we dont push it if parsing failed
            if (matched != 12 && matched != 9 && matched != 8 && matched != 6)
            {
                continue;
            }

            int face_vertex_count = (matched == 12 || matched == 8) ? 4 : 3;
            for (int i = 0; i < face_vertex_count; i++)
            {
                if (vertex_indices[i] <= 0 || normal_indices[i] <= 0)
                {
                    face_vertex_count = 0;
                    break;
                }
                if ((matched == 12 || matched == 9) && texcoord_indices[i] <= 0)
                {
                    face_vertex_count = 0;
                    break;
                }
            }
            if (face_vertex_count == 0)
            {
                continue;
            }

            // Triangle face
            if (matched == 9 || matched == 6)
            {
                face_t face = {
                    .a = vertex_indices[0] - 1,
                    .b = vertex_indices[1] - 1,
                    .c = vertex_indices[2] - 1,
                    .a_n = normal_indices[0] - 1,
                    .b_n = normal_indices[1] - 1,
                    .c_n = normal_indices[2] - 1,
                    .color = 0xFFFFFFFF};

                // Only assign UVs when texture coordinates are present (v/vt/vn format)
                if (matched == 9 && texcoords != NULL)
                {
                    face.a_uv = texcoords[texcoord_indices[0] - 1];
                    face.b_uv = texcoords[texcoord_indices[1] - 1];
                    face.c_uv = texcoords[texcoord_indices[2] - 1];
                }

                array_push(mesh->faces, face);
            }
            else
            {
                // Quad face -> split into two triangles: (0,1,2) and (0,2,3)
                face_t face1 = {
                    .a = vertex_indices[0] - 1,
                    .b = vertex_indices[1] - 1,
                    .c = vertex_indices[2] - 1,
                    .a_n = normal_indices[0] - 1,
                    .b_n = normal_indices[1] - 1,
                    .c_n = normal_indices[2] - 1,
                    .color = 0xFFFFFFFF};
                face_t face2 = {
                    .a = vertex_indices[0] - 1,
                    .b = vertex_indices[2] - 1,
                    .c = vertex_indices[3] - 1,
                    .a_n = normal_indices[0] - 1,
                    .b_n = normal_indices[2] - 1,
                    .c_n = normal_indices[3] - 1,
                    .color = 0xFFFFFFFF};

                // Only assign UVs when texture coordinates are present (v/vt/vn format)
                if (matched == 12 && texcoords != NULL)
                {
                    face1.a_uv = texcoords[texcoord_indices[0] - 1];
                    face1.b_uv = texcoords[texcoord_indices[1] - 1];
                    face1.c_uv = texcoords[texcoord_indices[2] - 1];

                    face2.a_uv = texcoords[texcoord_indices[0] - 1];
                    face2.b_uv = texcoords[texcoord_indices[2] - 1];
                    face2.c_uv = texcoords[texcoord_indices[3] - 1];
                }

                array_push(mesh->faces, face1);
                array_push(mesh->faces, face2);
            }
        }
    }
    fclose(file);
    array_free(texcoords);
}

void load_mesh_png_data(mesh_t *mesh, char *png_filepath)
{
    upng_t *png_image = upng_new_from_file(png_filepath);
    if (png_image != NULL)
    {
        upng_decode(png_image);
        if (upng_get_error(png_image) == UPNG_EOK)
        {
            mesh->texture = png_image;
        }
    }
}

int get_num_meshes(void)
{
    return mesh_count;
}

mesh_t *get_mesh(int index)
{
    return &meshes[index];
}

void free_meshes(void)
{
    for (int i = 0; i < mesh_count; i++)
    {
        upng_free(meshes[i].texture);
        array_free(meshes[i].faces);
        array_free(meshes[i].vertices);
        array_free(meshes[i].normals);
    }
}
