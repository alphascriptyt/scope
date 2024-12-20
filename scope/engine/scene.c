#include "scene.h"

#include "utils/logger.h"

#include <string.h>

Status scene_init(Scene* scene)
{
	memset(scene, 0, sizeof(Scene));

	models_init(&scene->models);
	point_lights_init(&scene->point_lights);

	scene->ambient_light[0] = 0.1f;
	scene->ambient_light[1] = 0.1f;
	scene->ambient_light[2] = 0.1f;

	return STATUS_OK;
}

Status scene_destroy(Scene* scene)
{
	// TODO: Cleanup
	log_warn("Need to implement scene_destroy.");
	return STATUS_OK;
}
