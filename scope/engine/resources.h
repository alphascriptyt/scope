#ifndef RESOURCES_H
#define RESOURCES_H

#include "canvas.h"

#include "common/status.h"
#include "utils/logger.h"

#include <string.h>
#include <stdlib.h>

typedef struct
{
	Canvas* textures;
	int textures_count;

} Resources;

inline void resources_init(Resources* resources)
{
	memset(resources, 0, sizeof(Resources));
}

inline Status resources_load_texture(Resources* resources, const char* file)
{
	// Get the texture's index.
	int i = resources->textures_count;

	// Make room for the new texture.
	Canvas* textures_temp = realloc(resources->textures, (resources->textures_count + 1) * sizeof(Canvas));
	if (!textures_temp)
	{
		log_error("Failed to resources_load_texture because of %s.\n", status_to_str(STATUS_ALLOC_FAILURE));
		return STATUS_ALLOC_FAILURE;
	}
	resources->textures = textures_temp;

	// Try load the texture.
	Status status = canvas_init_from_bitmap(&textures_temp[i], file);
	if (STATUS_OK != status)
	{
		log_error("Failed to resources_load_texture because of %s.\n", status_to_str(status));
		return status;
	}

	// Successfully loaded the texture.
	++resources->textures_count;
	return STATUS_OK;
}

// TODO: Delete texture?

#endif