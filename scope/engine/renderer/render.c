#include "render.h"

#include "render_target.h"
#include "draw_2d.h"

#include "common/colour.h"

#include "maths/matrix4.h"
#include "maths/vector4.h"
#include "maths/vector3.h"
#include "maths/utils.h"

#include "frustum_culling.h"

#include "utils/timer.h"
#include "utils/common.h"

#include "globals.h"

#include "resources.h"

#include <stdio.h>
#include <string.h>

void debug_draw_point_lights(Canvas* canvas, const RenderSettings* settings, PointLights* point_lights)
{
	// Debug draw point light icons as rects.
	for (int i = 0; i < point_lights->count; ++i)
	{
		int idx_vsp = i * STRIDE_POSITION;
		V4 p = {
			point_lights->view_space_positions[idx_vsp],
			point_lights->view_space_positions[++idx_vsp],
			point_lights->view_space_positions[++idx_vsp],
			1.f
		};

		// Only draw if depth is visibile in clip space.
		if (p[2] > -settings->near_plane)
		{
			continue;
		}

		V4 projected;
		project(canvas, settings->projection_matrix, p, projected);

		int idx_attr = i * STRIDE_POINT_LIGHT_ATTRIBUTES;
		int colour = float_rgb_to_int(point_lights->attributes[idx_attr], point_lights->attributes[idx_attr + 1], point_lights->attributes[idx_attr + 2]);

		// Scale the radius so it's at a maximum of 10.
		const float radius = 10.f * (-settings->near_plane / p[2]); // Square radius nice.

		int x0 = (int)(projected[0] - radius);
		int x1 = (int)(projected[0] + radius);

		int y0 = (int)(projected[1] - radius);
		int y1 = (int)(projected[1] + radius);

		draw_rect(canvas, x0, y0, x1, y1, colour);
	}
}

void debug_draw_bounding_spheres(Canvas* canvas, const RenderSettings* settings, const Models* models, const M4 view_matrix)
{
	// TODO: This doesn't really work.

	// TODO: For a function like this, I should be able to do debug_draw_bounding_sphere and pass in the mi index.

	int colour = int_rgb_to_int(1, 0, 0);

	for (int i = 0; i < models->mis_count; ++i)
	{
		int sphere_index = i * STRIDE_SPHERE;

		V3 view_centre_v3 = {
			models->mis_bounding_spheres[sphere_index],
			models->mis_bounding_spheres[sphere_index + 1],
			models->mis_bounding_spheres[sphere_index + 2]
		};

		debug_draw_view_space_point(canvas, settings, view_centre_v3, COLOUR_LIME);

		V4 world_centre_v4 = {
			models->mis_bounding_spheres[sphere_index],
			models->mis_bounding_spheres[sphere_index + 1],
			models->mis_bounding_spheres[sphere_index + 2],
			1
		};

		V4 view_centre, view_top, view_bottom;

		m4_mul_v4(view_matrix, world_centre_v4, view_centre);

		if (view_centre[2] > -settings->near_plane)
		{
			continue;
		}

		float radius = models->mis_bounding_spheres[3];

		V4 world_bottom;
		v4_copy(world_centre_v4, world_bottom);
		world_bottom[1] -= radius;

		m4_mul_v4(view_matrix, world_bottom, view_bottom);

		V4 world_top;
		v4_copy(world_centre_v4, world_top);
		world_top[1] += radius;

		m4_mul_v4(view_matrix, world_top, view_top);

		V4 pc, pb, pt;
		project(canvas, settings->projection_matrix, view_centre, pc);
		project(canvas, settings->projection_matrix, view_top, pt);
		project(canvas, settings->projection_matrix, view_bottom, pb);
		
		float pr = fabsf(pb[1] - pt[1]) / 2.f;
		
		draw_circle(canvas, (int)pc[0], (int)pc[1], (int)pr, colour);
	}
}

void debug_draw_world_space_point(Canvas* canvas, const RenderSettings* settings, const V3 point, const M4 view_matrix, int colour)
{
	// Convert from world space to screen space.
	V4 wsp = { point[0], point[1], point[2], 1 };
	V4 vsp, ssp;

	m4_mul_v4(view_matrix, wsp, vsp);
	
	// Don't draw points behind the camera.
	if (vsp[2] > -settings->near_plane) 
	{
		return;
	}

	project(canvas, settings->projection_matrix, vsp, ssp);


	// TODO: Could be a draw 2d rect function.
	int n = 2;
	int y0 = (int)(ssp[1] - n);
	int y1 = (int)(ssp[1] + n);
	int x0 = (int)(ssp[0] - n);
	int x1 = (int)(ssp[0] + n);

	draw_rect(canvas, x0, y0, x1, y1, colour);
}

void debug_draw_view_space_point(Canvas* canvas, const RenderSettings* settings, const V3 point, int colour)
{
	// Convert from world space to screen space.
	V4 vsp = { point[0], point[1], point[2], 1 };
	V4 ssp;

	// Don't draw points behind the camera.
	if (vsp[2] > -settings->near_plane)
	{
		return;
	}

	project(canvas, settings->projection_matrix, vsp, ssp);

	// TODO: Could be a draw 2d rect function.
	int n = 2;
	int y0 = (int)(ssp[1] - n);
	int y1 = (int)(ssp[1] + n);
	int x0 = (int)(ssp[0] - n);
	int x1 = (int)(ssp[0] + n);

	draw_rect(canvas, x0, y0, x1, y1, colour);
}

void debug_draw_world_space_line(Canvas* canvas, const RenderSettings* settings, const M4 view_matrix, const V3 v0, const V3 v1, const V3 colour)
{
	V4 ws_v0;
	v3_to_v4_point(v0, ws_v0);

	V4 ws_v1;
	v3_to_v4_point(v1, ws_v1);

	V4 vs_v0, vs_v1;
	m4_mul_v4(view_matrix, ws_v0, vs_v0);
	m4_mul_v4(view_matrix, ws_v1, vs_v1);

	// Don't draw if behind the camera.
	if (vs_v0[2] > -settings->near_plane && vs_v1[2] > -settings->near_plane)
	{
		return;
	}

	// Clip the points to the near plane so we don't draw anything behind.
	// I think this works okay.
	if (vs_v0[2] > -settings->near_plane)
	{
		V4 between;
		v4_sub_v4_out(vs_v1, vs_v0, between);

		float dist = vs_v0[2] + settings->near_plane;
		
		float dxdz = (vs_v1[0] - vs_v0[0]) / (vs_v1[2] - vs_v0[2]);
		float dydz = (vs_v1[1] - vs_v0[1]) / (vs_v1[2] - vs_v0[2]);

		vs_v0[0] += dxdz * dist;
		vs_v0[1] += dydz * dist;
		vs_v0[2] = -settings->near_plane;
	}
	else if (vs_v1[2] > -settings->near_plane)
	{
		V4 between;
		v4_sub_v4_out(vs_v0, vs_v1, between);

		float dist = vs_v1[2] + settings->near_plane;

		float dxdz = (vs_v0[0] - vs_v1[0]) / (vs_v0[2] - vs_v1[2]);
		float dydz = (vs_v0[1] - vs_v1[1]) / (vs_v0[2] - vs_v1[2]);

		vs_v1[0] += dxdz * dist;
		vs_v1[1] += dydz * dist;
		vs_v1[2] = -settings->near_plane;
	}

	V4 ss_v0, ss_v1;
	project(canvas, settings->projection_matrix, vs_v0, ss_v0);
	project(canvas, settings->projection_matrix, vs_v1, ss_v1);

	const int colour_int = float_rgb_to_int(colour[0], colour[1], colour[2]);

	draw_line(canvas, (int)ss_v0[0], (int)ss_v0[1], (int)ss_v1[0], (int)ss_v1[1], colour_int);
}

void debug_draw_mi_normals(Canvas* canvas, const RenderSettings* settings, const Models* models, int mi_index)
{
	// TODO: How can we just access the data without this sort of loop??
	//		 If we actually need this for something other than debugging,
	//		 we should store offsets that are calculated per render call.
	int front_faces_offset = 0;
	for (int i = 0; i < mi_index; ++i)
	{
		front_faces_offset += models->front_faces_counts[i];
	}

	for (int i = front_faces_offset; i < front_faces_offset + models->front_faces_counts[mi_index]; ++i)
	{
		int face_index = i * STRIDE_ENTIRE_FACE;

		for (int j = 0; j < STRIDE_FACE_VERTICES; ++j)
		{
			int k = face_index + j * STRIDE_ENTIRE_VERTEX;
			const V3 start = {
				models->front_faces[k],
				models->front_faces[k + 1],
				models->front_faces[k + 2],
			};

			V3 normal = {
				models->front_faces[k + 5],
				models->front_faces[k + 6],
				models->front_faces[k + 7],
			};

			const float length = 0.5f;

			V3 dir;
			v3_copy(normal, dir);
			v3_mul_f(dir, length);

			V3 end = { 0,0,0 };
			v3_add_v3(end, dir);
			v3_add_v3(end, start);

			V4 start_v4, end_v4;

			v3_to_v4_point(start, start_v4);
			v3_to_v4_point(end, end_v4);

			V4 ss_start, ss_end;
			project(canvas, settings->projection_matrix, start_v4, ss_start);
			project(canvas, settings->projection_matrix, end_v4, ss_end);

			draw_line(canvas, (int)ss_start[0], (int)ss_start[1], (int)ss_end[0], (int)ss_end[1], COLOUR_LIME);
		}
	}
}

