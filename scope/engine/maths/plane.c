#include "plane.h"

#include "vector3.h"

#include <stdio.h>

float signed_distance(const Plane* plane, const V3 point)
{
	// TODO: Store this in the plane struct somewhere?
	float d = -dot(plane->normal, plane->point);
	return dot(point, plane->normal) + d;
}

float line_intersect_plane(const V3 v0, const V3 v1, const Plane* plane, V3 out)
{
	// This uses the fact that a plane can be expressed as the set of points p for which Dot((p - p0), n) = 0
	V3 ray;
	v3_sub_v3_out(v1, v0, ray);

	float normalDotRay = dot(plane->normal, ray);

	if (normalDotRay == 0)
	{
		// TODO: Parallel, no intersection. Should test if this ever actually happens.
		// It shouldn't happen with how we use this. Could just document that it doesn't handle the lines not intersecting.
		printf("normal_dot_ray == 0. Should not happen\n");
	}

	V3 v0_to_plane_point;
	v3_sub_v3_out(v0, plane->point, v0_to_plane_point);

	// Calculate the time of intersection.
	float t = -(dot(plane->normal, v0_to_plane_point)) / normalDotRay;
	
	// Interpolate for the point of intersection.
	v3_mul_f_out(ray, t, out);
	v3_add_v3(out, v0);

	return t;
}