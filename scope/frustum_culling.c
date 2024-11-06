#include "frustum_culling.h"

#include "maths/utils.h"

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
	cross(ne, se, left_normal);
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
	//view_frustum->planes[view_frustum->num_planes++] = left;
	//view_frustum->planes[view_frustum->num_planes++] = right;
	//view_frustum->planes[view_frustum->num_planes++] = top;
	//view_frustum->planes[view_frustum->num_planes++] = bottom;
}