#include "frustum_culling.h"

#include "maths/utils.h"

#include <string.h>

void view_frustum_init(ViewFrustum* view_frustum, float near_dist, float far_dist, float fov, float aspect_ratio)
{
	// TODO: All 6. Could draw this in front of the camera to check the shape maybe. Remember 
	// TODO: Check that I can even allocate enough space for 1000 monkeys with 6 planes. (if not, do i need to draw straight away.
	// TODO: If i can clip against all 5 (far doesnt matetr), I will be able to stop screen space clipping.


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



	// TODO: Cleanup all this code and add the rest of the planes to clip against, again, if this 
	//		 is going to be a slowdown, we can comment them out. But I should at least get them working.
	//		 left right and near working.

	// TODO: Actually, clipping still seems to break with 120 fov. Not sure why. look into how normals
	//		 are calculated agian.

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


	// TODO: YOU IDIOT. THE PLANE POINTS ARE NOT THE NEAR CENTRE ARGHHHHHHHH THE NEAR PLANE IS A SQUARE............
	// Although there is still something going wrong with the normals. and directions in the program, must fix 
	// the model directions first i think.
	//Plane right = {
		//.normal = { -0.707, 0, -0.707 }
	//};
	Plane right = { 0 };
	v3_copy(near_top_right, right.point);
	v3_copy(nr, right.normal);
	normalise(right.normal);


	v3_sub_v3_out(far_top_left, near_bottom_left, e0);
	v3_sub_v3_out(near_top_left, near_bottom_left, e1);
	V3 left_normal;
	cross(e0, e1, left_normal);
	
	Plane left = { 0 };
	v3_copy(near_top_left, left.point);
	v3_copy(left_normal, left.normal);
	normalise(left.normal);

	/*
	{
		.point = { near_top_right[0]},
		//.normal = { nr[0], nr[1], nr[2] }
		
	};*/

	
	/*
	Plane left =
	{
		.point = { near_centre[0], near_centre[1], near_centre[2] },
		.normal = { -0.707, 0, -0.707 }
	};*/

	Plane top =
	{
		.point = { near_centre[0], near_centre[1], near_centre[2] },
		.normal = { 0.f, 0.866f, -0.5 }
	};

	Plane bottom =
	{
		.point = { near_centre[0], near_centre[1], near_centre[2] },
		.normal = { 0.f, -0.866f, -0.5 }
	};

	
	view_frustum->planes[view_frustum->num_planes++] = near;
	//view_frustum->planes[view_frustum->num_planes++] = far;
	//view_frustum->planes[view_frustum->num_planes++] = right;
	//view_frustum->planes[view_frustum->num_planes++] = left;
	//view_frustum->planes[view_frustum->num_planes++] = top;
	//view_frustum->planes[view_frustum->num_planes++] = bottom;


	


}