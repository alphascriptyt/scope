#include "lights.h"

#include "utils/memory_utils.h"

void create_point_light(PointLights* lights, const V3 position, const V3 colour, float strength)
{
	// Resize the view positions buffers.
	// TODO: I Would like to use STRIDE_POSITION without importing models.
	resize_float_buffer(&lights->world_space_positions, lights->count * 3);
	resize_float_buffer(&lights->view_space_positions, lights->count * 3);

	// Copy the lights position.
	int i = lights->count * 3;
	lights->world_space_positions[i] = position[0];
	lights->world_space_positions[++i] = position[1];
	lights->world_space_positions[++i] = position[2];

	// Resize the point lights buffer.
	int old_size = lights->count * STRIDE_POINT_LIGHT_ATTRIBUTES;
	resize_float_buffer(&lights->attributes, old_size + STRIDE_POINT_LIGHT_ATTRIBUTES);

	// Copy the point light's attributes across.
	lights->attributes[old_size] = colour[0];
	lights->attributes[++old_size] = colour[1];
	lights->attributes[++old_size] = colour[2];
	lights->attributes[++old_size] = strength;

	++lights->count;
}