void draw_scanline(RenderTarget* rt, int x0, int x1, int y, float z0, float z1, float w0, float w1, const V3 c0, const V3 c1)
{
	// TODO: Refactor function args.
	// TODO: If not combining with a texture, we don't need the colour alpha.

	// Precalculate deltas.
	const unsigned int dx = x1 - x0;
	float inv_dx = 1.f / dx;

	float w_step = (w1 - w0) * inv_dx;

	// Offset x by the given y.
	int row_offset = rt->canvas.width * y;

	int start_x = x0 + row_offset;
	int end_x = x1 + row_offset;

	V3 c_step;
	v3_sub_v3_out(c1, c0, c_step);
	v3_mul_f(c_step, inv_dx);

	V3 c = {
		c0[0],
		c0[1],
		c0[2]
	};

	// Render the scanline
	unsigned int* pixels = rt->canvas.pixels + start_x;
	float* depth_buffer = rt->depth_buffer + start_x;

	float inv_w = w0;
	float z = z0;
	float z_step = (z1 - z0) * inv_dx;

	for (unsigned int i = 0; i < dx; ++i)
	{
		// Depth test, only draw closer values.
		if (*depth_buffer > z)
		{
			// Recover w
			const float w = 1.0f / inv_w;

			// Render as depth buffer, testing depth values
			/*
			V3 white = { 1,1,1 };
			float r = (white[0] * (1.f - z));
			float g = (white[1] * (1.f - z));
			float b = (white[2] * (1.f - z));
			*/

			const float r = (c[0] * w);
			const float g = (c[1] * w);
			const float b = (c[2] * w);

			*pixels = float_rgb_to_int(r, g, b);
			*depth_buffer = z;
		}

		// Move to the next pixel
		++pixels;
		++depth_buffer;

		// Step per pixel values.
		z += z_step;
		inv_w += w_step;
		v3_add_v3(c, c_step);
	}
}

void draw_flat_bottom_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2)
{
	// Sort the flat vertices left to right.
	float* pv1 = v1;
	float* pv2 = v2;
	float* pc1 = c1;
	float* pc2 = c2;

	// Sort the flat top left to right.
	if (v1[0] > v2[0]) 
	{  
		swap(&pv1, &pv2);
		swap(&pc1, &pc2);
	}

	float invDy = 1 / (pv2[1] - v0[1]);

	float dxdy0 = (pv1[0] - v0[0]) * invDy;
	float dxdy1 = (pv2[0] - v0[0]) * invDy;

	float dzdy0 = (pv1[2] - v0[2]) * invDy;
	float dzdy1 = (pv2[2] - v0[2]) * invDy;

	float dwdy0 = (pv1[3] - v0[3]) * invDy;
	float dwdy1 = (pv2[3] - v0[3]) * invDy;

	V3 dcdy0;
	v3_sub_v3_out(pc1, c0, dcdy0);
	v3_mul_f(dcdy0, invDy);

	V3 dcdy1;
	v3_sub_v3_out(pc2, c0, dcdy1);
	v3_mul_f(dcdy1, invDy);

	// Do I need 
	V3 start_c = {
		c0[0],
		c0[1],
		c0[2]
	};

	V3 end_c = {
		c0[0],
		c0[1],
		c0[2]
	};

	int yStart = (int)(ceil(v0[1] - 0.5f));
	int yEnd = (int)(ceil(pv2[1] - 0.5f));

	for (int y = yStart; y < yEnd; ++y) {
		// Must lerp for the vertex attributes otherwise the accuracy is poor.
		// TODO: Would be nice to not have to actually lerp but step instead.
		float a = (y + 0.5f - v0[1]);

		// Calculate the start and ends of the scanline
		float x0 = v0[0] + dxdy0 * a;
		float x1 = v0[0] + dxdy1 * a;

		float z0 = v0[2] + dzdy0 * a;
		float z1 = v0[2] + dzdy1 * a;

		float wStart = v0[3] + dwdy0 * a;
		float wEnd = v0[3] + dwdy1 * a;

		V3 tempc0;
		v3_mul_f_out(dcdy0, a, tempc0);
		v3_add_v3(tempc0, start_c);

		V3 tempc1;
		v3_mul_f_out(dcdy1, a, tempc1);
		v3_add_v3(tempc1, start_c);

		int xStart = (int)(ceilf(x0 - 0.5f));
		int xEnd = (int)(ceilf(x1 - 0.5f));

		draw_scanline(rt, xStart, xEnd, y, z0, z1, wStart, wEnd, tempc0, tempc1);
	}
}

void draw_flat_top_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2)
{
	// Sort the flat vertices left to right.
	float* pv0 = v0;
	float* pv1 = v1;
	float* pc0 = c0;
	float* pc1 = c1;

	if (v0[0] > v1[0]) 
	{	
		swap(&pv0, &pv1);
		swap(&pc0, &pc1);
	}

	float invDy = 1 / (v2[1] - pv0[1]);

	float dxdy0 = (v2[0] - pv0[0]) * invDy;
	float dxdy1 = (v2[0] - pv1[0]) * invDy;

	float dzdy0 = (v2[2] - pv0[2]) * invDy;
	float dzdy1 = (v2[2] - pv1[2]) * invDy;

	float dwdy0 = (v2[3] - pv0[3]) * invDy;
	float dwdy1 = (v2[3] - pv1[3]) * invDy;

	V3 dcdy0;
	v3_sub_v3_out(c2, pc0, dcdy0);
	v3_mul_f(dcdy0, invDy);

	V3 dcdy1;
	v3_sub_v3_out(c2, pc1, dcdy1);
	v3_mul_f(dcdy1, invDy);

	V3 start_c = { 
		pc0[0],
		pc0[1],
		pc0[2]
	};

	V3 end_c = {
		pc1[0],
		pc1[1],
		pc1[2]
	};

	int yStart = (int)(ceil(pv0[1] - 0.5f));
	int yEnd = (int)(ceil(v2[1] - 0.5f));

	for (int y = yStart; y < yEnd; ++y) {
		// Must lerp for the vertex attributes to get them accurately.
		// TODO: Would be nice to find a way to step not lerp.
		float a = (y + 0.5f - pv0[1]);

		float x0 = pv0[0] + dxdy0 * a;
		float x1 = pv1[0] + dxdy1 * a;

		float z0 = pv0[2] + dzdy0 * a;
		float z1 = pv1[2] + dzdy1 * a;

		float wStart = pv0[3] + dwdy0 * a;
		float wEnd = pv1[3] + dwdy1 * a;

		V3 tempc0;
		v3_mul_f_out(dcdy0, a, tempc0);
		v3_add_v3(tempc0, start_c);

		V3 tempc1;
		v3_mul_f_out(dcdy1, a, tempc1);
		v3_add_v3(tempc1, end_c);

		int xStart = (int)(ceil(x0 - 0.5f));
		int xEnd = (int)(ceil(x1 - 0.5f));
		draw_scanline(rt, xStart, xEnd, y, z0, z1, wStart, wEnd, tempc0, tempc1);
	}
}

void draw_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2)
{
	// Sort vertices in ascending order.
	float* pv0 = v0;
	float* pv1 = v1;
	float* pv2 = v2;
	float* pc0 = c0;
	float* pc1 = c1;
	float* pc2 = c2;

	if (pv0[1] > pv1[1]) 
	{ 
		swap(&pv0, &pv1);
		swap(&pc0, &pc1);
	}
	if (pv0[1] > pv2[1])
	{
		swap(&pv0, &pv2);
		swap(&pc0, &pc2);
	}
	if (pv1[1] > pv2[1]) 
	{ 
		swap(&pv1, &pv2);
		swap(&pc1, &pc2);
	}
	
	// Handle if the triangle is already flat.
	if (pv0[1] == pv1[1])
	{
		draw_flat_top_triangle(rt, pv0, pv1, pv2, pc0, pc1, pc2);
		return;
	}

	if (pv1[1] == pv2[1])
	{
		draw_flat_bottom_triangle(rt, pv0, pv1, pv2, pc0, pc1, pc2);
		return;
	}

	// The triangle isn't flat, so split it into two flat triangles.

	// Linear interpolate for v3.
	float t = (pv1[1] - pv0[1]) / (pv2[1] - pv0[1]); 

	V4 pv3 = {
		pv0[0] + (pv2[0] - pv0[0]) * t,
		pv1[1],
		pv0[2] + (pv2[2] - pv0[2]) * t,
		pv0[3] + (pv2[3] - pv0[3]) * t
	};

	// Lerp for the new colour of the vertex.
	V3 pc3;
	v3_sub_v3_out(pc2, pc0, pc3);
	v3_mul_f(pc3, t);
	v3_add_v3(pc3, pc0);

	// TODO: UVs
	// V2 tex4 = tex1 + (tex3 - tex1) * t;
	draw_flat_top_triangle(rt, pv1, pv3, pv2, pc1, pc3, pc2);
	draw_flat_bottom_triangle(rt, pv0, pv1, pv3, pc0, pc1, pc3);
}

