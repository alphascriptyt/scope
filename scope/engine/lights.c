#include "lights.h"

#include "utils/memory_utils.h"

#include <string.h>

void point_lights_init(PointLights* point_lights)
{
	memset(point_lights, 0, sizeof(PointLights));
}

void point_lights_create(PointLights* point_lights, V3 position, V3 colour, float strength)
{
	const int new_count = point_lights->count + 1;

	// Resize the view positions buffers.
	// TODO: I Would like to use STRIDE_POSITION without importing models.h.
	resize_float_buffer(&point_lights->world_space_positions, new_count * 3);
	resize_float_buffer(&point_lights->view_space_positions, new_count * 3);

	// Copy the lights position.
	int i = point_lights->count * 3;
	point_lights->world_space_positions[i] = position.x;
	point_lights->world_space_positions[++i] = position.y;
	point_lights->world_space_positions[++i] = position.z;

	// Resize the point lights buffer.
	int old_size = point_lights->count * STRIDE_POINT_LIGHT_ATTRIBUTES;
	resize_float_buffer(&point_lights->attributes, old_size + STRIDE_POINT_LIGHT_ATTRIBUTES);

	// Copy the point light's attributes across.
	point_lights->attributes[old_size] = colour.x;
	point_lights->attributes[++old_size] = colour.y;
	point_lights->attributes[++old_size] = colour.z;
	point_lights->attributes[++old_size] = strength;

	// TODO: Create shadow maps.
	//		 All temporary for now.
	// TODO: No idea what size would be best.
	const int RES = 128;
	depth_buffer_init(&point_lights->depth_maps[0], RES, RES);
	depth_buffer_fill(&point_lights->depth_maps[0], 1.f);

	point_lights->count = new_count;
}
