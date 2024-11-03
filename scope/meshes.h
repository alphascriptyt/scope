#ifndef MESHES_H
#define MESHES_H

#include "maths/vector3.h"

// Define the size of strides for the arrays.
#define STRIDE_FACE_POSITIONS	3
#define STRIDE_FACE_ATTRIBUTES	27	// u, v, x, y, z, r, g, b, a - per vertex TODO: Could separate normals out.
#define STRIDE_COLOUR	4			// RGBA so we can have colour and texture.
#define STRIDE_POSITION 3
#define STRIDE_NORMAL	3
#define STRIDE_UV		2
#define STRIDE_SPHERE	4			// Center (x,y,z), Radius	
#define STRIDE_ENTIRE_FACE 36	// x, y, z, u, v, x, y, z, r, g, b, a - per vertex TODO: Could think about this more.

// TODO: Need to think if i will differentiate between textures and no textures for this. With textures we would need the extra u,v.

#define STRIDE_PROJECTED_FACE 21 // x,y,w,r,g,b,a per vertex - Note, we don't need a as that is only used for combining textures and colours.

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

// For each stage in the render, we will write each stage to a buffer, this way we only
// have to apply a transformation to a vertex once due to indexed rendering.
typedef struct
{
	// TODO: Plural or not plural.
	int mesh_count;

	// Store the size of the buffers.
	int positions_count;
	int colours_count;
	int uvs_count;
	int normals_count;
	
	int* face_counts;			// Number of faces that make up the mesh
	int* front_face_counts;		// Number of faces that are visible to the camera.
	int* clipped_face_counts;	// Number of faces that are visible to the camera after clipping.
	int* mesh_texture_ids;		// The texture id for each mesh.

	float* mesh_bounding_spheres;// [MAX_MESHES * STRIDE_SPHERE] ;

	// Buffers for vertex positions at different stages of the render.
	// We only need to store the world space positions as the meshes will not move.
	float* world_space_positions;

	// Store the face vertex attributes.
	float* colours;
	float* uvs;
	float* normals;


	// Store the view spcae positions.
	float* view_space_positions;



	// Buffers defining the faces.
	int* face_position_indices; 
	float* face_attributes;				// An interleaved buffer of { u, v, x, y, z, r, g, b, a}. TODO: It could be beneficial to separate this if the attributes aren't
										// always accessed together. At first we will need these all together so could potentially split later. Not sure it's necessary.

	// After backface culling
	float* front_faces;					// An invertleaved buffer of {x, y, z, u, v, x, y, z, r, g, b, a} * 3 for each face. 

	// TODO: Clipping.
	float* clipped_faces; // An invertleaved buffer of {x, y, z, u, v, x, y, z, r, g, b, a} * 3 for each face.
	float* temp_clipped_faces_in; // Used for temporarily storing the faces whilst clipping against multiple planes.
	float* temp_clipped_faces_out; // Used for temporarily storing the faces whilst clipping against multiple planes.
	int max_mesh_faces;

	// Buffers for screen space clipping.
	float* projected_clipped_faces;	//  x,y,w,r,g,b,a per vertex, TODO: UVs.
	float* projected_clipped_faces_temp;
	

} StaticMeshes;



// Load a static mesh from a .obj file, the transforms will be applied to the vertices and will be unchangeable.
void load_static_mesh_from_obj(StaticMeshes* static_meshes, const char* file, const V3 position, const V3 orientation, const V3 scale);
void free_static_meshes(StaticMeshes* static_meshes);




#endif