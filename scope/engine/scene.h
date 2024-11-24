#ifndef SCENE_H
#define SCENE_H

#include "models.h"
#include "lights.h"

#include "common/status.h"

// A scene is essentially a wrapper for models and lights.
typedef struct
{
	Models models;
	PointLights point_lights;

} Scene;

Status scene_init(Scene* scene);
Status scene_destroy(Scene* scene);

#endif