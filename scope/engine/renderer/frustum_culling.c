#include "frustum_culling.h"

#include "maths/utils.h"

#include <string.h>

void view_frustum_init(ViewFrustum* view_frustum, float near_dist, float far_dist, float fov, float aspect_ratio)
{
	// Reset the struct.
	memset(view_frustum, 0, sizeof(ViewFrustum));

	// Calculate the dimensions of the near and far planes.
	float double_tanf_half_fov = 2.f * tanf(radians(fov) / 2.f);

	float near_height = double_tanf_half_fov * near_dist;
	float near_width = near_height * aspect_ratio;

	float far_height = double_tanf_half_fov * far_dist;
	float far_width = far_height * aspect_ratio;

	// Define the forward direction.
	// TODO: Should these be defined globally somewhere? or at least in a function? like v3_world_up?
	// TODO: Also these are really world up as well. Not specific to view space. 
	// TODO: Should be defined in some engine_globals.h maybe.
	V3 view_forward = { 0, 0, -1.f };
	V3 view_up = { 0, 1.f, 0 };
	V3 view_right = { 1.f, 0, 0 };

	// Define offsets for the near and far planes.
	V3 near_centre;
	v3_mul_f_out(view_forward, near_dist, near_centre);

	V3 near_top_offset;
	v3_mul_f_out(view_up, near_height * 0.5f, near_top_offset);

	V3 near_right_offset;
	v3_mul_f_out(view_right, near_width * 0.5f, near_right_offset);

	V3 far_centre;
	v3_mul_f_out(view_forward, far_dist, far_centre);

	V3 far_top_offset;
	v3_mul_f_out(view_up, far_height * 0.5f, far_top_offset);

	V3 far_right_offset;
	v3_mul_f_out(view_right, far_width * 0.5f, far_right_offset);

	// To calculate each plane normal, we can define 3 points on each plane
	// and use the cross product of the edges. So define the four corners
	// of each plane.
	V3 near_top_left;
	v3_add_v3_out(near_centre, near_top_offset, near_top_left);
	v3_sub_v3(near_top_left, near_right_offset);

	V3 near_top_right;
	v3_add_v3_out(near_centre, near_top_offset, near_top_right);
	v3_add_v3(near_top_right, near_right_offset);

	V3 near_bottom_left;
	v3_sub_v3_out(near_centre, near_top_offset, near_bottom_left);
	v3_sub_v3(near_bottom_left, near_right_offset);

	V3 near_bottom_right;
	v3_sub_v3_out(near_centre, near_top_offset, near_bottom_right);
	v3_add_v3(near_bottom_right, near_right_offset);

	V3 far_top_left;
	v3_add_v3_out(far_centre, far_top_offset, far_top_left);
	v3_sub_v3(far_top_left, far_right_offset);

	V3 far_top_right;
	v3_add_v3_out(far_centre, far_top_offset, far_top_right);
	v3_add_v3(far_top_right, far_right_offset);

	V3 far_bottom_left;
	v3_sub_v3_out(far_centre, far_top_offset, far_bottom_left);
	v3_sub_v3(far_bottom_left, far_right_offset);

	V3 far_bottom_right;
	v3_sub_v3_out(far_centre, far_top_offset, far_bottom_right);
	v3_add_v3(far_bottom_right, far_right_offset);

	// Near and far are trivial to define.
	Plane near =
	{
		.point = { near_centre[0], near_centre[1], near_centre[2] },
		.normal = { 0, 0, -1.f}
	};

	Plane far =
	{
		.point = { far_centre[0], far_centre[1], far_centre[2] },
		.normal = { 0, 0, 1.f}
	};

	// Define the left/right planes, opposite x direction.
	V3 right_normal, e0, e1;
	v3_sub_v3_out(far_top_right, near_bottom_right, e0);
	v3_sub_v3_out(near_top_right, near_bottom_right, e1);
	cross(e1, e0, right_normal);
	normalise(right_normal);

	Plane right = { 0 };
	v3_copy(near_top_right, right.point);
	v3_copy(right_normal, right.normal);

	// Define the left plane.
	Plane left = { 0 };
	v3_copy(near_top_left, left.point);
	v3_copy(right_normal, left.normal);
	left.normal[0] *= -1;

	// Define the top/bottom planes, opposite y direction.
	v3_sub_v3_out(far_top_right, far_top_left, e0);
	v3_sub_v3_out(near_top_left, far_top_left, e1);
	V3 top_normal;
	cross(e0, e1, top_normal);
	normalise(top_normal);

	Plane top = { 0 };
	v3_copy(near_top_left, top.point);
	v3_copy(top_normal, top.normal);

	Plane bottom = { 0 };
	v3_copy(near_bottom_left, bottom.point);
	v3_copy(top_normal, bottom.normal);
	bottom.normal[1] *= -1;
	
	// As we're not doing screen space clipping, all are necessary except far,
	// however, if we're rendering stuff far away, far is a great optimisation.
	// Also, there is almost no real cost to enabling far even if we're not 
	// rendering anything far away because it's just a broad phase check that
	// will fail.
	view_frustum->planes[view_frustum->planes_count++] = near;
	view_frustum->planes[view_frustum->planes_count++] = far;
	view_frustum->planes[view_frustum->planes_count++] = right;
	view_frustum->planes[view_frustum->planes_count++] = left;
	view_frustum->planes[view_frustum->planes_count++] = top;
	view_frustum->planes[view_frustum->planes_count++] = bottom;
}