#include "lights.h"

#include "utils/memory_utils.h"

#include <string.h>

void point_lights_init(PointLights* point_lights)
{
	memset(point_lights, 0, sizeof(PointLights));
}

void point_lights_create(PointLights* point_lights, const V3 position, const V3 colour, float strength)
{
	// Resize the view positions buffers.
	// TODO: I Would like to use STRIDE_POSITION without importing models.h.
	resize_float_buffer(&point_lights->world_space_positions, point_lights->count * 3);
	resize_float_buffer(&point_lights->view_space_positions, point_lights->count * 3);

	// Copy the lights position.
	int i = point_lights->count * 3;
	point_lights->world_space_positions[i] = position[0];
	point_lights->world_space_positions[++i] = position[1];
	point_lights->world_space_positions[++i] = position[2];

	// Resize the point lights buffer.
	int old_size = point_lights->count * STRIDE_POINT_LIGHT_ATTRIBUTES;
	resize_float_buffer(&point_lights->attributes, old_size + STRIDE_POINT_LIGHT_ATTRIBUTES);

	// Copy the point light's attributes across.
	point_lights->attributes[old_size] = colour[0];
	point_lights->attributes[++old_size] = colour[1];
	point_lights->attributes[++old_size] = colour[2];
	point_lights->attributes[++old_size] = strength;

	++point_lights->count;
}
