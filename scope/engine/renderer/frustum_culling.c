#include "frustum_culling.h"

#include "maths/utils.h"

#include <string.h>

/*
void create_clipping_view_frustum(float near_plane_dist, float fov, float aspect_ratio, ViewFrustum* view_frustum)
{
	view_frustum->num_planes = 0;

	Plane near = { 
		.normal = {0, 0, -1} 
	};

	v3_mul_f_out(near.normal, near_plane_dist, near.point);
	

	// TODO: http://davidlively.com/programming/graphics/frustum-calculation-and-culling-hopefully-demystified/
	// Use these calcs ^^, my ones below don't work for different FOVs.
	// TODO: THIS DOESN@T WORK. BUT NEITHER DOES MY CALCS. LOOK INTO THIS.

	// TODO: must calculate based off of FOV, actually just check this is working.
	// TODO: Check the negatives.
	float hh = tanf(radians(fov) / 2.f) * -near_plane_dist;
	float hw = hh * aspect_ratio;


	// TODO: WHAT SHOULD THIS ACTUALLY BE? IF WE'RE CLIPPING IN VIEW SPACE.
	// SURELY THIS CHANGES???
	// Calculate the 4 corners of the near plane.
	V3 nw = { -hw, hh, -near_plane_dist };
	V3 ne = { hw, -hh, -near_plane_dist };
	V3 se = { hw, -hh, -near_plane_dist };
	V3 sw = { -hw, -hh, -near_plane_dist };

	// Calculate the planes.
	//V3 left_normal = { -hw, 0, -near_plane_dist };
	V3 left_normal;



	//cross(se, ne, left_normal);
	normalise(left_normal);

	// TODO: This copying is annoying.
	Plane left = {
		.normal = {left_normal[0], left_normal[1], left_normal[2] },
		.point = {near.point[0], near.point[1], near.point[2] }
	};

	V3 right_normal;// = { hw, 0, -near_plane_dist };
	cross(sw, nw, right_normal);
	normalise(right_normal);

	Plane right = {
		.normal = {right_normal[0], right_normal[1], right_normal[2] },
		.point = {near.point[0], near.point[1], near.point[2] }
	};

	V3 top_normal;// = { 0, hh, -near_plane_dist };
	cross(nw, ne, top_normal);
	normalise(top_normal);

	Plane top = {
		.normal = {top_normal[0], top_normal[1], top_normal[2] },
		.point = {near.point[0], near.point[1], near.point[2] }
	};

	V3 bottom_normal;// = { 0, -hh, -near_plane_dist };
	cross(sw, se, bottom_normal);
	normalise(bottom_normal);

	Plane bottom = {
		.normal = {bottom_normal[0], bottom_normal[1], bottom_normal[2] },
		.point = {near.point[0], near.point[1], near.point[2] }
	};
	
	view_frustum->planes[view_frustum->num_planes++] = near;
	view_frustum->planes[view_frustum->num_planes++] = left;
	view_frustum->planes[view_frustum->num_planes++] = right;
	view_frustum->planes[view_frustum->num_planes++] = top;
	view_frustum->planes[view_frustum->num_planes++] = bottom;
}
*/

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

	V3 a;
	v3_add_v3_out(near_centre, near_right_offset, a);
	normalise(a);

	V3 nr;
	//cross(up, a, nr);
	//nr[0] = -near_dist;
	//nr[1] = 0;
	//nr[2] = -near_width;

	V3 e0;
	v3_sub_v3_out(far_top_right, near_bottom_right, e0);
	V3 e1;
	v3_sub_v3_out(near_top_right, near_bottom_right, e1);
	
	cross(e1, e0, nr);

	normalise(nr);



	Plane near =
	{
		.point = { near_centre[0], near_centre[1], near_centre[2] },
		.normal = { 0, 0, -1.f}
	};


	// TODO: YOU IDIOT. THE PLANE POINTS ARE NOT THE NEAR CENTRE ARGHHHHHHHH THE NEAR PLANE IS A SQUARE............
	// Although there is still something going wrong with the normals. and directions in the program, must fix 
	// the model directions first i think.
	Plane right = {
		.normal = { -1, 0, -0.5 }
	};
	v3_copy(near_top_right, right.point);

	


	/*
	{
		.point = { near_top_right[0]},
		//.normal = { nr[0], nr[1], nr[2] }
		
	};*/

	normalise(right.normal);

	Plane left =
	{
		.point = { near_centre[0], near_centre[1], near_centre[2] },
		.normal = { -0.707, 0, -0.707 }
	};

	Plane top =
	{
		.point = { near_centre[0], near_centre[1], near_centre[2] },
		.normal = { 0, 0.866, -0.5 }
	};

	Plane bottom =
	{
		.point = { near_centre[0], near_centre[1], near_centre[2] },
		.normal = { 0, -0.866, -0.5 }
	};

	
	view_frustum->planes[view_frustum->num_planes++] = near;
	//view_frustum->planes[view_frustum->num_planes++] = right;
	//view_frustum->planes[view_frustum->num_planes++] = left;
	//view_frustum->planes[view_frustum->num_planes++] = top;
	//view_frustum->planes[view_frustum->num_planes++] = bottom;


	


}