void draw_textured_scanline(RenderTarget* rt, int x0, int x1, int y, float z0, float z1, float w0, float w1, const V3 c0, const V3 c1, const V2 uv0, const V2 uv1, const Canvas* texture)
{
	// TODO: Refactor function args.

	// Precalculate deltas.
	const unsigned int dx = x1 - x0;
	float inv_dx = 1.f / dx;

	float w_step = (w1 - w0) * inv_dx;

	// Offset x by the given y.
	int row_offset = rt->canvas.width * y;

	int start_x = x0 + row_offset;
	int end_x = x1 + row_offset;

	V3 c_step;
	v3_sub_v3_out(c1, c0, c_step);
	v3_mul_f(c_step, inv_dx);

	V3 c = {
		c0[0],
		c0[1],
		c0[2]
	};

	V2 uv = {
		uv0[0],
		uv0[1]
	};

	V2 uv_step = 
	{
		(uv1[0] - uv0[0]) * inv_dx,
		(uv1[1] - uv0[1]) * inv_dx
	};

	// Render the scanline
	const unsigned int* texels = texture->pixels;
	unsigned int* pixels = rt->canvas.pixels + start_x;
	float* depth_buffer = rt->depth_buffer + start_x;

	float inv_w = w0;
	float z = z0;
	float z_step = (z1 - z0) * inv_dx;

	for (unsigned int i = 0; i < dx; ++i)
	{
		// Depth test, only draw closer values.
		if (*depth_buffer > z)
		{
			// Recover w
			const float w = 1.0f / inv_w;

			int cols = (int)((uv[0] * w) * texture->width);
			int rows = (int)((uv[1] * w) * texture->height);
			
			// TODO: Could store texels as float[3] to solve this.
			int tex = texels[rows * texture->width + cols];
			float r, g, b;
			unpack_int_rgb_to_floats(tex, &r, &g, &b);

			// Apply the lighting.
			r *= c[0] * w;
			g *= c[1] * w;
			b *= c[2] * w;
			
			// TODO: Try write out the components seperately? Can do this by
			//		 making the canvas an unsigned char array.
			// TODO: For interpolation, will have to change t based off of 
			//		 whatever r0 or r1 is larger as only have range 0-255.
			*pixels = float_rgb_to_int(r, g, b);
			*depth_buffer = z;
		}

		// Move to the next pixel
		++pixels;
		++depth_buffer;

		// Step per pixel values.
		z += z_step;
		inv_w += w_step;


		uv[0] += uv_step[0];
		uv[1] += uv_step[1];

		v3_add_v3(c, c_step);
	}
}

void draw_textured_flat_bottom_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2, V2 uv0, V2 uv1, V2 uv2, const Canvas* texture)
{
	// Sort the flat vertices left to right.
	float* pv1 = v1;
	float* pv2 = v2;
	float* pc1 = c1;
	float* pc2 = c2;
	float* puv1 = uv1;
	float* puv2 = uv2;

	// Sort the flat top left to right.
	if (v1[0] > v2[0])
	{
		swap(&pv1, &pv2);
		swap(&pc1, &pc2);
		swap(&puv1, &puv2);
	}

	float invDy = 1 / (pv2[1] - v0[1]);

	float dxdy0 = (pv1[0] - v0[0]) * invDy;
	float dxdy1 = (pv2[0] - v0[0]) * invDy;

	float dzdy0 = (pv1[2] - v0[2]) * invDy;
	float dzdy1 = (pv2[2] - v0[2]) * invDy;

	float dwdy0 = (pv1[3] - v0[3]) * invDy;
	float dwdy1 = (pv2[3] - v0[3]) * invDy;

	V3 dcdy0;
	v3_sub_v3_out(pc1, c0, dcdy0);
	v3_mul_f(dcdy0, invDy);

	V3 dcdy1;
	v3_sub_v3_out(pc2, c0, dcdy1);
	v3_mul_f(dcdy1, invDy);

	V2 duvdy0 =
	{
		(puv1[0] - uv0[0]) * invDy,
		(puv1[1] - uv0[1]) * invDy
	};

	V2 duvdy1 =
	{
		(puv2[0] - uv0[0]) * invDy,
		(puv2[1] - uv0[1]) * invDy
	};


	// TODO: Should be able to just lerp this stuff by 'incrementing' as we won't notice a difference,
	//		 right?
	V3 start_c = {
		c0[0],
		c0[1],
		c0[2]
	};

	V3 end_c = {
		c0[0],
		c0[1],
		c0[2]
	};

	int yStart = (int)(ceil(v0[1] - 0.5f));
	int yEnd = (int)(ceil(pv2[1] - 0.5f));

	for (int y = yStart; y < yEnd; ++y) {
		// Must lerp for the vertex attributes otherwise the accuracy is poor.
		// TODO: Would be nice to not have to actually lerp but step instead.
		float a = (y + 0.5f - v0[1]);

		// Calculate the start and ends of the scanline
		float x0 = v0[0] + dxdy0 * a;
		float x1 = v0[0] + dxdy1 * a;

		float z0 = v0[2] + dzdy0 * a;
		float z1 = v0[2] + dzdy1 * a;

		float wStart = v0[3] + dwdy0 * a;
		float wEnd = v0[3] + dwdy1 * a;

		V3 tempc0;
		v3_mul_f_out(dcdy0, a, tempc0);
		v3_add_v3(tempc0, start_c);

		V3 tempc1;
		v3_mul_f_out(dcdy1, a, tempc1);
		v3_add_v3(tempc1, start_c);

		int xStart = (int)(ceilf(x0 - 0.5f));
		int xEnd = (int)(ceilf(x1 - 0.5f));

		V2 temp_uv0 = {
			uv0[0] + duvdy0[0] * a,
			uv0[1] + duvdy0[1] * a
		};

		V2 temp_uv1 = {
			uv0[0] + duvdy1[0] * a,
			uv0[1] + duvdy1[1] * a
		};

		draw_textured_scanline(rt, xStart, xEnd, y, z0, z1, wStart, wEnd, tempc0, tempc1, temp_uv0, temp_uv1, texture);
	}
}

void draw_textured_flat_top_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2, V2 uv0, V2 uv1, V2 uv2, const Canvas* texture)
{
	// Sort the flat vertices left to right.
	float* pv0 = v0;
	float* pv1 = v1;
	float* pc0 = c0;
	float* pc1 = c1;
	float* puv0 = uv0;
	float* puv1 = uv1;

	if (v0[0] > v1[0])
	{
		swap(&pv0, &pv1);
		swap(&pc0, &pc1);
		swap(&puv0, &puv1);
	}

	float invDy = 1 / (v2[1] - pv0[1]);

	float dxdy0 = (v2[0] - pv0[0]) * invDy;
	float dxdy1 = (v2[0] - pv1[0]) * invDy;

	float dzdy0 = (v2[2] - pv0[2]) * invDy;
	float dzdy1 = (v2[2] - pv1[2]) * invDy;

	float dwdy0 = (v2[3] - pv0[3]) * invDy;
	float dwdy1 = (v2[3] - pv1[3]) * invDy;

	V3 dcdy0;
	v3_sub_v3_out(c2, pc0, dcdy0);
	v3_mul_f(dcdy0, invDy);

	V3 dcdy1;
	v3_sub_v3_out(c2, pc1, dcdy1);
	v3_mul_f(dcdy1, invDy);

	V2 duvdy0 =
	{
		(uv2[0] - puv0[0]) * invDy,
		(uv2[1] - puv0[1]) * invDy
	};

	V2 duvdy1 =
	{
		(uv2[0] - puv1[0]) * invDy,
		(uv2[1] - puv1[1]) * invDy
	};

	V3 start_c = {
		pc0[0],
		pc0[1],
		pc0[2]
	};

	V3 end_c = {
		pc1[0],
		pc1[1],
		pc1[2]
	};

	int yStart = (int)(ceil(pv0[1] - 0.5f));
	int yEnd = (int)(ceil(v2[1] - 0.5f));

	for (int y = yStart; y < yEnd; ++y) {
		// Must lerp for the vertex attributes to get them accurately.
		// TODO: Would be nice to find a way to step not lerp.
		float a = (y + 0.5f - pv0[1]);

		float x0 = pv0[0] + dxdy0 * a;
		float x1 = pv1[0] + dxdy1 * a;

		float z0 = pv0[2] + dzdy0 * a;
		float z1 = pv1[2] + dzdy1 * a;

		float wStart = pv0[3] + dwdy0 * a;
		float wEnd = pv1[3] + dwdy1 * a;

		// TODO: Should make a lerp v3/v4.
		V3 tempc0;
		v3_mul_f_out(dcdy0, a, tempc0);
		v3_add_v3(tempc0, start_c);

		V3 tempc1;
		v3_mul_f_out(dcdy1, a, tempc1);
		v3_add_v3(tempc1, end_c);

		V2 temp_uv0 = {
			puv0[0] + duvdy0[0] * a,
			puv0[1] + duvdy0[1] * a
		};

		V2 temp_uv1 = {
			puv1[0] + duvdy1[0] * a,
			puv1[1] + duvdy1[1] * a
		};

		int xStart = (int)(ceil(x0 - 0.5f));
		int xEnd = (int)(ceil(x1 - 0.5f));
		draw_textured_scanline(rt, xStart, xEnd, y, z0, z1, wStart, wEnd, tempc0, tempc1, temp_uv0, temp_uv1, texture);
	}
}

