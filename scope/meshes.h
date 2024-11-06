#ifndef MESHES_H
#define MESHES_H

#include "status.h"
#include "maths/vector3.h"

#include <stdio.h>
#include <stdlib.h>

// Define the size of strides for the arrays.
#define STRIDE_FACE_VERTICES	3
#define STRIDE_FACE_ATTRIBUTES	18	// u, v, r, g, b, a - per vertex
#define STRIDE_COLOUR	4			// RGBA so we can have colour and texture.
#define STRIDE_POSITION 3
#define STRIDE_NORMAL	3
#define STRIDE_UV		2
#define STRIDE_SPHERE	4			// Center (x,y,z), Radius	
#define STRIDE_ENTIRE_FACE 36		// x, y, z, u, v, x, y, z, r, g, b, a - per vertex TODO: Could think about this more.
#define STRIDE_MESH_TRANSFORM 9		// Position, Direction, Scale

// TODO: Need to think if i will differentiate between textures and no textures for this. With textures we would need the extra u,v.

#define STRIDE_PROJECTED_FACE 21	// x,y,w,r,g,b,a per vertex

// Define indices for the components of a vertex of an ENTIRE_FACE.
// Not sure if this is necessary but could be helpful.
// TODO: These need a prefix like ENTIRE_FACE_INDEX_X. or just remove.
#define INDEX_X 0
#define INDEX_Y 1
#define INDEX_Z 2
#define INDEX_U 3
#define INDEX_V 4
#define INDEX_NX 5
#define INDEX_NY 6
#define INDEX_NZ 7
#define INDEX_R 8
#define INDEX_G 9
#define INDEX_B 10
#define INDEX_A 11

// This struct contains arrays for all mesh data at each stage of the rendering pipeline.
// Where indexed rendering is used, transformations are performed on all vertices at once,
// resulting in less transformations overall. This means we need arrays to store the data
// at each intermediate stage.
// We also need data like the number of faces and components used in a mesh due to the flat
// arrays of data.
typedef struct
{	
	// Buffer sizes.
	int meshes_count;

	int positions_count;
	int colours_count;
	int uvs_count;
	int normals_count;

	int max_mesh_faces;				// The highest number of faces in a mesh, out of all the meshes. Used for the temporary clipping buffers.

	// Per mesh sizes.
	int* mesh_positions_counts;		// The model matrix transform is model specific, therefore, we must store how many positions the mesh has.
	int* mesh_normals_counts;		// The model normal matrix transform is model specific, therefore, we must store how many normals the mesh has.
	int* mesh_faces_counts;			// Number of faces.
	int* mesh_front_faces_counts;	// Number of faces that are visible to the camera.
	int* mesh_clipped_faces_counts;	// Number of faces that are visible to the camera after clipping.

	// Per mesh data.
	int* mesh_transforms_updated_flags; // Tells the renderer that the world space positions need recalculating.
	int* mesh_texture_ids;				// The texture id for each mesh.
	float* mesh_bounding_spheres;		// The bounding sphere for each mesh in view space.
	float* mesh_transforms;				// Position, Direction, Scale

	// Per vertex attributes.
	float* colours;
	float* uvs;
	float* model_space_normals;

	// Coordinate space buffers. 
	float* model_space_positions;	// Original model positions without any transforms applied.
	float* world_space_positions;	// Positions after applying the model matrix.
	float* view_space_positions;	// Positions after applying the view matrix.

	float* world_space_normals;
	float* view_space_normals;

	// Buffers defining the faces.
	int* face_position_indices;		// The indices to positions that make up the faces of each mesh, used for indexed rendering.
	int* face_normal_indices;		// The indices to normals that make up the faces of each mesh, used for indexed rendering.
	float* face_attributes;			// An interleaved buffer of the per vertex data { u, v, r, g, b, a} for each vertex of each face.

	// Buffers used for backface culling.
	float* front_faces;				// An invertleaved buffer of {x, y, z, u, v, x, y, z, r, g, b, a} for each vertex of each front face. 

	// Buffers used for frustum culling.
	float* clipped_faces;			// An invertleaved buffer of {x, y, z, u, v, x, y, z, r, g, b, a} for each vertex of each face after frustum culling.
	float* temp_clipped_faces_in;	// Used for temporarily storing the faces whilst clipping against multiple planes.
	float* temp_clipped_faces_out;	// Used for temporarily storing the faces whilst clipping against multiple planes.

	// Buffers used for screen space clipping.
	float* projected_clipped_faces;	// x,y,w,r,g,b,a per vertex, TODO: UVs.
	float* projected_clipped_faces_temp;
	
} Meshes;


void parse_obj_counts(FILE* file, int* num_vertices, int* num_uvs, int* num_normals, int* num_faces);

// Load a static mesh from a .obj file, the transforms will be applied to the vertices and will be unchangeable.
void load_mesh_from_obj(Meshes* meshes, const char* file, const V3 position, const V3 orientation, const V3 scale);
void free_meshes(Meshes* meshes);


#endif