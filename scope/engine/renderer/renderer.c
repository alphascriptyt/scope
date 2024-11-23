#include "renderer.h"

Status renderer_init(Renderer* renderer, int width, int height)
{
	// Initialise the render target.
	Status status = render_target_init(&renderer->target, width, height);
	if (STATUS_OK != status)
	{
		return status;
	}

	// Initialise the render settings.
	memset(&renderer->settings, 0, sizeof(RenderSettings));

	renderer->settings.fov = 60.f;
	renderer->settings.near_plane = 1.f;
	renderer->settings.far_plane = 100.f;

	update_projection_m4(&renderer->settings, width / (float)height);

	// Create a camera.
	memset(&renderer->camera, 0, sizeof(Camera));
	v3_init(renderer->camera.direction, 0, 0, -1.f);
	v3_init(renderer->camera.position, 0, 0, 10.f);
	renderer->camera.yaw = PI;

	return STATUS_OK;
}

Status renderer_resize(Renderer* renderer, int width, int height)
{
	// Resize the render target.
	Status status = render_target_resize(&renderer->target, width, height);
	if (STATUS_OK != status)
	{
		return status;
	}

	// Update the projection matrix.
	update_projection_m4(&renderer->settings, width / (float)height);

	return STATUS_OK;
}