void draw_textured_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2, V2 uv0, V2 uv1, V2 uv2, const Canvas* texture)
{
	// Sort vertices in ascending order.
	float* pv0 = v0;
	float* pv1 = v1;
	float* pv2 = v2;
	float* pc0 = c0;
	float* pc1 = c1;
	float* pc2 = c2;
	float* puv0 = uv0;
	float* puv1 = uv1;
	float* puv2 = uv2;

	if (pv0[1] > pv1[1])
	{
		swap(&pv0, &pv1);
		swap(&pc0, &pc1);
		swap(&puv0, &puv1);
	}
	if (pv0[1] > pv2[1])
	{
		swap(&pv0, &pv2);
		swap(&pc0, &pc2);
		swap(&puv0, &puv2);
	}
	if (pv1[1] > pv2[1])
	{
		swap(&pv1, &pv2);
		swap(&pc1, &pc2);
		swap(&puv1, &puv2);
	}

	// Handle if the triangle is already flat.
	if (pv0[1] == pv1[1])
	{
		draw_textured_flat_top_triangle(rt, pv0, pv1, pv2, pc0, pc1, pc2, puv0, puv1, puv2, texture);
		return;
	}

	if (pv1[1] == pv2[1])
	{
		draw_textured_flat_bottom_triangle(rt, pv0, pv1, pv2, pc0, pc1, pc2, puv0, puv1, puv2, texture);
		return;
	}

	// The triangle isn't flat, so split it into two flat triangles.

	// Linear interpolate for v3.
	float t = (pv1[1] - pv0[1]) / (pv2[1] - pv0[1]);

	V4 v3 = {
		pv0[0] + (pv2[0] - pv0[0]) * t,
		pv1[1],
		pv0[2] + (pv2[2] - pv0[2]) * t,
		pv0[3] + (pv2[3] - pv0[3]) * t
	};

	// Lerp for the colour.
	V3 c3;
	v3_sub_v3_out(pc2, pc0, c3);
	v3_mul_f(c3, t);
	v3_add_v3(c3, pc0);

	// Lerp for the uv
	V2 uv3 =
	{
		puv0[0] + (puv2[0] - puv0[0]) * t,
		puv0[1] + (puv2[1] - puv0[1]) * t,
	};


	// TODO: UVs
	// V2 tex4 = tex1 + (tex3 - tex1) * t;
	draw_textured_flat_top_triangle(rt, pv1, v3, pv2, pc1, c3, pc2, puv1, uv3, puv2, texture);
	draw_textured_flat_bottom_triangle(rt, pv0, pv1, v3, pc0, pc1, c3, puv0, puv1, uv3, texture);
}

float calculate_diffuse_factor(const V3 v, const V3 n, const V3 light_pos, float a, float b)
{
	// TODO: Comments, check maths etc.

	// calculate the direction of the light to the vertex
	V3 light_dir; 
	v3_sub_v3_out(light_pos, v, light_dir);

	float light_distance = size(light_dir);

	v3_mul_f(light_dir, 1.f / light_distance);

	// Calculate how much the vertex is lit
	float diffuse_factor = max(0.0f, dot(light_dir, n));

	// TODO: Just hardcode the attenuation factors here? Not sure we will need to change them.

	float attenuation = 1.0f / (1.0f + (a * light_distance) + (b * light_distance * light_distance));
	float dp = diffuse_factor * attenuation;

	// TODO: What is the name for this after attentuation is applied to the 
	// diffsue factor?
	return dp;
}

void project(const Canvas* canvas, const M4 projection_matrix, const V4 v, V4 o)
{
	// Opengl uses a right handed coordinate system, camera looks down the -z axis,
	// however, NDC space is left handed, from -1 to 1 in all axis. 
	// Therefore, the perspective projection matrix copies and inverts the 
	// initial depth z, to w' in v_projected.
	
	// Apply the perspective projection matrix to project
	// the 3D coordinates into 2D.
	V4 v_projected;
	m4_mul_v4(projection_matrix, v, v_projected);

	// Perform perspective divide to bring to NDC space.
	// NDC space is a left handed coordinate system from -1 to 1 in all axis.
	const float inv_w = 1.0f / v_projected[3]; // Precalculate the perspective divide.

	v_projected[0] *= inv_w;
	v_projected[1] *= inv_w;
	v_projected[2] *= inv_w;

	// Convert from NDC space to screen space.
	// Convert from [-1:1] to [0:1], then scale to the screen dimensions.
	o[0] = (v_projected[0] + 1) * 0.5f * canvas->width;
	o[1] = (-v_projected[1] + 1) * 0.5f * canvas->height;

	// Projecting depth z results in z' which encodes a nonlinear transformation
	// of the depth, just like with x' and y'. So use this to depth test for 
	// more accurate results closer to the camera. Also, this means we only need
	// to recover the depth from w' if the depth test passes, saving a divison
	// per pixel.
	o[2] = (v_projected[2] + 1) * 0.5f; // Offset from [-1:1] to [0:1].

	// Save w' for perspective correct interpolation. This allows us to lerp
	// between vertex components. 
	o[3] = inv_w;
}

void model_to_view_space(Models* models, const M4 view_matrix)
{
	// NOTE: Timings Before these changes. 20fps rendering with no changing transforms
	//							   10 when spinning all. - THIS IS WHAT WE WANT TO BEAT


	// AIM: Combine the model/view matrices so we can move meshes at 'no' extra cost.

	// model_to_world_space with spinning takes about 12ms.
	// world_to_view_space took: 13ms

	// Combines the model and view matrices.
	// Calculates the new radius of the bounding sphere.

	const int mis_count = models->mis_count;

	const float* mis_transforms = models->mis_transforms;

	const int* mis_base_ids = models->mis_base_ids;
	int* mis_dirty_bounding_sphere_flags = models->mis_dirty_bounding_sphere_flags;

	const int* mbs_positions_counts = models->mbs_positions_counts;
	const int* mbs_positions_offsets = models->mbs_positions_offsets;
	const int* mbs_normals_counts = models->mbs_normals_counts;
	const int* mbs_normals_offsets = models->mbs_normals_offsets;

	const float* object_space_positions = models->mbs_object_space_positions;
	const float* object_space_normals = models->mbs_object_space_normals;
	const float* object_space_centres = models->mbs_object_space_centres;

	float* view_space_positions = models->view_space_positions;
	float* view_space_normals = models->view_space_normals;

	float* mis_bounding_spheres = models->mis_bounding_spheres;

	// TODO: For some of this I could probably put in {} to let some go out of scope?
	int vsp_out_index = 0;
	int vsn_out_index = 0;

	// TODO: Rename vars.

	for (int i = 0; i < mis_count; ++i)
	{
		// Convert the model base object space positions to world space
		// for the current model instance.
		const int mb_index = mis_base_ids[i];
		const int mb_positions_count = mbs_positions_counts[mb_index];
		const int normals_count = mbs_normals_counts[mb_index];

		// Calculate the new model/normal matrix from the mi's transform.
		int transform_index = i * STRIDE_MI_TRANSFORM;

		const V3 position = {
			mis_transforms[transform_index],
			mis_transforms[++transform_index],
			mis_transforms[++transform_index]
		};

		const V3 eulers = {
			mis_transforms[++transform_index],
			mis_transforms[++transform_index],
			mis_transforms[++transform_index]
		};

		const V3 scale = {
			mis_transforms[++transform_index],
			mis_transforms[++transform_index],
			mis_transforms[++transform_index]
		};

		M4 model_matrix;
		m4_model_matrix(position, eulers, scale, model_matrix);

		M4 model_view_matrix;
		m4_mul_m4(view_matrix, model_matrix, model_view_matrix);

		M4 normal_matrix;
		m4_normal_matrix(eulers, scale, normal_matrix);

		M4 view_normal_matrix;
		m4_mul_m4(view_matrix, normal_matrix, view_normal_matrix);



		// Store the initial out index so we can iterate over the 
		// wsp later when calculating the radius of the bounding sphere.
		const int start_vsp_out_index = vsp_out_index;

		const int mb_positions_offset = mbs_positions_offsets[mb_index];

		for (int j = 0; j < mb_positions_count; ++j)
		{
			int index_object_space_position = (j + mb_positions_offset) * STRIDE_POSITION;

			// TODO: A function like inline read_v4(float* out, float* in, int offset);
			// V4 object_space_position;
			// v4_read(object_space_position, object_space_positions, index_object_space_position)
			V4 object_space_position = {
				object_space_positions[index_object_space_position],
				object_space_positions[index_object_space_position + 1],
				object_space_positions[index_object_space_position + 2],
				1
			};

			V4 view_space_position;
			m4_mul_v4(model_view_matrix, object_space_position, view_space_position);

			// inline v4_write()?
			view_space_positions[vsp_out_index++] = view_space_position[0];
			view_space_positions[vsp_out_index++] = view_space_position[1];
			view_space_positions[vsp_out_index++] = view_space_position[2];
		}

		

		// Do the same for normals.		
		const int mb_normals_offset = mbs_normals_offsets[mb_index];

		for (int j = 0; j < normals_count; ++j)
		{
			int index_object_space_normals = (j + mb_normals_offset) * STRIDE_NORMAL;

			V4 object_space_normal = {
				object_space_normals[index_object_space_normals],
				object_space_normals[index_object_space_normals + 1],
				object_space_normals[index_object_space_normals + 2],
				0 // No translation
			};

			V4 view_space_normal;
			m4_mul_v4(view_normal_matrix, object_space_normal, view_space_normal);

			normalise(view_space_normal);

			view_space_normals[vsn_out_index++] = view_space_normal[0];
			view_space_normals[vsn_out_index++] = view_space_normal[1];
			view_space_normals[vsn_out_index++] = view_space_normal[2];
		}

		// Update the mi's bounding sphere.
		const int centre_index = mb_index * STRIDE_POSITION;
		V4 centre =
		{
			object_space_centres[centre_index],
			object_space_centres[centre_index + 1],
			object_space_centres[centre_index + 2],
			1
		};

		// Convert the model base centre to view space for the instance.
		V4 vs_centre;
		m4_mul_v4(model_view_matrix, centre, vs_centre);

		const int bs_index = i * STRIDE_SPHERE;
		mis_bounding_spheres[bs_index] = vs_centre[0];
		mis_bounding_spheres[bs_index + 1] = vs_centre[1];
		mis_bounding_spheres[bs_index + 2] = vs_centre[2];

		// Only update the bounding sphere if the scale is changed, otherwise
		// we don't need to update it.
		if (mis_dirty_bounding_sphere_flags[i])
		{
			mis_dirty_bounding_sphere_flags[i] = 0;

			// Calculate the new radius of the mi's bounding sphere.
			float radius_squared = -1;

			for (int j = start_vsp_out_index; j < vsp_out_index; j += STRIDE_POSITION)
			{
				V3 v =
				{
					view_space_positions[j],
					view_space_positions[j + 1],
					view_space_positions[j + 2],
				};

				V3 between;
				v3_sub_v3_out(v, vs_centre, between);

				radius_squared = max(size_squared(between), radius_squared);
			}

			// Save the radius.
			mis_bounding_spheres[bs_index + 3] = sqrtf(radius_squared);
		}
	}
}

