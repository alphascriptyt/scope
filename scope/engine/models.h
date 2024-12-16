#ifndef MODELS_H
#define MODELS_H

#include "common/status.h"
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
#define STRIDE_MI_TRANSFORM 9		// Position, Eulers, Scale

// TODO: Need to think if i will differentiate between textures and no textures for this. With textures we would need the extra u,v.

#define STRIDE_PROJECTED_FACE 24	// x,y,z,w,r,g,b,a per vertex

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

/*

ModelBase
- Defines object space data: positions, normals, uvs.
- Read from .obj
- Used to create instances.

ModelInstance
- Created from a ModelBase.
- Defines a transform: Position, Direction, Scale.
- Defines visual stuff like colours and textures (although uvs from base).

There are also intermediate buffers to store indexed results for less overall
computations. Also, these can be used by other engine components, like the 
physics system, to avoid recomputation of world space positions etc.

*/

/*

TODO: 

One day, I reckon I will need models to be made up of meshes. Where each mesh can be 
moved seperately. Maybe this is something that would be abstracted from the render functions
I'm not sure. I will need to think about it properly. But let's say we had a basic character
and wanted to do a running animation, it would be nice to be able to animate it's bodyparts,
for example, we could store transforms for parts of the animation and lerp each mesh in the 
model for those. Or reloading etc. This will be quite important.

*/

typedef struct
{	
	// TODO: Comments and naming. Currently mb = model base, mi = model instance

	// Buffer sizes.
	int mbs_count;
	int mis_count;
	int max_mb_faces;					// The highest number of faces in a mesh, out of all the models. Used for the temporary clipping buffers.

	// ModelBase data
	int mbs_total_faces;				// The total number of faces defined by all mbs.
	int mbs_total_positions;
	int mbs_total_normals;
	int mbs_total_uvs;

	int* mbs_faces_offsets;
	int* mbs_positions_offsets;
	int* mbs_normals_offsets;
	int* mbs_uvs_offsets;

	int* mbs_positions_counts;			// The model matrix transform is model specific, therefore, we must store how many positions the mesh has.
	int* mbs_normals_counts;			// The model normal matrix transform is model specific, therefore, we must store how many normals the mesh has.
	int* mbs_faces_counts;				// Number of faces in the model.
	int* mbs_uvs_counts;

	// TODO: Get rid of face prefix??
	int* mbs_face_position_indices;		// The indices to positions that make up the faces, used for indexed rendering.
	int* mbs_face_normal_indices;		// The indices to normals that make up the faces, used for indexed rendering.
	int* mbs_face_uvs_indices;

	float* mbs_object_space_positions;	// Original vertex positions without any transforms applied.
	float* mbs_object_space_normals;
	float* mbs_uvs;
	float* mbs_object_space_centres;

	// Instance data
	// TODO: Naming.
	int mis_total_faces;				// Total number of faces from all mis, keeps track of the size of the buffers.
	int mis_total_positions;
	int mis_total_normals;

	int* mis_base_ids;					// The id of the model base.
	int* mis_texture_ids;				// The id of the texture.
	int* mis_dirty_bounding_sphere_flags;	// If a mi's scale has changed, the bounding sphere centre needs to be recalculated.
	int* mis_intersected_planes;		// For each mi, the number of planes intersected, then the indices of the planes.
	int* mis_passed_broad_phase_flags;	// Whether the mi is visible after broad phase culling. TODO: Name.

	float* mis_vertex_colours;			// Per vertex colours for the instances.
	float* mis_transforms;				// The instance world space transforms: [ Position, Direction, Scale ]
	float* mis_bounding_spheres;		// The bounding sphere for each instance in world space.

	// Transform results buffers.
	// TODO: These are specific to mis, should prefix.
	float* view_space_positions;
	float* view_space_normals;
	
	// Backface culling buffers.
	int* front_faces_counts;		// Number of faces that are visible to the camera.
	float* front_faces;				// An invertleaved buffer of {x, y, z, u, v, x, y, z, r, g, b, a} for each vertex of each front face after backface culling.

	// Buffers used for frustum culling.
	float* clipped_faces;			// An invertleaved buffer of {x, y, z, u, v, x, y, z, r, g, b, a} for each vertex of each face after frustum culling.
	float* temp_clipped_faces_in;	// Used for temporarily storing the faces whilst clipping against multiple planes.
	float* temp_clipped_faces_out;	// Used for temporarily storing the faces whilst clipping against multiple planes.

} Models;

// Initialises the models struct.
void models_init(Models* models);

// Parses the obj file for the number of each component.
void parse_obj_counts(FILE* file, int* num_vertices, int* num_uvs, int* num_normals, int* num_faces);

void load_model_base_from_obj(Models* models, const char* filename);

// TODO: It would be nice to be able to create different model
//		 instances without memory allocating each time. I think
//		 allocating a bigger pool of memory would be nice, then
//		 we can allocate when we need more capacity. So we would
//		 have like a capacity for each buffer size.

// TODO: For this, I can look into memory arenas: https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator
//		 But we don't need this for now. Essentially just allocate big blocks, store the used and total capacity etc.

// Allocates memory for n instances of the ModelBase at mb_index.
void create_model_instances(Models* models, int mb_index, int n);

void free_models(Models* models);

#endif