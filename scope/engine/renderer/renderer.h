#ifndef RENDERER_H
#define RENDERER_H

#include "render_target.h"
#include "render_settings.h"
#include "camera.h"

#include "common/status.h"

typedef struct
{
	RenderTarget target;
	RenderSettings settings;
	Camera camera;

} Renderer;

Status renderer_init(Renderer* renderer, int width, int height);
Status renderer_resize(Renderer* renderer, int width, int height);

#endif