void lights_world_to_view_space(PointLights* point_lights, const M4 view_matrix)
{
	// Transform the world space light positions.
	const float* world_space_positions = point_lights->world_space_positions;
	float* view_space_positions = point_lights->view_space_positions;

	// For each world space position, convert it to view space.
	const int num_position_components = point_lights->count * STRIDE_POSITION;
	for (int i = 0; i < num_position_components; i += STRIDE_POSITION)
	{
		V4 v = {
			world_space_positions[i],
			world_space_positions[i + 1],
			world_space_positions[i + 2],
			1
		};

		V4 v_view_space;
		m4_mul_v4(view_matrix, v, v_view_space);

		// There is no need to save the w component as it is always 1 until 
		// after projection.
		view_space_positions[i] = v_view_space[0];
		view_space_positions[i + 1] = v_view_space[1];
		view_space_positions[i + 2] = v_view_space[2];
	}
}

void broad_phase_frustum_culling(Models* models, const ViewFrustum* view_frustum)
{
	// Performs broad phase frustum culling on the models, writes out the planes
	// that can need to be clipped against.
	const int mis_count = models->mis_count;
	const float* bounding_spheres = models->mis_bounding_spheres;

	// Should be wrote out in the 
	int* intersected_planes = models->mis_intersected_planes;
	int intersected_planes_out_index = 0;
	int* passed_broad_phase_flags = models->mis_passed_broad_phase_flags;

	const int planes_count = view_frustum->planes_count;
	const Plane* planes = view_frustum->planes;

	// Perform frustum culling per model instance.
	for (int i = 0; i < mis_count; ++i)
	{
		// Perform board phase bounding sphere check against each plane.
		int index_bounding_sphere = i * STRIDE_SPHERE;

		// We must convert the center to view space
		V3 view_space_centre =
		{
			bounding_spheres[index_bounding_sphere++],
			bounding_spheres[index_bounding_sphere++],
			bounding_spheres[index_bounding_sphere++],
		};

		// Radius stays the same as the view matrix does not scale.
		const float radius = bounding_spheres[index_bounding_sphere];

		// Store what planes need clipping against.
		int clip_against_plane[MAX_FRUSTUM_PLANES] = { 0 };
		int num_planes_to_clip_against = 0;

		// Broad phase bounding sphere test.
		for (int j = 0; j < planes_count; ++j)
		{
			float dist = signed_distance(&planes[j], view_space_centre);
			if (dist < -radius)
			{
				// Completely outside the plane, therefore, no need to check against the others.
				num_planes_to_clip_against = -1; // -1 here means the mi is not visible.
				break;
			}
			else if (dist < radius)
			{
				// Mark that we need to clip against this plane.
				clip_against_plane[num_planes_to_clip_against] = j;
				++num_planes_to_clip_against;
			}
		}

		// Mark whether the mi passed the broad phase and store the intersection data
		// if it passed.
		if (-1 == num_planes_to_clip_against)
		{
			// Flag the mi as having failed the broad phase.
			passed_broad_phase_flags[i] = 0;
		}
		else
		{
			// Flag the mi as having passed the broad phase.
			passed_broad_phase_flags[i] = 1;

			// Write out for narrow phase to use.
			// In format: num_planes_intersecting, plane_index_0, plane_index_1, ...
			intersected_planes[intersected_planes_out_index++] = num_planes_to_clip_against;

			for (int j = 0; j < num_planes_to_clip_against; ++j)
			{
				intersected_planes[intersected_planes_out_index++] = clip_against_plane[j];
			}
		}
	}
}

void cull_backfaces(Models* models)
{
	// TODO: We're copying a lot of data here and unpacking indices. Some of this could
	//		 potentially be optimised if for example we write all positions, then all
	//		 normals, all colours etc.

	const int* passed_broad_phase_flags = models->mis_passed_broad_phase_flags;

	const int* mbs_positions_counts = models->mbs_positions_counts;
	const int* mbs_normals_counts = models->mbs_normals_counts;
	const int* mbs_faces_offsets = models->mbs_faces_offsets;
	const int* mbs_uvs_offsets = models->mbs_uvs_offsets;
	const int* mbs_faces_counts = models->mbs_faces_counts;

	const int* face_position_indices = models->mbs_face_position_indices;
	const int* face_normal_indices = models->mbs_face_normal_indices;
	const int* face_uvs_indices = models->mbs_face_uvs_indices;
	
	const float* view_space_positions = models->view_space_positions;
	const float* view_space_normals = models->view_space_normals;
	const float* uvs = models->mbs_uvs;
	
	float* front_faces = models->front_faces;
	int* front_faces_counts = models->front_faces_counts;

	const float* vertex_colours = models->mis_vertex_colours;

	int face_offset = 0;
	int front_face_out = 0;
	int positions_offset = 0;
	int normals_offset = 0;
	int uvs_offset = 0;

	for (int i = 0; i < models->mis_count; ++i)
	{
		const int mb_index = models->mis_base_ids[i];

		// Only need to do backface culling if the mi passed the broad phase.
		if (!passed_broad_phase_flags[i])
		{
			front_faces_counts[i] = 0;

			// Update the offsets for per instance data.
			positions_offset += mbs_positions_counts[mb_index];
			normals_offset += mbs_normals_counts[mb_index];

			continue;
		}

		// Get the offsets for the buffers that are not instance specific.
		const int mb_faces_offset = mbs_faces_offsets[mb_index];
		const int mb_uvs_offset = mbs_uvs_offsets[mb_index];

		int front_face_count = 0;

		for (int j = 0; j < mbs_faces_counts[mb_index]; ++j)
		{
			const int face_index = (mb_faces_offset + j) * STRIDE_FACE_VERTICES;

			// Get the indices to the first component of each vertex position.
			const int index_v0 = face_position_indices[face_index] + positions_offset;
			const int index_v1 = face_position_indices[face_index + 1] + positions_offset;
			const int index_v2 = face_position_indices[face_index + 2] + positions_offset;

			const int index_parts_v0 = index_v0 * STRIDE_POSITION;
			const int index_parts_v1 = index_v1 * STRIDE_POSITION;
			const int index_parts_v2 = index_v2 * STRIDE_POSITION;

			// Get the vertices from the face indices.
			const V3 v0 = {
				view_space_positions[index_parts_v0],
				view_space_positions[index_parts_v0 + 1],
				view_space_positions[index_parts_v0 + 2]
			};

			const V3 v1 = {
				view_space_positions[index_parts_v1],
				view_space_positions[index_parts_v1 + 1],
				view_space_positions[index_parts_v1 + 2]
			};

			const V3 v2 = {
				view_space_positions[index_parts_v2],
				view_space_positions[index_parts_v2 + 1],
				view_space_positions[index_parts_v2 + 2]
			};

			// If the face is front facing, we can possibly see it.
			if (is_front_face(v0, v1, v2))
			{		
				// Get the indices to the first component of each vertex normal.
				const int index_n0 = face_normal_indices[face_index] + normals_offset;
				const int index_n1 = face_normal_indices[face_index + 1] + normals_offset;
				const int index_n2 = face_normal_indices[face_index + 2] + normals_offset;

				int index_parts_n0 = index_n0 * STRIDE_NORMAL;
				int index_parts_n1 = index_n1 * STRIDE_NORMAL;
				int index_parts_n2 = index_n2 * STRIDE_NORMAL;
				
				const int index_uv0 = face_uvs_indices[face_index] + mb_uvs_offset;
				const int index_uv1 = face_uvs_indices[face_index + 1] + mb_uvs_offset;
				const int index_uv2 = face_uvs_indices[face_index + 2] + mb_uvs_offset;

				int index_parts_uv0 = index_uv0 * STRIDE_UV;
				int index_parts_uv1 = index_uv1 * STRIDE_UV;
				int index_parts_uv2 = index_uv2 * STRIDE_UV;

				// Vertex colours are defined aligned with the faces.
				const int index_parts_c0 = face_index * STRIDE_COLOUR;
				const int index_parts_c1 = (face_index + 1) * STRIDE_COLOUR;
				const int index_parts_c2 = (face_index + 2) * STRIDE_COLOUR;

				// Copy all the face vertex data.
				// We copy the attributes over here as well because when clipping we need the data
				// all together for lerping.
				// TODO: Make some defines for writing to the buffers i think.
				front_faces[front_face_out++] = v0[0];
				front_faces[front_face_out++] = v0[1];
				front_faces[front_face_out++] = v0[2];

				front_faces[front_face_out++] = uvs[index_parts_uv0];
				front_faces[front_face_out++] = uvs[index_parts_uv0 + 1];

				front_faces[front_face_out++] = view_space_normals[index_parts_n0];
				front_faces[front_face_out++] = view_space_normals[index_parts_n0 + 1];
				front_faces[front_face_out++] = view_space_normals[index_parts_n0 + 2];

				front_faces[front_face_out++] = vertex_colours[index_parts_c0];
				front_faces[front_face_out++] = vertex_colours[index_parts_c0 + 1];
				front_faces[front_face_out++] = vertex_colours[index_parts_c0 + 2];

				front_faces[front_face_out++] = v1[0];
				front_faces[front_face_out++] = v1[1];
				front_faces[front_face_out++] = v1[2];

				front_faces[front_face_out++] = uvs[index_parts_uv1];
				front_faces[front_face_out++] = uvs[index_parts_uv1 + 1];

				front_faces[front_face_out++] = view_space_normals[index_parts_n1];
				front_faces[front_face_out++] = view_space_normals[index_parts_n1 + 1];
				front_faces[front_face_out++] = view_space_normals[index_parts_n1 + 2];

				front_faces[front_face_out++] = vertex_colours[index_parts_c1];
				front_faces[front_face_out++] = vertex_colours[index_parts_c1 + 1];
				front_faces[front_face_out++] = vertex_colours[index_parts_c1 + 2];

				front_faces[front_face_out++] = v2[0];
				front_faces[front_face_out++] = v2[1];
				front_faces[front_face_out++] = v2[2];

				front_faces[front_face_out++] = uvs[index_parts_uv2];
				front_faces[front_face_out++] = uvs[index_parts_uv2 + 1];

				front_faces[front_face_out++] = view_space_normals[index_parts_n2];
				front_faces[front_face_out++] = view_space_normals[index_parts_n2 + 1];
				front_faces[front_face_out++] = view_space_normals[index_parts_n2 + 2];

				front_faces[front_face_out++] = vertex_colours[index_parts_c2];
				front_faces[front_face_out++] = vertex_colours[index_parts_c2 + 1];
				front_faces[front_face_out++] = vertex_colours[index_parts_c2 + 2];

				++front_face_count;
			}
		}

		// Update the number of front faces for the current mesh.
		// This is needed for frustum culling.
		front_faces_counts[i] = front_face_count;
		
		// Update the offsets for per instance data.
		positions_offset += mbs_positions_counts[mb_index];
		normals_offset += mbs_normals_counts[mb_index];
	}
}

