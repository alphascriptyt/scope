#ifndef FRUSTUM_CULLING_H
#define FRUSTUM_CULLING_H

#include "maths/plane.h"

#define MAX_FRUSTUM_PLANES 5

typedef struct
{
	Plane planes[MAX_FRUSTUM_PLANES];
	int num_planes;

} ViewFrustum;

void create_clipping_view_frustum(float near_plane_dist, float fov, float aspect_ratio, ViewFrustum* view_frustum);


#endif