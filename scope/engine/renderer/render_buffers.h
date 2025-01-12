#ifndef RENDER_BUFFERS_H
#define RENDER_BUFFERS_H

#include "strides.h"

#include "utils/memory_utils.h"

#include "common/status.h"

#include <string.h>
#include <math.h>

typedef struct
{
	// TODO: Eventually move intermediate buffers out of models to here.
	

	// Counts for helping with resizing.
	int mbs_max_faces;
	int lights_count; // TODO: Shadow casting lights only?
	int total_faces;



	
	float* light_space_positions; // Contains vertex positions in light space.
	float* front_face_light_space_positions;

	// Contains the data for rendering to triangles.
	//float* temp_light_space_positions;

	// Temporary buffer for drawing a triangle.
	float* triangle_vertices;
	
} RenderBuffers;

inline Status render_buffers_init(RenderBuffers* rbs)
{
	memset(rbs, 0, sizeof(RenderBuffers));

	return STATUS_OK;
}

inline Status render_buffers_resize(RenderBuffers* rbs)
{
	// Realloc fails if the size is 0, just leave the arrays as is,
	// we don't really care about shrinking them anyways.
	if (rbs->mbs_max_faces == 0 || rbs->lights_count == 0)
	{
		return STATUS_OK;
	}


	// TODO: Refactor, should make this only resize if necessary? Probably doesn't matter as won't be called too often.

	// TODO: TEMP: Resizing render buffer for storing light stuff.



	// TODO: Surely this is gonna take too much memory. May have to refactor some of the rendering code.
	// TEMP: 4 for 4 vertices, allowing for space when we split triangles for v4.
	// TODO: TEMP: Also 4 for x,y,z,w
	const int MAX_TRIS_FACTOR = (int)pow(2, 6);
	


	// TODO: CALCULATE THE SIZE OF THE STRIDE PROPERLY.
	Status status = resize_float_buffer(&rbs->tri_vertex_buffer, MAX_TRIS_FACTOR * rbs->mbs_max_faces * 4 * 4 * rbs->lights_count);


	
	resize_float_buffer(&rbs->light_space_positions, rbs->total_faces * STRIDE_FACE_VERTICES * rbs->lights_count * STRIDE_V4); 
	resize_float_buffer(&rbs->front_face_light_space_positions, rbs->total_faces * STRIDE_FACE_VERTICES * rbs->lights_count * STRIDE_V4);




	// Pos (V4), UV (V2), albedo (V3), diffuse (V3)
	// 4 vertices to allow for the split.
	const int vertex_components = (12 + rbs->lights_count * STRIDE_V4) * 4;
	resize_float_buffer(&rbs->triangle_vertices, vertex_components);

	return status;
}

#endif