void light_front_faces(Models* models, const PointLights* point_lights)
{
	/*
	TODO: Why doens't the per vertex seem to work?

	looks more like per face?

	for some reason it worked perfectly with mb loading issue when uvs werent defined
	and we were loading a different obj utah teapot (diff to the one we have now) 
	then having the light inside the mesh. No idea what happened but was smooth


	it will be something stupid but 5am now bed time :D
	
	
	*/


	// TODO: For optimising this, some sort of broad phase could be implemented.
	//		 Potentially when we 

	// Apply lighting to all the front faces.
	// We do this before clipping so if we don't get inconsistent results. 
	float* front_faces = models->front_faces;
	const int* front_faces_counts = models->front_faces_counts;

	int face_offset = 0;

	const int mis_count = models->mis_count;

	const int* passed_broad_phase_flags = models->mis_passed_broad_phase_flags;

	for (int i = 0; i < mis_count; ++i)
	{
		// Mesh isn't visible, so move to the next.
		if (!passed_broad_phase_flags[i])
		{
			// Move to the next mi.
			face_offset += front_faces_counts[i];
			continue;
		}

		const int front_faces_count = front_faces_counts[i];

		for (int j = face_offset; j < face_offset + front_faces_count; ++j)
		{
			int index_face = j * STRIDE_ENTIRE_FACE;

			// For each vertex apply lighting directly to the colour.
			// 12 attributes per vertex. TODO: STRIDE for this?
			for (int k = index_face; k < index_face + STRIDE_ENTIRE_FACE; k += STRIDE_ENTIRE_VERTEX)
			{
				const V3 pos = {
					front_faces[k],
					front_faces[k + 1],
					front_faces[k + 2],
				};

				const V3 normal = {
					front_faces[k + 5],
					front_faces[k + 6],
					front_faces[k + 7],
				};

				// Only need RGB, only need A for combining with texture???

				// This is how much light it absorbs?
				V3 colour = {
					front_faces[k + 8],
					front_faces[k + 9],
					front_faces[k + 10]
				};

				// TODO: I'm pretty sure the per pixel interpolation is broken or maybe that's just the lighting.

				V3 diffuse_part = { 0, 0, 0 };

				// For each light
				for (int i_light = 0; i_light < point_lights->count; ++i_light)
				{
					int i_light_pos = i_light * STRIDE_POSITION;
					int i_light_attr = i_light * STRIDE_POINT_LIGHT_ATTRIBUTES;

					const V3 light_pos =
					{
						point_lights->view_space_positions[i_light_pos],
						point_lights->view_space_positions[i_light_pos + 1],
						point_lights->view_space_positions[i_light_pos + 2]
					};

					V3 light_colour =
					{
						point_lights->attributes[i_light_attr],
						point_lights->attributes[i_light_attr + 1],
						point_lights->attributes[i_light_attr + 2]
					};

					float strength = point_lights->attributes[3];

					// TODO: Could cache this? Then the user can also set
					//		 the attenuation?
					float a = 0.1f / strength;
					float b = 0.01f / strength;

					float df = calculate_diffuse_factor(pos, normal, light_pos, a, b);

					v3_mul_f(light_colour, df);
					v3_add_v3(diffuse_part, light_colour);
				}

				diffuse_part[0] = min(diffuse_part[0], 1);
				diffuse_part[1] = min(diffuse_part[1], 1);
				diffuse_part[2] = min(diffuse_part[2], 1);

				v3_mul_v3(colour, diffuse_part);

				// TODO: Add Ambient too.

				front_faces[k + 8] = colour[0];
				front_faces[k + 9] = colour[1];
				front_faces[k + 10] = colour[2];
			}
		}
	
		face_offset += front_faces_count;
	}
}

void clip_to_screen(
	RenderTarget* rt, 
	const M4 projection_matrix, 
	const ViewFrustum* view_frustum, 
	const M4 view_matrix, 
	Models* models,
	const Resources* resources)
{
	// TODO: Rename this.

	
	// TODO: Look into this: https://zeux.io/2009/01/31/view-frustum-culling-optimization-introduction/


	// TODO: With 1000 monkeys takes about 160ms.
	
	// Frustum culling
	const int* intersected_planes = models->mis_intersected_planes;
	const int* passed_broad_phase_flags = models->mis_passed_broad_phase_flags;

	float* clipped_faces = models->clipped_faces;

	int face_offset = 0;
	int positions_offset = 0;

	float* front_faces = models->front_faces; // TEMP: Not const whilst drawing normals.
	const int* front_faces_counts = models->front_faces_counts;

	int intersected_planes_index = 0;

	// Perform frustum culling per model instance.
	for (int i = 0; i < models->mis_count; ++i)
	{
		// Mesh isn't visible, so move to the next.
		if (!passed_broad_phase_flags[i])
		{
			// Move to the next mi.
			face_offset += front_faces_counts[i];
			continue;
		}
		
		int num_planes_to_clip_against = intersected_planes[intersected_planes_index++];

		// Skip the mesh if it's not visible at all.
		if (0 == num_planes_to_clip_against)
		{
			// Entire mesh is visible so just copy the vertices over.
			int index_face = face_offset * STRIDE_ENTIRE_FACE;
			int front_faces_count = models->front_faces_counts[i];

			memcpy(clipped_faces, front_faces + index_face, (size_t)front_faces_count * STRIDE_ENTIRE_FACE * sizeof(float));

			// Draw the clipped face
			project_and_draw_clipped(rt, projection_matrix, models, i, front_faces_count, resources);

		}
		else
		{

			// TODO: The last mesh added is getting clipped when nowhere near it.
			// TODO: I think it's actually clipping is overwriting something.

			// Partially inside so must clip the vertices against the planes.

			// Initially read from the front_faces buffer.
			float* temp_clipped_faces_in = front_faces;
			float* temp_clipped_faces_out = models->temp_clipped_faces_out;

			// Store the index to write out to, needs to be defined here so we can
			// update the clipped_faces_index after writing to the clipped_faces buffer.
			int index_out = 0;

			// After each plane, we will have a different number of faces to clip again.
			// Initially set this to the number of front faces.
			int num_faces_to_process = models->front_faces_counts[i];

			// This is needed as an offset into the front_faces buffer for the first plane.
			int clipped_faces_offset = face_offset;

			// The logic for setting in/out buffers depends on how many planes we actually
			// render against, not the plane index.
			int num_planes_clipped_against = 0;

			for (int j = 0; j < num_planes_to_clip_against; ++j)
			{
				const Plane* plane = &view_frustum->planes[intersected_planes[intersected_planes_index++]];
				
				// Reset the index to write out to.
				index_out = 0;

				// Store how many triangles were wrote to the out buffer.				
				int temp_visible_faces_count = 0;

				// After the first plane we want to read from the in buffer.
				if (num_planes_clipped_against == 1)
				{
					// Initially we used the in front faces buffer, after the first iteration 
					// we have wrote to the out buffer, so that can now be our in buffer.
					temp_clipped_faces_out = models->temp_clipped_faces_in;

					// Now we want to read from the start of the in buffer, 
					// not the offset into the front faces buffer.
					clipped_faces_offset = 0;
				}

				// If we're processing the last plane, write out to the clipped_faces buffer.
				if (num_planes_clipped_against == num_planes_to_clip_against - 1)
				{
					// On the last plane, we want to write out to the clipped faces.
					temp_clipped_faces_out = models->clipped_faces;
				}

				// For faces in mesh.
				for (int j = clipped_faces_offset; j < clipped_faces_offset + num_faces_to_process; ++j)
				{
					int index_face = j * STRIDE_ENTIRE_FACE;

					int num_inside_points = 0;
					int num_outside_points = 0;

					int inside_points_indices[3] = { 0 };
					int outside_points_indices[3] = { 0 };

					const V3 v0 = {
						temp_clipped_faces_in[index_face],
						temp_clipped_faces_in[index_face + 1],
						temp_clipped_faces_in[index_face + 2]
					};

					const V3 v1 = {
						temp_clipped_faces_in[index_face + STRIDE_ENTIRE_VERTEX],
						temp_clipped_faces_in[index_face + STRIDE_ENTIRE_VERTEX + 1],
						temp_clipped_faces_in[index_face + STRIDE_ENTIRE_VERTEX + 2]
					};

					const V3 v2 = {
						temp_clipped_faces_in[index_face + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX],
						temp_clipped_faces_in[index_face + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 1],
						temp_clipped_faces_in[index_face + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 2]
					};

					float d0 = signed_distance(plane, v0);
					float d1 = signed_distance(plane, v1);
					float d2 = signed_distance(plane, v2);

					// Determine what points are inside and outside the plane.
					if (d0 >= 0)
					{
						inside_points_indices[num_inside_points++] = index_face; // index_v0 
					}
					else
					{
						outside_points_indices[num_outside_points++] = index_face; // index_v0
					}
					if (d1 >= 0)
					{
						inside_points_indices[num_inside_points++] = index_face + STRIDE_ENTIRE_VERTEX; // index_v1
					}
					else
					{
						outside_points_indices[num_outside_points++] = index_face + STRIDE_ENTIRE_VERTEX; // index_v1
					}
					if (d2 >= 0)
					{
						inside_points_indices[num_inside_points++] = index_face + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX; // index_v2
					}
					else
					{
						outside_points_indices[num_outside_points++] = index_face + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX; // index_v2
					}

					if (num_inside_points == 3)
					{
						// The whole triangle is inside the plane, so copy the face.
						int index_face = j * STRIDE_ENTIRE_FACE;

						memcpy(temp_clipped_faces_out + index_out, temp_clipped_faces_in + index_face, STRIDE_ENTIRE_FACE * sizeof(float));

						index_out += STRIDE_ENTIRE_FACE;
						++temp_visible_faces_count;
					}
					else if (num_inside_points == 1 && num_outside_points == 2)
					{
						// Form a new triangle with the plane edge.
						// Unpack the points.
						int index_ip0 = inside_points_indices[0];
						int index_op0 = outside_points_indices[0];
						int index_op1 = outside_points_indices[1];

						const V3 ip0 = {
							temp_clipped_faces_in[index_ip0],
							temp_clipped_faces_in[index_ip0 + 1],
							temp_clipped_faces_in[index_ip0 + 2]
						};

						const V3 op0 = {
							temp_clipped_faces_in[index_op0],
							temp_clipped_faces_in[index_op0 + 1],
							temp_clipped_faces_in[index_op0 + 2]
						};

						const V3 op1 = {
							temp_clipped_faces_in[index_op1],
							temp_clipped_faces_in[index_op1 + 1],
							temp_clipped_faces_in[index_op1 + 2]
						};

						// Lerp for the new points.
						V3 p0;
						float t = line_intersect_plane(ip0, op0, plane, p0);

						// Lerp for the attributes.
						const float u0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_U], temp_clipped_faces_in[index_op0 + INDEX_U], t);
						const float v0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_V], temp_clipped_faces_in[index_op0 + INDEX_V], t);
						const float nx0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NX], temp_clipped_faces_in[index_op0 + INDEX_NX], t);
						const float ny0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NY], temp_clipped_faces_in[index_op0 + INDEX_NY], t);
						const float nz0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NZ], temp_clipped_faces_in[index_op0 + INDEX_NZ], t);
						const float r0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_R], temp_clipped_faces_in[index_op0 + INDEX_R], t);
						const float g0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_G], temp_clipped_faces_in[index_op0 + INDEX_G], t);
						const float b0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_B], temp_clipped_faces_in[index_op0 + INDEX_B], t);

						V3 p1;
						t = line_intersect_plane(ip0, op1, plane, p1);
						
						// Lerp for the attributes.
						const float u1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_U], temp_clipped_faces_in[index_op1 + INDEX_U], t);
						const float v1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_V], temp_clipped_faces_in[index_op1 + INDEX_V], t);
						const float nx1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NX], temp_clipped_faces_in[index_op1 + INDEX_NX], t);
						const float ny1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NY], temp_clipped_faces_in[index_op1 + INDEX_NY], t);
						const float nz1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NZ], temp_clipped_faces_in[index_op1 + INDEX_NZ], t);
						const float r1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_R], temp_clipped_faces_in[index_op1 + INDEX_R], t);
						const float g1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_G], temp_clipped_faces_in[index_op1 + INDEX_G], t);
						const float b1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_B], temp_clipped_faces_in[index_op1 + INDEX_B], t);

						// Copy the attributes into the new face.
						// TODO: Must make helpers for writing these out, or just memcpy? Memcpy seems just as fast.
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];

						temp_clipped_faces_out[index_out++] = p0[0];
						temp_clipped_faces_out[index_out++] = p0[1];
						temp_clipped_faces_out[index_out++] = p0[2];
						temp_clipped_faces_out[index_out++] = u0;
						temp_clipped_faces_out[index_out++] = v0;
						temp_clipped_faces_out[index_out++] = nx0;
						temp_clipped_faces_out[index_out++] = ny0;
						temp_clipped_faces_out[index_out++] = nz0;
						temp_clipped_faces_out[index_out++] = r0;
						temp_clipped_faces_out[index_out++] = g0;
						temp_clipped_faces_out[index_out++] = b0;

						temp_clipped_faces_out[index_out++] = p1[0];
						temp_clipped_faces_out[index_out++] = p1[1];
						temp_clipped_faces_out[index_out++] = p1[2];
						temp_clipped_faces_out[index_out++] = u1;
						temp_clipped_faces_out[index_out++] = v1;
						temp_clipped_faces_out[index_out++] = nx1;
						temp_clipped_faces_out[index_out++] = ny1;
						temp_clipped_faces_out[index_out++] = nz1;
						temp_clipped_faces_out[index_out++] = r1;
						temp_clipped_faces_out[index_out++] = g1;
						temp_clipped_faces_out[index_out++] = b1;

						++temp_visible_faces_count;
					}
					else if (num_inside_points == 2 && num_outside_points == 1)
					{
						// Form two new triangles with the plane edge.
						// Unpack the points.
						int index_ip0 = inside_points_indices[0];
						int index_ip1 = inside_points_indices[1];
						int index_op0 = outside_points_indices[0];

						const V3 ip0 = {
							temp_clipped_faces_in[index_ip0],
							temp_clipped_faces_in[index_ip0 + 1],
							temp_clipped_faces_in[index_ip0 + 2]
						};

						const V3 ip1 = {
							temp_clipped_faces_in[index_ip1],
							temp_clipped_faces_in[index_ip1 + 1],
							temp_clipped_faces_in[index_ip1 + 2]
						};

						const V3 op0 = {
							temp_clipped_faces_in[index_op0],
							temp_clipped_faces_in[index_op0 + 1],
							temp_clipped_faces_in[index_op0 + 2]
						};

						// Lerp for the new points.
						V3 p0;
						float t = line_intersect_plane(ip0, op0, plane, p0);
						
						// Lerp for the attributes.
						const float u0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_U], temp_clipped_faces_in[index_op0 + INDEX_U], t);
						const float v0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_V], temp_clipped_faces_in[index_op0 + INDEX_V], t);
						const float nx0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NX], temp_clipped_faces_in[index_op0 + INDEX_NX], t);
						const float ny0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NY], temp_clipped_faces_in[index_op0 + INDEX_NY], t);
						const float nz0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NZ], temp_clipped_faces_in[index_op0 + INDEX_NZ], t);
						const float r0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_R], temp_clipped_faces_in[index_op0 + INDEX_R], t);
						const float g0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_G], temp_clipped_faces_in[index_op0 + INDEX_G], t);
						const float b0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_B], temp_clipped_faces_in[index_op0 + INDEX_B], t);

						// Copy the attributes into the new face.
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip0];

						int index_ip1_copy = index_ip1; // We need this for the next face also.

						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[index_ip1];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1];

						temp_clipped_faces_out[index_out++] = p0[0];
						temp_clipped_faces_out[index_out++] = p0[1];
						temp_clipped_faces_out[index_out++] = p0[2];
						temp_clipped_faces_out[index_out++] = u0;
						temp_clipped_faces_out[index_out++] = v0;
						temp_clipped_faces_out[index_out++] = nx0;
						temp_clipped_faces_out[index_out++] = ny0;
						temp_clipped_faces_out[index_out++] = nz0;
						temp_clipped_faces_out[index_out++] = r0;
						temp_clipped_faces_out[index_out++] = g0;
						temp_clipped_faces_out[index_out++] = b0;

						++temp_visible_faces_count;

						V3 p1;
						t = line_intersect_plane(ip1, op0, plane, p1);
						
						// Lerp for the attributes.
						const float u1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_U], temp_clipped_faces_in[index_op0 + INDEX_U], t);
						const float v1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_V], temp_clipped_faces_in[index_op0 + INDEX_V], t);
						const float nx1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_NX], temp_clipped_faces_in[index_op0 + INDEX_NX], t);
						const float ny1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_NY], temp_clipped_faces_in[index_op0 + INDEX_NY], t);
						const float nz1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_NZ], temp_clipped_faces_in[index_op0 + INDEX_NZ], t);
						const float r1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_R], temp_clipped_faces_in[index_op0 + INDEX_R], t);
						const float g1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_G], temp_clipped_faces_in[index_op0 + INDEX_G], t);
						const float b1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_B], temp_clipped_faces_in[index_op0 + INDEX_B], t);

						// Copy the attributes into the new face.
						temp_clipped_faces_out[index_out++] = p0[0];
						temp_clipped_faces_out[index_out++] = p0[1];
						temp_clipped_faces_out[index_out++] = p0[2];
						temp_clipped_faces_out[index_out++] = u0;
						temp_clipped_faces_out[index_out++] = v0;
						temp_clipped_faces_out[index_out++] = nx0;
						temp_clipped_faces_out[index_out++] = ny0;
						temp_clipped_faces_out[index_out++] = nz0;
						temp_clipped_faces_out[index_out++] = r0;
						temp_clipped_faces_out[index_out++] = g0;
						temp_clipped_faces_out[index_out++] = b0;

						temp_clipped_faces_out[index_out++] = p1[0];
						temp_clipped_faces_out[index_out++] = p1[1];
						temp_clipped_faces_out[index_out++] = p1[2];
						temp_clipped_faces_out[index_out++] = u1;
						temp_clipped_faces_out[index_out++] = v1;
						temp_clipped_faces_out[index_out++] = nx1;
						temp_clipped_faces_out[index_out++] = ny1;
						temp_clipped_faces_out[index_out++] = nz1;
						temp_clipped_faces_out[index_out++] = r1;
						temp_clipped_faces_out[index_out++] = g1;
						temp_clipped_faces_out[index_out++] = b1;

						// TODO: Memcpy?
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[index_ip1_copy];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1_copy];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1_copy];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1_copy];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1_copy];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1_copy];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1_copy];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1_copy];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1_copy];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1_copy];
						temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[++index_ip1_copy];

						++temp_visible_faces_count;
					}
				}

				// Update how many faces are visible after being clipped.
				num_faces_to_process = temp_visible_faces_count;

				// Swap the in and out buffers.
				float* temp = temp_clipped_faces_in;
				temp_clipped_faces_in = temp_clipped_faces_out;
				temp_clipped_faces_out = temp;

				// Increment to show we actually clipped a plane.
				++num_planes_clipped_against;
			}

			// Draw the clipped face
			if (num_faces_to_process > 0)
			{
				project_and_draw_clipped(rt, projection_matrix, models, i, num_faces_to_process, resources);
			}
		}

		// Move to the next model instance.
		face_offset += front_faces_counts[i];
	}
}

void project_and_draw_clipped(
	RenderTarget* rt, 
	const M4 projection_matrix, 
	const Models* models, 
	int mi_index, 
	int clipped_face_count,
	const Resources* resources)
{
	// TODO: We don't need normals here. Don't write them in clipping stage.


	// TODO: Comments. This function renders out the triangles in the clipped face buffer. 
	const float* clipped_faces = models->clipped_faces;

	// TODO: Refactor, how do I get rid of this duplicated code, or at least make it nicer
	//		 by finally adding read helpers for the buffers?

	const int texture_index = models->mis_texture_ids[mi_index];
	if (texture_index == -1)
	{
		for (int i = 0; i < clipped_face_count; ++i)
		{
			int clipped_face_index = i * STRIDE_ENTIRE_FACE;

			// Project the vertex positions.
			V4 v0 = {
				clipped_faces[clipped_face_index],
				clipped_faces[clipped_face_index + 1],
				clipped_faces[clipped_face_index + 2],
				1
			};

			V4 v1 = {
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 1],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 2],
				1
			};

			V4 v2 = {
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 1],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 2],
				1
			};

			V4 projected_v0, projected_v1, projected_v2;

			project(&rt->canvas, projection_matrix, v0, projected_v0);
			project(&rt->canvas, projection_matrix, v1, projected_v1);
			project(&rt->canvas, projection_matrix, v2, projected_v2);

			V3 colour0 = {
				clipped_faces[clipped_face_index + 8] * projected_v0[3],
				clipped_faces[clipped_face_index + 9] * projected_v0[3],
				clipped_faces[clipped_face_index + 10] * projected_v0[3]
			};

			V3 colour1 = {
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 8] * projected_v1[3],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 9] * projected_v1[3],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 10] * projected_v1[3]
			};

			V3 colour2 = {
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 8] * projected_v2[3],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 9] * projected_v2[3],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 10] * projected_v2[3]
			};

			draw_triangle(rt, projected_v0, projected_v1, projected_v2, colour0, colour1, colour2);
		}
	}
	else
	{
		Canvas* texture = &resources->textures[texture_index];
		for (int i = 0; i < clipped_face_count; ++i)
		{
			int clipped_face_index = i * STRIDE_ENTIRE_FACE;

			// Project the vertex positions.
			V4 v0 = {
				clipped_faces[clipped_face_index],
				clipped_faces[clipped_face_index + 1],
				clipped_faces[clipped_face_index + 2],
				1
			};

			V4 v1 = {
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 1],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 2],
				1
			};

			V4 v2 = {
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 1],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 2],
				1
			};

			V4 pv0, pv1, pv2;

			project(&rt->canvas, projection_matrix, v0, pv0);
			project(&rt->canvas, projection_matrix, v1, pv1);
			project(&rt->canvas, projection_matrix, v2, pv2);

			V2 uv0 =
			{
				clipped_faces[clipped_face_index + 3] * pv0[3],
				clipped_faces[clipped_face_index + 4] * pv0[3],
			};

			V3 colour0 = {
				clipped_faces[clipped_face_index + 8] * pv0[3],
				clipped_faces[clipped_face_index + 9] * pv0[3],
				clipped_faces[clipped_face_index + 10] * pv0[3]
			};

			V2 uv1 =
			{
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 3] * pv1[3],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 4] * pv1[3],
			};

			V3 colour1 = {
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 8] * pv1[3],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 9] * pv1[3],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 10] * pv1[3]
			};

			V2 uv2 =
			{
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 3] * pv2[3],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 4] * pv2[3],
			};

			V3 colour2 = {
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 8] * pv2[3],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 9] * pv2[3],
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 10] * pv2[3]
			};

			draw_textured_triangle(rt, pv0, pv1, pv2, colour0, colour1, colour2, uv0, uv1, uv2, texture);
		}
	}
}

void render(
	RenderTarget* rt, 
	const RenderSettings* settings, 
	Scene* scene,
	const Resources* resources,
	const M4 view_matrix)
{
/*
	Timer temp = timer_start();

	V4 v0 = { 0, 0, 0, 1 };
	V4 v1 = { rt->canvas.width, 0, 0, 1 };
	V4 v2 = { rt->canvas.width, rt->canvas.height, 0, 1 };
	V4 v3 = { 0, rt->canvas.height, 0, 1 };

	V3 c0 = { 1,0,0 };
	V3 c1 = { 0,1,0 };
	V3 c2 = { 0,0,1 };
	V3 c3 = { 1,1,1 };

	V2 uv0 = { 0,0 };
	V2 uv1 = { 1,0 };
	V2 uv2 = { 0,1 };
	V2 uv3 = { 1,1 };

	Canvas* texture = &resources->textures[0];

	draw_textured_triangle(rt, v0, v1, v2, c0, c1, c2, uv0, uv1, uv2, texture);
	draw_textured_triangle(rt, v0, v3, v2, c0, c3, c2, uv0, uv3, uv2, texture);

	//draw_triangle(rt, v0, v1, v2, c0, c1, c2);
	//draw_triangle(rt, v0, v3, v2, c0, c3, c2);
	printf("tooK: %d\n", timer_get_elapsed(&temp));
	// Before takes like: 4-6ms, getting like 160fps. default resolution.
	return;
*/
	// TODO: Make view matrix a part of the renderer, and the camera maybe. Then render should take the renderer i would assume.
	//		 or maybe these are a part of the settings. Bascially that part needs a refactor.
	Timer t = timer_start();
	
	// Transform object space positions to view space.
	model_to_view_space(&scene->models, view_matrix);
	//printf("model_to_view_space took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);

	lights_world_to_view_space(&scene->point_lights, view_matrix);
	//printf("lights_world_to_view_space took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);

	// Perform broad phase frustum culling to avoid unnecessary backface culling.
	broad_phase_frustum_culling(&scene->models, &settings->view_frustum);
	//printf("broad_phase_frustum_culling took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);

	// Perform backface culling.
	cull_backfaces(&scene->models);
	//printf("cull_backfaces took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);

	// At this point we know what mis arepartially visible at least.
	// Apply lighting here so that the if a vertex is clipped closer
	// to the light, the lighing doesn't change.
	light_front_faces(&scene->models, &scene->point_lights);
	//printf("light_front_faces took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);

	// Draws the front faces by performing the narrow phase of frustum culling
	// and then projecting and rasterising the faces.
	clip_to_screen(rt, settings->projection_matrix, &settings->view_frustum, view_matrix, &scene->models, resources);
	//printf("clip_to_screen took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);

	debug_draw_point_lights(&rt->canvas, settings, &scene->point_lights);
	//printf("debug_draw_point_lights took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);
	
	// Draw crosshair temporarily cause looks cool.
	int r = 2;
	int cx = (int)(rt->canvas.width / 2.f);
	int cy = (int)(rt->canvas.height / 2.f);
	draw_rect(&rt->canvas, cx - r, cy - r, cx + r, cy + r, COLOUR_WHITE);

	if (g_draw_normals)
	{
		debug_draw_mi_normals(&rt->canvas, settings, &scene->models, 0);
	}
	
}