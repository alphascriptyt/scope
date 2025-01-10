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
		if (p.z > -settings->near_plane)
		{
			continue;
		}

		V4 projected;
		project(canvas, settings->projection_matrix, p, &projected);

		int idx_attr = i * STRIDE_POINT_LIGHT_ATTRIBUTES;
		int colour = float_rgb_to_int(point_lights->attributes[idx_attr], point_lights->attributes[idx_attr + 1], point_lights->attributes[idx_attr + 2]);

		// Scale the radius so it's at a maximum of 10.
		const float radius = 10.f * (-settings->near_plane / p.z); // Square radius nice.

		int x0 = (int)(projected.x - radius);
		int x1 = (int)(projected.x + radius);

		int y0 = (int)(projected.y - radius);
		int y1 = (int)(projected.y + radius);

		draw_rect(canvas, x0, y0, x1, y1, colour);
	}
}

void debug_draw_bounding_spheres(Canvas* canvas, const RenderSettings* settings, const Models* models, const M4 view_matrix)
{
	// TODO: This doesn't really work.

	// TODO: For a function like this, I should be able to do debug_draw_bounding_sphere and pass in the mi index.
	/*
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

		V4 view_centre = v3_to_v4(view_centre_v3, 1.f);

		V4 view_centre = m4_mul_v4(view_matrix, world_centre_v4);

		if (view_centre.z > -settings->near_plane)
		{
			continue;
		}

		float radius = models->mis_bounding_spheres[3];

		V4 world_bottom = world_centre_v4;
		world_bottom.y -= radius;

		V4 view_bottom = m4_mul_v4(view_matrix, world_bottom);

		V4 world_top = world_centre_v4;
		world_top.y += radius;

		V4 view_top = m4_mul_v4(view_matrix, world_top);

		V4 pc = project(canvas, settings->projection_matrix, view_centre);
		V4 pt = project(canvas, settings->projection_matrix, view_top);
		V4 pb = project(canvas, settings->projection_matrix, view_bottom);
		
		float pr = fabsf(pb.y - pt.y) / 2.f;
		
		draw_circle(canvas, (int)pc.x, (int)pc.y, (int)pr, colour);
	}*/
}

void debug_draw_world_space_point(Canvas* canvas, const RenderSettings* settings, V3 point, const M4 view_matrix, int colour)
{
	// Convert from world space to screen space.
	V4 wsp = v3_to_v4(point, 1.f);

	V4 vsp;
	m4_mul_v4(view_matrix, wsp, &vsp);
	
	// Don't draw points behind the camera.
	if (vsp.z > -settings->near_plane) 
	{
		return;
	}

	V4 ssp;
	project(canvas, settings->projection_matrix, vsp, &ssp);

	// TODO: Could be a draw 2d rect function.
	int n = 2;
	int y0 = (int)(ssp.y - n);
	int y1 = (int)(ssp.y + n);
	int x0 = (int)(ssp.x - n);
	int x1 = (int)(ssp.x + n);

	draw_rect(canvas, x0, y0, x1, y1, colour);
}

void debug_draw_view_space_point(Canvas* canvas, const RenderSettings* settings, V3 point, int colour)
{
	// Convert from world space to screen space.
	V4 vsp = v3_to_v4(point, 1.f);

	// Don't draw points behind the camera.
	if (vsp.z > -settings->near_plane)
	{
		return;
	}

	V4 ssp; 
	project(canvas, settings->projection_matrix, vsp, &ssp);

	// TODO: Could be a draw 2d rect function.
	int n = 2;
	int y0 = (int)(ssp.y - n);
	int y1 = (int)(ssp.y + n);
	int x0 = (int)(ssp.x - n);
	int x1 = (int)(ssp.x + n);

	draw_rect(canvas, x0, y0, x1, y1, colour);
}

void debug_draw_world_space_line(Canvas* canvas, const RenderSettings* settings, const M4 view_matrix, V3 v0, V3 v1, V3 colour)
{
	V4 ws_v0 = v3_to_v4(v0, 1.f);
	V4 ws_v1 = v3_to_v4(v1, 1.f);

	V4 vs_v0, vs_v1;
	m4_mul_v4(view_matrix, ws_v0, &vs_v0);
	m4_mul_v4(view_matrix, ws_v1, &vs_v1);

	// Don't draw if behind the camera.
	if (vs_v0.z > -settings->near_plane && vs_v1.z > -settings->near_plane)
	{
		return;
	}

	// Clip the points to the near plane so we don't draw anything behind.
	// I think this works okay.
	if (vs_v0.z > -settings->near_plane)
	{
		float dist = vs_v0.z + settings->near_plane;
		
		float dxdz = (vs_v1.x - vs_v0.x) / (vs_v1.z - vs_v0.z);
		float dydz = (vs_v1.y - vs_v0.y) / (vs_v1.z - vs_v0.z);

		vs_v0.x += dxdz * dist;
		vs_v0.y += dydz * dist;
		vs_v0.z = -settings->near_plane;
	}
	else if (vs_v1.z > -settings->near_plane)
	{
		float dist = vs_v1.z + settings->near_plane;

		float dxdz = (vs_v0.x - vs_v1.x) / (vs_v0.z - vs_v1.z);
		float dydz = (vs_v0.y - vs_v1.y) / (vs_v0.z - vs_v1.z);

		vs_v1.x += dxdz * dist;
		vs_v1.y += dydz * dist;
		vs_v1.z = -settings->near_plane;
	}

	V4 ss_v0, ss_v1; 
	project(canvas, settings->projection_matrix, vs_v0, &ss_v0);
	project(canvas, settings->projection_matrix, vs_v1, &ss_v1);

	const int colour_int = float_rgb_to_int(colour.x, colour.y, colour.z);

	draw_line(canvas, (int)ss_v0.x, (int)ss_v0.y, (int)ss_v1.x, (int)ss_v1.y, colour_int);
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

			V3 dir = v3_mul_f(normal, length);
			V3 end = v3_add_v3(start, dir);

			V4 start_v4 = v3_to_v4(start, 1.f);
			V4 end_v4 = v3_to_v4(end, 1.f);

			V4 ss_start, ss_end; 
			project(canvas, settings->projection_matrix, start_v4, &ss_start);
			project(canvas, settings->projection_matrix, end_v4, &ss_end);

			draw_line(canvas, (int)ss_start.x, (int)ss_start.y, (int)ss_end.x, (int)ss_end.y, COLOUR_LIME);
		}
	}
}

void draw_scanline(RenderTarget* rt, int x0, int x1, int y, float z0, float z1, float w0, float w1, V3 c0, V3 c1, float* lsp0, float* lsp1, int lights_count, DepthBuffer* depth_maps)
{
	// TODO: Refactor function args.

	if (x0 == x1) return;

	// Precalculate deltas.
	const unsigned int dx = x1 - x0;
	float inv_dx = 1.f / dx;

	float w_step = (w1 - w0) * inv_dx;

	// Offset x by the given y.
	int row_offset = rt->canvas.width * y;

	int start_x = x0 + row_offset;
	int end_x = x1 + row_offset;

	V3 c_step = v3_mul_f(v3_sub_v3(c1, c0), inv_dx);
	V3 c = c0;

	// Render the scanline
	unsigned int* pixels = rt->canvas.pixels + start_x;
	float* depth_buffer = rt->depth_buffer + start_x;

	float inv_w = w0;
	float z = z0;
	float z_step = (z1 - z0) * inv_dx;

	// TODO: TEMP JUST ONE FOR NOW
	float lsp_start[4] = { lsp0[0], lsp0[1], lsp0[2], lsp0[3] };

	// TODO: why are the z/w almost always equal?
	//printf("%f %f %f %f\n", lsp0[0], lsp0[1], lsp0[2], lsp0[3]);
	//printf("%f %f %f %f\n\n", lsp1[0], lsp1[1], lsp1[2], lsp1[3]);
	float d_lsp[4] = { 
		(lsp1[0] - lsp0[0]) * inv_dx, 
		(lsp1[1] - lsp0[1]) * inv_dx, 
		(lsp1[2] - lsp0[2]) * inv_dx, 
		(lsp1[3] - lsp0[3]) * inv_dx };

	// TODO: Trying not accessing by pointer
	DepthBuffer db = depth_maps[0];

	const float hw = 0.5f * db.width;
	const float hh = 0.5f * db.height;

	for (unsigned int i = 0; i < dx; ++i)
	{
		// Depth test, only draw closer values.
		if (*depth_buffer > z)
		{
			// Recover w
			const float w = 1.0f / inv_w;

			const float r = (c.x * w);
			const float g = (c.y * w);
			const float b = (c.z * w);

			// TODO: FIX SHADOWS SPILLING OVER SIDES - is it actually a glitch or not.

			// Perform perspective correction to handle the perspective of the
			// camera? 
			V4 projected = {
				lsp_start[0] * w,
				lsp_start[1] * w,
				lsp_start[2] * w,
				lsp_start[3] * w
			};

			//https://www.youtube.com/watch?v=3FMONJ1O39U

			// Perspective divide to convert the light clip space to light NDC.
			float light_w = 1.f / projected.w;

			// Calculate the 
			V3 shadow_coords = {
				(projected.x * light_w + 1) * hw,
				(-projected.y * light_w + 1) * hh,
				(projected.z * light_w + 1) * 0.5f
			};
			

			int cols = (int)((shadow_coords.x));
			int rows = (int)((shadow_coords.y));
			//printf("%d %d\n", cols, rows);

			// the -1 test for lsp0 is temporary as we don't have clipping.
			// TODO: Although, what should be the default value if something is not on the map?
			//		 what if one vertex is and another isn't?
			if (cols < 0 || cols > db.width - 1 || rows < 0 || rows > db.height - 1)
			{
				// Light pixel because off the shadow map.
				*pixels = float_rgb_to_int(r, g, b);

				if (g_debug_shadows)
				{
					*pixels = COLOUR_RED;
				}
					
			}
			else
			{
				int xo = rt->canvas.width - db.width;

				int index = (int)(rows * rt->canvas.width + xo + cols);
				

				//int cols = (int)(rel_cols * db.width) - 1;
				//int rows = (int)(rel_rows * db.height) - 1;

 				float light_min_depth = db.data[rows * db.width + cols];

				// TODO: We're quite close...... i think....
				// TODO: But why does the effect change with the camera? That seems to be an issue here
				//float pixel_light_depth = lsp_start[2];
				float pixel_light_depth = shadow_coords.z;

				//printf("%f %f\n", pixel_light_depth, light_min_depth);
				if (pixel_light_depth > light_min_depth + 0.002f) // Constant bias to avoid peter panning.
				{
					if (g_debug_shadows)
					{
						rt->canvas.pixels[index] = COLOUR_LIME;
						*pixels = COLOUR_LIME;
					}
					else
					{
						*pixels = float_rgb_to_int(0.1f, 0.1f, 0.1f); // TODO: Ambient/ combine all lights.
						//printf("%f %f\n", pixel_light_depth, light_min_depth);
					}
				}
				else
				{
					if (g_debug_shadows)
					{
						rt->canvas.pixels[index] = COLOUR_BLUE;
					}
					
					*pixels = float_rgb_to_int(r, g, b);
				}
			}

			*depth_buffer = z;
		}

		// Move to the next pixel
		++pixels;
		++depth_buffer;

		// Step per pixel values.
		z += z_step;
		inv_w += w_step;

		lsp_start[0] += d_lsp[0];
		lsp_start[1] += d_lsp[1];
		lsp_start[2] += d_lsp[2];
		lsp_start[3] += d_lsp[3];

		v3_add_eq_v3(&c, c_step);
	}
}

void draw_flat_bottom_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2, float* lsp0, float* lsp1, float* lsp2, int lights_count, DepthBuffer* depth_maps)
{
	float* plsp1 = lsp1;
	float* plsp2 = lsp2;

	// Sort the flat vertices left to right.
	if (v1.x > v2.x) 
	{  
		v4_swap(&v1, &v2);
		v3_swap(&c1, &c2);

		float* temp = plsp1;
		plsp1 = plsp2;
		plsp2 = temp;
	}

	float inv_dy = 1 / (v2.y - v0.y);

	float dxdy0 = (v1.x - v0.x) * inv_dy;
	float dxdy1 = (v2.x - v0.x) * inv_dy;

	float dzdy0 = (v1.z - v0.z) * inv_dy;
	float dzdy1 = (v2.z - v0.z) * inv_dy;

	float dwdy0 = (v1.w - v0.w) * inv_dy;
	float dwdy1 = (v2.w - v0.w) * inv_dy;

	V3 dcdy0 = v3_mul_f(v3_sub_v3(c1, c0), inv_dy);
	V3 dcdy1 = v3_mul_f(v3_sub_v3(c2, c0), inv_dy);

	int yStart = (int)(ceil(v0.y - 0.5f));
	int yEnd = (int)(ceil(v2.y - 0.5f));

	float lsp0dy[4] = {
		(plsp1[0] - lsp0[0]) * inv_dy,
		(plsp1[1] - lsp0[1]) * inv_dy,
		(plsp1[2] - lsp0[2]) * inv_dy,
		(plsp1[3] - lsp0[3]) * inv_dy
	};

	float lsp1dy[4] = {
		(plsp2[0] - lsp0[0]) * inv_dy,
		(plsp2[1] - lsp0[1]) * inv_dy,
		(plsp2[2] - lsp0[2]) * inv_dy,
		(plsp2[3] - lsp0[3]) * inv_dy
	};

	for (int y = yStart; y < yEnd; ++y) {
		// Must lerp for the vertex attributes otherwise the accuracy is poor.
		// TODO: Would be nice to not have to actually lerp but step instead.
		float a = (y + 0.5f - v0.y);

		// Calculate the start and ends of the scanline
		float x0 = v0.x + dxdy0 * a;
		float x1 = v0.x + dxdy1 * a;

		float z0 = v0.z + dzdy0 * a;
		float z1 = v0.z + dzdy1 * a;

		float wStart = v0.w + dwdy0 * a;
		float wEnd = v0.w + dwdy1 * a;

		// TODO: Lerp v3 function.
		V3 tempc0 = v3_add_v3(c0, v3_mul_f(dcdy0, a));
		V3 tempc1 = v3_add_v3(c0, v3_mul_f(dcdy1, a));

		int xStart = (int)(ceilf(x0 - 0.5f));
		int xEnd = (int)(ceilf(x1 - 0.5f));

		float lsp_temp0[4] = { 
			lsp0[0] + lsp0dy[0] * a, 
			lsp0[1] + lsp0dy[1] * a, 
			lsp0[2] + lsp0dy[2] * a, 
			lsp0[3] + lsp0dy[3] * a };

		float lsp_temp1[4] = { 
			lsp0[0] + lsp1dy[0] * a, 
			lsp0[1] + lsp1dy[1] * a, 
			lsp0[2] + lsp1dy[2] * a, 
			lsp0[3] + lsp1dy[3] * a };

		draw_scanline(rt, xStart, xEnd, y, z0, z1, wStart, wEnd, tempc0, tempc1, lsp_temp0, lsp_temp1, lights_count, depth_maps);
	}
}

void draw_flat_top_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2, float* lsp0, float* lsp1, float* lsp2, int lights_count, DepthBuffer* depth_maps)
{
	float* plsp0 = lsp0;
	float* plsp1 = lsp1;

	// Sort the flat vertices left to right.
	if (v0.x > v1.x) 
	{	
		v4_swap(&v0, &v1);
		v3_swap(&c0, &c1);

		float* temp = lsp0;
		plsp0 = plsp1;
		plsp1 = temp;
	}

	float inv_dy = 1 / (v2.y - v0.y);

	float dxdy0 = (v2.x - v0.x) * inv_dy;
	float dxdy1 = (v2.x - v1.x) * inv_dy;

	float dzdy0 = (v2.z - v0.z) * inv_dy;
	float dzdy1 = (v2.z - v1.z) * inv_dy;

	float dwdy0 = (v2.w - v0.w) * inv_dy;
	float dwdy1 = (v2.w - v1.w) * inv_dy;

	V3 dcdy0 = v3_mul_f(v3_sub_v3(c2, c0), inv_dy);
	V3 dcdy1 = v3_mul_f(v3_sub_v3(c2, c1), inv_dy);

	V3 start_c = c0;
	V3 end_c = c1;

	int yStart = (int)(ceil(v0.y - 0.5f));
	int yEnd = (int)(ceil(v2.y - 0.5f));

	// TODO: TEMP: For now just processing one light rather than lerp loop.
	float lsp0dy[4] = {
		(lsp2[0] - plsp0[0]) * inv_dy,
		(lsp2[1] - plsp0[1]) * inv_dy,
		(lsp2[2] - plsp0[2]) * inv_dy,
		(lsp2[3] - plsp0[3]) * inv_dy
	};

	float lsp1dy[4] = {
		(lsp2[0] - plsp1[0]) * inv_dy,
		(lsp2[1] - plsp1[1]) * inv_dy,
		(lsp2[2] - plsp1[2]) * inv_dy,
		(lsp2[3] - plsp1[3]) * inv_dy
	};

	for (int y = yStart; y < yEnd; ++y) {
		// Must lerp for the vertex attributes to get them accurately.
		// TODO: Would be nice to find a way to step not lerp.
		float a = (y + 0.5f - v0.y);

		float x0 = v0.x + dxdy0 * a;
		float x1 = v1.x + dxdy1 * a;

		float z0 = v0.z + dzdy0 * a;
		float z1 = v1.z + dzdy1 * a;

		float wStart = v0.w + dwdy0 * a;
		float wEnd = v1.w + dwdy1 * a;

		V3 tempc0 = v3_add_v3(start_c, v3_mul_f(dcdy0, a));
		V3 tempc1 = v3_add_v3(end_c, v3_mul_f(dcdy1, a));

		int xStart = (int)(ceil(x0 - 0.5f));
		int xEnd = (int)(ceil(x1 - 0.5f));

		float lsp_temp0[4] = { 
			plsp0[0] + lsp0dy[0] * a, 
			plsp0[1] + lsp0dy[1] * a, 
			plsp0[2] + lsp0dy[2] * a, 
			plsp0[3] + lsp0dy[3] * a };

		float lsp_temp1[4] = { 
			plsp1[0] + lsp1dy[0] * a, 
			plsp1[1] + lsp1dy[1] * a, 
			plsp1[2] + lsp1dy[2] * a, 
			plsp1[3] + lsp1dy[3] * a };

		draw_scanline(rt, xStart, xEnd, y, z0, z1, wStart, wEnd, tempc0, tempc1, lsp_temp0, lsp_temp1, lights_count, depth_maps);
	}
}

void draw_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2, float* lsp0, float* lsp1, float* lsp2, int lights_count, DepthBuffer* depth_maps)
{
	// There is space at the end...
	// need x,y,z,w.
	float* lsp3 = lsp0 + lights_count * 3 * 4;

	float* plsp0 = lsp0;
	float* plsp1 = lsp1;
	float* plsp2 = lsp2;

	// Sort vertices in ascending order.
	if (v0.y > v1.y) 
	{ 
		v4_swap(&v0, &v1);
		v3_swap(&c0, &c1);

		float* temp = plsp0;
		plsp0 = plsp1;
		plsp1 = temp;
	}
	if (v0.y > v2.y)
	{
		v4_swap(&v0, &v2);
		v3_swap(&c0, &c2);

		float* temp = plsp0;
		plsp0 = plsp2;
		plsp2 = temp;
	}
	if (v1.y > v2.y) 
	{ 
		v4_swap(&v1, &v2);
		v3_swap(&c1, &c2);

		float* temp = plsp1;
		plsp1 = plsp2;
		plsp2 = temp;
	}
	
	// Handle if the triangle is already flat.
	if (v0.y == v1.y)
	{
		draw_flat_top_triangle(rt, v0, v1, v2, c0, c1, c2, plsp0, plsp1, plsp2, lights_count, depth_maps);
		return;
	}

	if (v1.y == v2.y)
	{
		draw_flat_bottom_triangle(rt, v0, v1, v2, c0, c1, c2, plsp0, plsp1, plsp2, lights_count, depth_maps);
		return;
	}
	
	// The triangle isn't flat, so split it into two flat triangles.

	// Linear interpolate for v3.
	float t = (v1.y - v0.y) / (v2.y - v0.y); 

	V4 v3 = {
		v0.x + (v2.x - v0.x) * t,
		v1.y,
		v0.z + (v2.z - v0.z) * t,
		v0.w + (v2.w - v0.w) * t
	};

	// Lerp for the new colour of the vertex.
	V3 c3 = v3_add_v3(c0, v3_mul_f(v3_sub_v3(c2, c0), t));

	////// TODO: At this point we need to calculate between each point????... how....

	// Where can we get space for a v4 array..... pass one in???

	
	// TODO: TEMP HARDCODED FOR 1
	
	lsp3[0] = plsp0[0] + (plsp2[0] - plsp0[0]) * t;
	lsp3[1] = plsp0[1] + (plsp2[1] - plsp0[1]) * t;
	lsp3[2] = plsp0[2] + (plsp2[2] - plsp0[2]) * t;
	lsp3[3] = plsp0[3] + (plsp2[3] - plsp0[3]) * t;

	draw_flat_top_triangle(rt, v1, v3, v2, c1, c3, c2, plsp1, lsp3, plsp2, lights_count, depth_maps);
	draw_flat_bottom_triangle(rt, v0, v1, v3, c0, c1, c3, plsp0, plsp1, lsp3, lights_count, depth_maps);
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

	V3 c_step = v3_mul_f(v3_sub_v3(c1, c0), inv_dx);
	V3 c = c0;

	V2 uv = uv0;

	V2 uv_step = 
	{
		(uv1.x - uv0.x) * inv_dx,
		(uv1.y - uv0.y) * inv_dx
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

			// TODO: Could avoiding the pointer derefernce outside of the per-pixel loop give a speed up?
			int cols = (int)((uv.x * w) * texture->width);
			int rows = (int)((uv.y * w) * texture->height);
			
			// TODO: Could store texels as float[3] to solve this.
			int tex = texels[rows * texture->width + cols];
			float r, g, b;
			unpack_int_rgb_to_floats(tex, &r, &g, &b);

			// Apply the lighting.
			r *= c.x * w;
			g *= c.y * w;
			b *= c.z * w;
			
			// TODO: Try write out the components seperately? Can do this by
			//		 making the canvas an unsigned char array.
			// This should save a fair bit on packing the components together.
			*pixels = float_rgb_to_int(r, g, b);
			*depth_buffer = z;
		}

		// Move to the next pixel
		++pixels;
		++depth_buffer;

		// Step per pixel values.
		z += z_step;
		inv_w += w_step;


		uv.x += uv_step.x;
		uv.y += uv_step.y;

		v3_add_eq_v3(&c, c_step);
	}
}

void draw_textured_flat_bottom_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2, V2 uv0, V2 uv1, V2 uv2, const Canvas* texture)
{
	// Sort the flat vertices left to right.
	if (v1.x > v2.x)
	{
		v4_swap(&v1, &v2);
		v3_swap(&c1, &c2);
		v2_swap(&uv1, &uv2);
	}

	float inv_dy = 1 / (v2.y - v0.y);

	float dxdy0 = (v1.x - v0.x) * inv_dy;
	float dxdy1 = (v2.x - v0.x) * inv_dy;

	float dzdy0 = (v1.z - v0.z) * inv_dy;
	float dzdy1 = (v2.z - v0.z) * inv_dy;

	float dwdy0 = (v1.w - v0.w) * inv_dy;
	float dwdy1 = (v2.w - v0.w) * inv_dy;

	V3 dcdy0 = v3_mul_f(v3_sub_v3(c1, c0), inv_dy);
	V3 dcdy1 = v3_mul_f(v3_sub_v3(c2, c0), inv_dy);

	V2 duvdy0 =
	{
		(uv1.x - uv0.x) * inv_dy,
		(uv1.y - uv0.y) * inv_dy
	};

	V2 duvdy1 =
	{
		(uv2.x - uv0.x) * inv_dy,
		(uv2.y - uv0.y) * inv_dy
	};


	// TODO: I should be able to lerp and just increment right? Apart from x,
	//		 we shouldn't notice any other glitches in the textures or colour.
	int yStart = (int)(ceil(v0.y - 0.5f));
	int yEnd = (int)(ceil(v2.y - 0.5f));

	for (int y = yStart; y < yEnd; ++y) {
		// Must lerp for the vertex attributes otherwise the accuracy is poor.
		// TODO: Would be nice to not have to actually lerp but step instead.
		float a = (y + 0.5f - v0.y);

		// Calculate the start and ends of the scanline
		float x0 = v0.x + dxdy0 * a;
		float x1 = v0.x + dxdy1 * a;

		float z0 = v0.z + dzdy0 * a;
		float z1 = v0.z + dzdy1 * a;

		float wStart = v0.w + dwdy0 * a;
		float wEnd = v0.w + dwdy1 * a;

		V3 temc0 = v3_add_v3(c0, v3_mul_f(dcdy0, a));
		V3 temc1 = v3_add_v3(c0, v3_mul_f(dcdy1, a));

		int xStart = (int)(ceilf(x0 - 0.5f));
		int xEnd = (int)(ceilf(x1 - 0.5f));

		V2 temp_uv0 = {
			uv0.x + duvdy0.x * a,
			uv0.y + duvdy0.y * a
		};

		V2 temp_uv1 = {
			uv0.x + duvdy1.x * a,
			uv0.y + duvdy1.y * a
		};

		draw_textured_scanline(rt, xStart, xEnd, y, z0, z1, wStart, wEnd, temc0, temc1, temp_uv0, temp_uv1, texture);
	}
}

void draw_textured_flat_top_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2, V2 uv0, V2 uv1, V2 uv2, const Canvas* texture)
{
	// Sort the flat vertices left to right.
	if (v0.x > v1.x)
	{
		v4_swap(&v0, &v1);
		v3_swap(&c0, &c1);
		v2_swap(&uv0, &uv1);
	}

	float inv_dy = 1 / (v2.y - v0.y);

	float dxdy0 = (v2.x - v0.x) * inv_dy;
	float dxdy1 = (v2.x - v1.x) * inv_dy;

	float dzdy0 = (v2.z - v0.z) * inv_dy;
	float dzdy1 = (v2.z - v1.z) * inv_dy;

	float dwdy0 = (v2.w - v0.w) * inv_dy;
	float dwdy1 = (v2.w - v1.w) * inv_dy;

	V3 dcdy0 = v3_mul_f(v3_sub_v3(c2, c0), inv_dy);
	V3 dcdy1 = v3_mul_f(v3_sub_v3(c2, c1), inv_dy);

	V2 duvdy0 =
	{
		(uv2.x - uv0.x) * inv_dy,
		(uv2.y - uv0.y) * inv_dy
	};

	V2 duvdy1 =
	{
		(uv2.x - uv1.x) * inv_dy,
		(uv2.y - uv1.y) * inv_dy
	};

	V3 start_c = c0;
	V3 end_c = c1;

	int yStart = (int)(ceil(v0.y - 0.5f));
	int yEnd = (int)(ceil(v2.y - 0.5f));

	for (int y = yStart; y < yEnd; ++y) {
		// Must lerp for the vertex attributes to get them accurately.
		// TODO: Would be nice to find a way to step not lerp.
		float a = ((float)y + 0.5f - v0.y);

		float x0 = v0.x + dxdy0 * a;
		float x1 = v1.x + dxdy1 * a;

		float z0 = v0.z + dzdy0 * a;
		float z1 = v1.z + dzdy1 * a;

		float wStart = v0.w + dwdy0 * a;
		float wEnd = v1.w + dwdy1 * a;

		// TODO: Should make a lerp v3/v4.
		V3 temc0 = v3_add_v3(start_c, v3_mul_f(dcdy0, a));
		V3 temc1 = v3_add_v3(end_c, v3_mul_f(dcdy1, a));

		V2 temp_uv0 = {
			uv0.x + duvdy0.x * a,
			uv0.y + duvdy0.y * a
		};

		V2 temp_uv1 = {
			uv1.x + duvdy1.x * a,
			uv1.y + duvdy1.y * a
		};

		int xStart = (int)(ceil(x0 - 0.5f));
		int xEnd = (int)(ceil(x1 - 0.5f));
		draw_textured_scanline(rt, xStart, xEnd, y, z0, z1, wStart, wEnd, temc0, temc1, temp_uv0, temp_uv1, texture);
	}
}

void draw_textured_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2, V2 uv0, V2 uv1, V2 uv2, const Canvas* texture)
{
	// Sort vertices in ascending order.
	if (v0.y > v1.y)
	{
		v4_swap(&v0, &v1);
		v3_swap(&c0, &c1);
		v2_swap(&uv0, &uv1);
	}
	if (v0.y > v2.y)
	{
		v4_swap(&v0, &v2);
		v3_swap(&c0, &c2);
		v2_swap(&uv0, &uv2);
	}
	if (v1.y > v2.y)
	{
		v4_swap(&v1, &v2);
		v3_swap(&c1, &c2);
		v2_swap(&uv1, &uv2);
	}

	// Handle if the triangle is already flat.
	if (v0.y == v1.y)
	{
		draw_textured_flat_top_triangle(rt, v0, v1, v2, c0, c1, c2, uv0, uv1, uv2, texture);
		return;
	}

	if (v1.y == v2.y)
	{
		draw_textured_flat_bottom_triangle(rt, v0, v1, v2, c0, c1, c2, uv0, uv1, uv2, texture);
		return;
	}

	// The triangle isn't flat, so split it into two flat triangles.

	// Linear interpolate for v3.
	float t = (v1.y - v0.y) / (v2.y - v0.y);

	V4 v3 = {
		v0.x + (v2.x - v0.x) * t,
		v1.y,
		v0.z + (v2.z - v0.z) * t,
		v0.w + (v2.w - v0.w) * t
	};

	// Lerp for the colour.
	V3 c3 = v3_add_v3(c0, v3_mul_f(v3_sub_v3(c2, c0), t));

	// Lerp for the uv
	V2 uv3 =
	{
		uv0.x + (uv2.x - uv0.x) * t,
		uv0.y + (uv2.y - uv0.y) * t,
	};


	// TODO: UVs
	// V2 tex4 = tex1 + (tex3 - tex1) * t;
	draw_textured_flat_top_triangle(rt, v1, v3, v2, c1, c3, c2, uv1, uv3, uv2, texture);
	draw_textured_flat_bottom_triangle(rt, v0, v1, v3, c0, c1, c3, uv0, uv1, uv3, texture);
}

void draw_depth_scanline(DepthBuffer* db, int x0, int x1, int y, float z0, float z1)
{
	if (x0 == x1) return;

	// Precalculate deltas.
	const unsigned int dx = x1 - x0;
	float inv_dx = 1.f / dx;

	// Offset x by the given y.
	int row_offset = db->width * y;

	int start_x = x0 + row_offset;
	int end_x = x1 + row_offset;

	// Render the scanline
	float* depth_buffer = db->data + start_x;

	float z = z0;
	float z_step = (z1 - z0) * inv_dx;

	for (unsigned int i = 0; i < dx; ++i)
	{
		// Depth test, only draw closer values.
		if (*depth_buffer > z)
		{
			// TODO: min function for better speed?
			*depth_buffer = z;
		}

		// Move to the next pixel
		++depth_buffer;

		// Step per pixel values.
		z += z_step;
	}
}

void draw_depth_flat_bottom_triangle(DepthBuffer* db, V4 v0, V4 v1, V4 v2)
{
	// Sort the flat vertices left to right.
	if (v1.x > v2.x)
	{
		v4_swap(&v1, &v2);
	}

	float inv_dy = 1 / (v2.y - v0.y);

	float dxdy0 = (v1.x - v0.x) * inv_dy;
	float dxdy1 = (v2.x - v0.x) * inv_dy;

	float dzdy0 = (v1.z - v0.z) * inv_dy;
	float dzdy1 = (v2.z - v0.z) * inv_dy;

	float dwdy0 = (v1.w - v0.w) * inv_dy;
	float dwdy1 = (v2.w - v0.w) * inv_dy;

	int yStart = (int)(ceil(v0.y - 0.5f));
	int yEnd = (int)(ceil(v2.y - 0.5f));

	for (int y = yStart; y < yEnd; ++y) {
		// Must lerp for the vertex attributes otherwise the accuracy is poor.
		// TODO: Would be nice to not have to actually lerp but step instead.
		float a = (y + 0.5f - v0.y);

		// Calculate the start and ends of the scanline
		float x0 = v0.x + dxdy0 * a;
		float x1 = v0.x + dxdy1 * a;

		float z0 = v0.z + dzdy0 * a;
		float z1 = v0.z + dzdy1 * a;

		float wStart = v0.w + dwdy0 * a;
		float wEnd = v0.w + dwdy1 * a;


		int xStart = (int)(ceilf(x0 - 0.5f));
		int xEnd = (int)(ceilf(x1 - 0.5f));

		draw_depth_scanline(db, xStart, xEnd, y, z0, z1);
	}
}

void draw_depth_flat_top_triangle(DepthBuffer* db, V4 v0, V4 v1, V4 v2)
{
	// Sort the flat vertices left to right.
	if (v0.x > v1.x)
	{
		v4_swap(&v0, &v1);
	}

	float inv_dy = 1 / (v2.y - v0.y);

	float dxdy0 = (v2.x - v0.x) * inv_dy;
	float dxdy1 = (v2.x - v1.x) * inv_dy;

	float dzdy0 = (v2.z - v0.z) * inv_dy;
	float dzdy1 = (v2.z - v1.z) * inv_dy;

	float dwdy0 = (v2.w - v0.w) * inv_dy;
	float dwdy1 = (v2.w - v1.w) * inv_dy;

	int yStart = (int)(ceil(v0.y - 0.5f));
	int yEnd = (int)(ceil(v2.y - 0.5f));

	for (int y = yStart; y < yEnd; ++y) {
		// Must lerp for the vertex attributes to get them accurately.
		// TODO: Would be nice to find a way to step not lerp.
		float a = ((float)y + 0.5f - v0.y);

		float x0 = v0.x + dxdy0 * a;
		float x1 = v1.x + dxdy1 * a;

		float z0 = v0.z + dzdy0 * a;
		float z1 = v1.z + dzdy1 * a;

		float wStart = v0.w + dwdy0 * a;
		float wEnd = v1.w + dwdy1 * a;

		int xStart = (int)(ceil(x0 - 0.5f));
		int xEnd = (int)(ceil(x1 - 0.5f));
		draw_depth_scanline(db, xStart, xEnd, y, z0, z1);
	}
}

void draw_depth_triangle(DepthBuffer* db, V4 v0, V4 v1, V4 v2)
{
	// TODO: I don't think we need w, so should make these V3.

	// Sort vertices in ascending order.
	if (v0.y > v1.y)
	{
		v4_swap(&v0, &v1);
	}
	if (v0.y > v2.y)
	{
		v4_swap(&v0, &v2);
	}
	if (v1.y > v2.y)
	{
		v4_swap(&v1, &v2);
	}

	// Handle if the triangle is already flat.
	if (v0.y == v1.y)
	{
		draw_depth_flat_top_triangle(db, v0, v1, v2);
		return;
	}

	if (v1.y == v2.y)
	{
		draw_depth_flat_bottom_triangle(db, v0, v1, v2);
		return;
	}

	// The triangle isn't flat, so split it into two flat triangles.

	// Linear interpolate for v3.
	float t = (v1.y - v0.y) / (v2.y - v0.y);

	V4 v3 = {
		v0.x + (v2.x - v0.x) * t,
		v1.y,
		v0.z + (v2.z - v0.z) * t,
		v0.w + (v2.w - v0.w) * t
	};

	draw_depth_flat_top_triangle(db, v1, v3, v2);
	draw_depth_flat_bottom_triangle(db, v0, v1, v3);
}

float calculate_diffuse_factor(const V3 v, const V3 n, const V3 light_pos, float a, float b)
{
	// TODO: Comments, check maths etc.

	// calculate the direction of the light to the vertex
	V3 light_dir = v3_sub_v3(light_pos, v);

	float light_distance = size(light_dir);

	v3_mul_eq_f(&light_dir, 1.f / light_distance);

	// Calculate how much the vertex is lit
	float diffuse_factor = max(0.0f, dot(light_dir, n));

	// TODO: Just hardcode the attenuation factors here? Not sure we will need to change them.

	float attenuation = 1.0f / (1.0f + (a * light_distance) + (b * light_distance * light_distance));
	float dp = diffuse_factor * attenuation;

	// TODO: What is the name for this after attentuation is applied to the 
	// diffsue factor?
	return dp;
}

void project(const Canvas* canvas, const M4 projection_matrix, V4 v, V4* out)
{
	// Opengl uses a right handed coordinate system, camera looks down the -z axis,
	// however, NDC space is left handed, from -1 to 1 in all axis. 
	// Therefore, the perspective projection matrix copies and inverts the 
	// initial depth z, to w' in v_projected.

	// Apply the perspective projection matrix to project
	// the 3D coordinates into 2D.
	V4 v_projected;
	m4_mul_v4(projection_matrix, v, &v_projected);

	// Perform perspective divide to bring to NDC space.
	// NDC space is a left handed coordinate system from -1 to 1 in all axis.
	const float inv_w = 1.0f / v_projected.w; // Precalculate the perspective divide.

	v_projected.x *= inv_w;
	v_projected.y *= inv_w;
	v_projected.z *= inv_w;

	// Convert from NDC space to screen space.
	// Convert from [-1:1] to [0:1], then scale to the screen dimensions.

	out->x = (v_projected.x + 1) * 0.5f * canvas->width;
	out->y = (-v_projected.y + 1) * 0.5f * canvas->height;

	// Projecting depth z results in z' which encodes a nonlinear transformation
	// of the depth, just like with x' and y'. So use this to depth test for 
	// more accurate results closer to the camera. Also, this means we only need
	// to recover the depth from w' if the depth test passes, saving a divison
	// per pixel.
	out->z = (v_projected.z + 1) * 0.5f; // Offset from [-1:1] to [0:1]

	// Save inv of w' for perspective correct interpolation. This allows us to lerp
	// between vertex components. 
	out->w = inv_w;
}

void model_to_view_space(Models* models, const M4 view_matrix)
{
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
			m4_mul_v4(model_view_matrix, object_space_position, &view_space_position);

			// TODO: inline v4_write()? or DEFINE?
			view_space_positions[vsp_out_index++] = view_space_position.x;
			view_space_positions[vsp_out_index++] = view_space_position.y;
			view_space_positions[vsp_out_index++] = view_space_position.z;
		}

		// TODO: Only convert the normals after backface culling? We don't need them until lighting.
		//		 Make a function, model_normals_to_view_space.

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

			// TODO: Messy.
			V4 view_space_normal;
			m4_mul_v4(view_normal_matrix, object_space_normal, &view_space_normal);

			V3 vsn_v3 = {
				view_space_normal.x,
				view_space_normal.y,
				view_space_normal.z
			};
			normalise(&vsn_v3);

			view_space_normals[vsn_out_index++] = vsn_v3.x;
			view_space_normals[vsn_out_index++] = vsn_v3.y;
			view_space_normals[vsn_out_index++] = vsn_v3.z;
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
		m4_mul_v4(model_view_matrix, centre, &vs_centre);
		V3 vs_centre_v3 = {
			vs_centre.x,
			vs_centre.y,
			vs_centre.z
		};

		const int bs_index = i * STRIDE_SPHERE;
		mis_bounding_spheres[bs_index] = vs_centre.x;
		mis_bounding_spheres[bs_index + 1] = vs_centre.y;
		mis_bounding_spheres[bs_index + 2] = vs_centre.z;

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

				V3 between = v3_sub_v3(v, vs_centre_v3);

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
		m4_mul_v4(view_matrix, v, &v_view_space);

		// There is no need to save the w component as it is always 1 until 
		// after projection.
		view_space_positions[i] = v_view_space.x;
		view_space_positions[i + 1] = v_view_space.y;
		view_space_positions[i + 2] = v_view_space.z;
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

	float* light_space_front_faces = models->front_face_light_space_positions;
	const float* light_space_positions = models->light_space_positions;


	const float* vertex_colours = models->mis_vertex_colours;

	int face_offset = 0;
	int front_face_out = 0;
	int light_space_front_face_out = 0;
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
				front_faces[front_face_out++] = v0.x;
				front_faces[front_face_out++] = v0.y;
				front_faces[front_face_out++] = v0.z;

				front_faces[front_face_out++] = uvs[index_parts_uv0];
				front_faces[front_face_out++] = uvs[index_parts_uv0 + 1];

				front_faces[front_face_out++] = view_space_normals[index_parts_n0];
				front_faces[front_face_out++] = view_space_normals[index_parts_n0 + 1];
				front_faces[front_face_out++] = view_space_normals[index_parts_n0 + 2];

				front_faces[front_face_out++] = vertex_colours[index_parts_c0];
				front_faces[front_face_out++] = vertex_colours[index_parts_c0 + 1];
				front_faces[front_face_out++] = vertex_colours[index_parts_c0 + 2];

				front_faces[front_face_out++] = v1.x;
				front_faces[front_face_out++] = v1.y;
				front_faces[front_face_out++] = v1.z;

				front_faces[front_face_out++] = uvs[index_parts_uv1];
				front_faces[front_face_out++] = uvs[index_parts_uv1 + 1];

				front_faces[front_face_out++] = view_space_normals[index_parts_n1];
				front_faces[front_face_out++] = view_space_normals[index_parts_n1 + 1];
				front_faces[front_face_out++] = view_space_normals[index_parts_n1 + 2];

				front_faces[front_face_out++] = vertex_colours[index_parts_c1];
				front_faces[front_face_out++] = vertex_colours[index_parts_c1 + 1];
				front_faces[front_face_out++] = vertex_colours[index_parts_c1 + 2];

				front_faces[front_face_out++] = v2.x;
				front_faces[front_face_out++] = v2.y;
				front_faces[front_face_out++] = v2.z;

				front_faces[front_face_out++] = uvs[index_parts_uv2];
				front_faces[front_face_out++] = uvs[index_parts_uv2 + 1];

				front_faces[front_face_out++] = view_space_normals[index_parts_n2];
				front_faces[front_face_out++] = view_space_normals[index_parts_n2 + 1];
				front_faces[front_face_out++] = view_space_normals[index_parts_n2 + 2];

				front_faces[front_face_out++] = vertex_colours[index_parts_c2];
				front_faces[front_face_out++] = vertex_colours[index_parts_c2 + 1];
				front_faces[front_face_out++] = vertex_colours[index_parts_c2 + 2];

				// * 4 because we need x,y,z,w.
				const int index_lsp_parts_v0 = index_v0 * 4;
				const int index_lsp_parts_v1 = index_v1 * 4;
				const int index_lsp_parts_v2 = index_v2 * 4;

				light_space_front_faces[light_space_front_face_out++] = light_space_positions[index_lsp_parts_v0];
				light_space_front_faces[light_space_front_face_out++] = light_space_positions[index_lsp_parts_v0 + 1];
				light_space_front_faces[light_space_front_face_out++] = light_space_positions[index_lsp_parts_v0 + 2];
				light_space_front_faces[light_space_front_face_out++] = light_space_positions[index_lsp_parts_v0 + 3];

				light_space_front_faces[light_space_front_face_out++] = light_space_positions[index_lsp_parts_v1];
				light_space_front_faces[light_space_front_face_out++] = light_space_positions[index_lsp_parts_v1 + 1];
				light_space_front_faces[light_space_front_face_out++] = light_space_positions[index_lsp_parts_v1 + 2];
				light_space_front_faces[light_space_front_face_out++] = light_space_positions[index_lsp_parts_v1 + 3];

				light_space_front_faces[light_space_front_face_out++] = light_space_positions[index_lsp_parts_v2];
				light_space_front_faces[light_space_front_face_out++] = light_space_positions[index_lsp_parts_v2 + 1];
				light_space_front_faces[light_space_front_face_out++] = light_space_positions[index_lsp_parts_v2 + 2];
				light_space_front_faces[light_space_front_face_out++] = light_space_positions[index_lsp_parts_v2 + 3];

				// TODO: TEMP: LEAVING ROOM FOR 4th
				light_space_front_faces[light_space_front_face_out++] = 0;
				light_space_front_faces[light_space_front_face_out++] = 0;
				light_space_front_faces[light_space_front_face_out++] = 0;
				light_space_front_faces[light_space_front_face_out++] = 0;

				
				/*
				printf("backface\n");
				printf("%d %d %d\n", index_lsp_parts_v0, index_lsp_parts_v1, index_lsp_parts_v2);
				printf("%f %f %f %f\n", light_space_positions[index_lsp_parts_v0], light_space_positions[index_lsp_parts_v0 + 1], light_space_positions[index_lsp_parts_v0 + 2], light_space_positions[index_lsp_parts_v0 + 3]);
				printf("%f %f %f %f\n", light_space_positions[index_lsp_parts_v1], light_space_positions[index_lsp_parts_v1 + 1], light_space_positions[index_lsp_parts_v1 + 2], light_space_positions[index_lsp_parts_v1 + 3]);
				printf("%f %f %f %f\n", light_space_positions[index_lsp_parts_v2], light_space_positions[index_lsp_parts_v2 + 1], light_space_positions[index_lsp_parts_v2 + 2], light_space_positions[index_lsp_parts_v2 + 3]);
				printf("\n");
				*/

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

void light_front_faces(Scene* scene)
{
	// TODO: For optimising this, some sort of broad phase could be implemented.
	//		 Potentially when we 

	// Apply lighting to all the front faces.
	// We do this before clipping so if we don't get inconsistent results. 
	float* front_faces = scene->models.front_faces;
	const int* front_faces_counts = scene->models.front_faces_counts;

	int face_offset = 0;

	const int mis_count = scene->models.mis_count;

	const int* passed_broad_phase_flags = scene->models.mis_passed_broad_phase_flags;

	const int point_lights_count = scene->point_lights.count;

	const float* pls_view_space_positions = scene->point_lights.view_space_positions;
	const float* pls_attributes = scene->point_lights.attributes;

	// TODO: Once I stop using float[3] for these I won't have to do dumb stuff like this
	//		 hopefully.
	const V3 ambient_light = scene->ambient_light;

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
				for (int i_light = 0; i_light < point_lights_count; ++i_light)
				{
					int i_light_pos = i_light * STRIDE_POSITION;
					int i_light_attr = i_light * STRIDE_POINT_LIGHT_ATTRIBUTES;

					const V3 light_pos =
					{
						pls_view_space_positions[i_light_pos],
						pls_view_space_positions[i_light_pos + 1],
						pls_view_space_positions[i_light_pos + 2]
					};

					V3 light_colour =
					{
						pls_attributes[i_light_attr],
						pls_attributes[i_light_attr + 1],
						pls_attributes[i_light_attr + 2]
					};

					float strength = pls_attributes[3];

					// TODO: Could cache this? Then the user can also set
					//		 the attenuation?
					float a = 0.1f / strength;
					float b = 0.01f / strength;

					float df = calculate_diffuse_factor(pos, normal, light_pos, a, b);

					v3_mul_eq_f(&light_colour, df);
					v3_add_eq_v3(&diffuse_part, light_colour);
				}

				diffuse_part.x = max(min(diffuse_part.x, 1), ambient_light.x);
				diffuse_part.y = max(min(diffuse_part.y, 1), ambient_light.y);
				diffuse_part.z = max(min(diffuse_part.z, 1), ambient_light.z);

				v3_mul_eq_v3(&colour, diffuse_part);

				// TODO: Add Ambient too.

				front_faces[k + 8] = colour.x;
				front_faces[k + 9] = colour.y;
				front_faces[k + 10] = colour.z;
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
	Scene* scene,
	const Resources* resources)
{
	// TODO: Rename this.

	
	// TODO: Look into this: https://zeux.io/2009/01/31/view-frustum-culling-optimization-introduction/


	// TODO: With 1000 monkeys takes about 160ms.
	
	Models* models = &scene->models;
	PointLights* point_lights = &scene->point_lights;



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

		// TODO: TEMP: DISABLE CLIPPING. for testing shadows.
		num_planes_to_clip_against = 0;

		// Skip the mesh if it's not visible at all.
		if (0 == num_planes_to_clip_against)
		{
			// Entire mesh is visible so just copy the vertices over.
			int index_face = face_offset * STRIDE_ENTIRE_FACE;
			int front_faces_count = models->front_faces_counts[i];

			memcpy(clipped_faces, front_faces + index_face, (size_t)front_faces_count * STRIDE_ENTIRE_FACE * sizeof(float));

			// TODO: TEMP: Copy light positions.
			const int NUM_LIGHTS = 1;

			// x,y,z,w so 4


			

			// TODO: WE WERE NOT LEAVING SPACE FOR MY STUPID 4TH ONE.............
			memcpy(models->temp_light_space_positions, 
				   models->front_face_light_space_positions + face_offset * 4 * 4 * NUM_LIGHTS,
				   (size_t)front_faces_count * 4 * 4 * NUM_LIGHTS * sizeof(float));


			// Draw the clipped face
			project_and_draw_clipped(rt, projection_matrix, scene, i, front_faces_count, resources);
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
						float t = line_intersect_plane(ip0, op0, plane, &p0);

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
						t = line_intersect_plane(ip0, op1, plane, &p1);
						
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

						temp_clipped_faces_out[index_out++] = p0.x;
						temp_clipped_faces_out[index_out++] = p0.y;
						temp_clipped_faces_out[index_out++] = p0.z;
						temp_clipped_faces_out[index_out++] = u0;
						temp_clipped_faces_out[index_out++] = v0;
						temp_clipped_faces_out[index_out++] = nx0;
						temp_clipped_faces_out[index_out++] = ny0;
						temp_clipped_faces_out[index_out++] = nz0;
						temp_clipped_faces_out[index_out++] = r0;
						temp_clipped_faces_out[index_out++] = g0;
						temp_clipped_faces_out[index_out++] = b0;

						temp_clipped_faces_out[index_out++] = p1.x;
						temp_clipped_faces_out[index_out++] = p1.y;
						temp_clipped_faces_out[index_out++] = p1.z;
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
						float t = line_intersect_plane(ip0, op0, plane, &p0);
						
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

						temp_clipped_faces_out[index_out++] = p0.x;
						temp_clipped_faces_out[index_out++] = p0.y;
						temp_clipped_faces_out[index_out++] = p0.z;
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
						t = line_intersect_plane(ip1, op0, plane, &p1);
						
						// Copy the attributes into the new face.
						temp_clipped_faces_out[index_out++] = p0.x;
						temp_clipped_faces_out[index_out++] = p0.y;
						temp_clipped_faces_out[index_out++] = p0.z;
						temp_clipped_faces_out[index_out++] = u0;
						temp_clipped_faces_out[index_out++] = v0;
						temp_clipped_faces_out[index_out++] = nx0;
						temp_clipped_faces_out[index_out++] = ny0;
						temp_clipped_faces_out[index_out++] = nz0;
						temp_clipped_faces_out[index_out++] = r0;
						temp_clipped_faces_out[index_out++] = g0;
						temp_clipped_faces_out[index_out++] = b0;

						temp_clipped_faces_out[index_out++] = p1.x;
						temp_clipped_faces_out[index_out++] = p1.y;
						temp_clipped_faces_out[index_out++] = p1.z;
						temp_clipped_faces_out[index_out++] = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_U], temp_clipped_faces_in[index_op0 + INDEX_U], t);
						temp_clipped_faces_out[index_out++] = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_V], temp_clipped_faces_in[index_op0 + INDEX_V], t);
						temp_clipped_faces_out[index_out++] = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_NX], temp_clipped_faces_in[index_op0 + INDEX_NX], t);
						temp_clipped_faces_out[index_out++] = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_NY], temp_clipped_faces_in[index_op0 + INDEX_NY], t);
						temp_clipped_faces_out[index_out++] = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_NZ], temp_clipped_faces_in[index_op0 + INDEX_NZ], t);
						temp_clipped_faces_out[index_out++] = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_R], temp_clipped_faces_in[index_op0 + INDEX_R], t);
						temp_clipped_faces_out[index_out++] = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_G], temp_clipped_faces_in[index_op0 + INDEX_G], t);
						temp_clipped_faces_out[index_out++] = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_B], temp_clipped_faces_in[index_op0 + INDEX_B], t);

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
				project_and_draw_clipped(rt, projection_matrix, scene, i, num_faces_to_process, resources);
			}
		}

		// Move to the next model instance.
		face_offset += front_faces_counts[i];
	}
}

void project_and_draw_clipped(
	RenderTarget* rt, 
	const M4 projection_matrix, 
	Scene* scene,
	int mi_index, 
	int clipped_face_count,
	const Resources* resources)
{
	// TODO: We don't need normals here. Don't write them in clipping stage.


	Models* models = &scene->models;
	PointLights* point_lights = &scene->point_lights;

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

			V4 pv0, pv1, pv2;
			project(&rt->canvas, projection_matrix, v0, &pv0);
			project(&rt->canvas, projection_matrix, v1, &pv1);
			project(&rt->canvas, projection_matrix, v2, &pv2);

			V3 colour0 = {
				clipped_faces[clipped_face_index + 8] * pv0.w,
				clipped_faces[clipped_face_index + 9] * pv0.w,
				clipped_faces[clipped_face_index + 10] * pv0.w
			};

			V3 colour1 = {
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 8] * pv1.w,
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 9] * pv1.w,
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 10] * pv1.w
			};

			V3 colour2 = {
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 8] * pv2.w,
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 9] * pv2.w,
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 10] * pv2.w
			};

			// TODO: How can draw triangle access all coordinates for each vertex???
			// Just pass the array in? and a number of maps?

			// TODO: Tbf, this approach doesn't seem too bad.
			const int LIGHTS = 1;

			// TODO: ARE THESE OFFSETS CORRECT...........
			// TODO: GET ALL OF THESE CONSISTENT. TOO TIRED NOW.......... REMMEBER WE'RE GIVING ROOM FOR THE 4TH ONE.
			float* light_space = models->temp_light_space_positions + i * LIGHTS * 4 * 4;


			float* lsp0 = light_space;
			float* lsp1 = light_space + LIGHTS * 4; // per light there there is a x,y,z,w
			float* lsp2 = light_space + LIGHTS * 4 * 2;

			/*
			printf("draw\n");
			printf("%f %f %f %f\n", lsp0[0], lsp0[1], lsp0[2], lsp0[3]);
			printf("%f %f %f %f\n", lsp1[0], lsp1[1], lsp1[2], lsp1[3]);
			printf("%f %f %f %f\n", lsp2[0], lsp2[1], lsp2[2], lsp2[3]);
			printf("\n");
			*/


				// TEMP: HARDCODED PERSPECTIVE DIVIDE
			lsp0[0] *= pv0.w;
			lsp0[1] *= pv0.w;
			lsp0[2] *= pv0.w;
			lsp0[3] *= pv0.w;

			lsp1[0] *= pv1.w;
			lsp1[1] *= pv1.w;
			lsp1[2] *= pv1.w;
			lsp1[3] *= pv1.w;

			lsp2[0] *= pv2.w;
			lsp2[1] *= pv2.w;
			lsp2[2] *= pv2.w;
			lsp2[3] *= pv2.w;

			// TODO: Why are z/w always so equal??????????? Might not be a problem. but something defintiely wrong..........

			

			// TODO: Pretty sure we're having lerp problems/issues when swapping?
			draw_triangle(rt, pv0, pv1, pv2, colour0, colour1, colour2, lsp0, lsp1, lsp2, LIGHTS, point_lights->depth_maps);
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
			project(&rt->canvas, projection_matrix, v0, &pv0);
			project(&rt->canvas, projection_matrix, v1, &pv1);
			project(&rt->canvas, projection_matrix, v2, &pv2);

			V2 uv0 =
			{
				clipped_faces[clipped_face_index + 3] * pv0.w,
				clipped_faces[clipped_face_index + 4] * pv0.w,
			};

			V3 colour0 = {
				clipped_faces[clipped_face_index + 8] * pv0.w,
				clipped_faces[clipped_face_index + 9] * pv0.w,
				clipped_faces[clipped_face_index + 10] * pv0.w
			};

			V2 uv1 =
			{
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 3] * pv1.w,
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 4] * pv1.w,
			};

			V3 colour1 = {
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 8] * pv1.w,
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 9] * pv1.w,
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 10] * pv1.w
			};

			V2 uv2 =
			{
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 3] * pv2.w,
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 4] * pv2.w,
			};

			V3 colour2 = {
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 8] * pv2.w,
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 9] * pv2.w,
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + STRIDE_ENTIRE_VERTEX + 10] * pv2.w
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
	update_depth_maps(scene);

	// Draw the depth map temporarily.
	depth_buffer_draw(&scene->point_lights.depth_maps[0], &rt->canvas, rt->canvas.width - scene->point_lights.depth_maps[0].width, 0);




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
	light_front_faces(scene);
	//printf("light_front_faces took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);

	// Draws the front faces by performing the narrow phase of frustum culling
	// and then projecting and rasterising the faces.
	clip_to_screen(rt, settings->projection_matrix, &settings->view_frustum, view_matrix, scene, resources);
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

void update_depth_maps(Scene* scene)
{
	// TODO: 'Alternatively, culling front faces and only rendering the back of objects to the shadow map is sometimes used for a similar result.' 
	//		 NO NEED TO DO FRONT FACES.

	// TODO: Rename shadow maps?

	// TODO: Instead of 6 different textures for a point light, have one big/long cubemap?

	Models* models = &scene->models;
	PointLights* pls = &scene->point_lights;

	for (int i = 0; i < 1; ++i)
	{
		DepthBuffer* depth_map = &pls->depth_maps[i];
		depth_buffer_fill(depth_map, 1.f);

		int pos_i = i * STRIDE_POSITION;

		V3 pos = {
			pls->world_space_positions[pos_i],
			pls->world_space_positions[pos_i + 1],
			pls->world_space_positions[pos_i + 2]
		};
		
		// TODO: TEMP, hardcoded.
		V3 dir = { 0, -0.5, -1 };
		normalise(&dir);

		// Create MVP matrix for light.
		M4 view;
		look_at(v3_mul_f(pos, -1.f), v3_mul_f(dir, -1.f), view);

		// 90 degrees will give us a face of the cube map we want.
		float fov = 90.f;
		float aspect_ratio = depth_map->width / (float)depth_map->height;

		// TODO: TEMP: Hardcoded settings

		float near_plane = 1.f;
		float far_plane = 100.f; // TODO: Defined from strength of point light?

		M4 proj;
		m4_projection(fov, aspect_ratio, near_plane, far_plane, proj);
		
		const int mis_count = models->mis_count;

		const float* mis_transforms = models->mis_transforms;

		const int* mis_base_ids = models->mis_base_ids;

		const int* mbs_positions_counts = models->mbs_positions_counts;
		const int* mbs_positions_offsets = models->mbs_positions_offsets;
		const float* object_space_positions = models->mbs_object_space_positions;

		
		// TODO: Rename vars.

		int positions_offset = 0;

		int light_out_index = 0;

		for (int j = 0; j < mis_count; ++j)
		{
			// Convert the model base object space positions to world space
			// for the current model instance.
			const int mb_index = mis_base_ids[j];
			
			// Calculate the new model/normal matrix from the mi's transform.
			int transform_index = j * STRIDE_MI_TRANSFORM;

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

			M4 model_view;
			m4_mul_m4(view, model_matrix, model_view);

			for (int k = 0; k < models->mbs_faces_counts[mb_index]; ++k)
			{
				const int face_index = (models->mbs_faces_offsets[mb_index] + k) * STRIDE_FACE_VERTICES;

				// Get the indices to the first component of each vertex position.
				const int index_v0 = models->mbs_face_position_indices[face_index] + mbs_positions_offsets[mb_index];
				const int index_v1 = models->mbs_face_position_indices[face_index + 1] + mbs_positions_offsets[mb_index];
				const int index_v2 = models->mbs_face_position_indices[face_index + 2] + mbs_positions_offsets[mb_index];

				const int index_parts_v0 = index_v0 * STRIDE_POSITION;
				const int index_parts_v1 = index_v1 * STRIDE_POSITION;
				const int index_parts_v2 = index_v2 * STRIDE_POSITION;

				// Get the vertices from the face indices.
				V4 osp0 = {
					object_space_positions[index_parts_v0],
					object_space_positions[index_parts_v0 + 1],
					object_space_positions[index_parts_v0 + 2],
					1
				};

				V4 osp1 = {
					object_space_positions[index_parts_v1],
					object_space_positions[index_parts_v1 + 1],
					object_space_positions[index_parts_v1 + 2],
					1
				};

				V4 osp2 = {
					object_space_positions[index_parts_v2],
					object_space_positions[index_parts_v2 + 1],
					object_space_positions[index_parts_v2 + 2],
					1
				};


				// TODO: For this, refactor to do model view transformation first per vertex
				//		 so we're not doing it multiple times with the indexed rendering. 
				//		
				// TODO: This should all work the same as the normal rendering really, frustum
				//		 culling and clipping etc.

				// TODO: Ignore front faces. How do we do this......
				// Looks like we will have to separate projection and model view again :/
				V4 vsp0, vsp1, vsp2;
				m4_mul_v4(model_view, osp0, &vsp0);
				m4_mul_v4(model_view, osp1, &vsp1);
				m4_mul_v4(model_view, osp2, &vsp2);

				V3 vsp0_v3 = { vsp0.x, vsp0.y, vsp0.z };
				V3 vsp1_v3 = { vsp1.x, vsp1.y, vsp1.z };
				V3 vsp2_v3 = { vsp2.x, vsp2.y, vsp2.z };

				// Only fill depth map from back faces.
				if (is_front_face(vsp0_v3, vsp1_v3, vsp2_v3))
				{
					// Now the vertex values for front faces are undefined?
					// TODO: What do we do here?

					/*
					const int index_lsp_parts_v0 = (models->mbs_face_position_indices[face_index] + positions_offset) * 4;
					const int index_lsp_parts_v1 = (models->mbs_face_position_indices[face_index + 1] + positions_offset) * 4;
					const int index_lsp_parts_v2 = (models->mbs_face_position_indices[face_index + 2] + positions_offset) * 4;

					models->light_space_positions[index_lsp_parts_v0] = -100;
					models->light_space_positions[index_lsp_parts_v0 + 1] = -100;
					models->light_space_positions[index_lsp_parts_v0 + 2] = -100;
					models->light_space_positions[index_lsp_parts_v0 + 3] = -100;

					models->light_space_positions[index_lsp_parts_v1] = -100;
					models->light_space_positions[index_lsp_parts_v1 + 1] = -100;
					models->light_space_positions[index_lsp_parts_v1 + 2] = -100;
					models->light_space_positions[index_lsp_parts_v1 + 3] = -100;

					models->light_space_positions[index_lsp_parts_v2] = -100;
					models->light_space_positions[index_lsp_parts_v2 + 1] = -100;
					models->light_space_positions[index_lsp_parts_v2 + 2] = -100;
					models->light_space_positions[index_lsp_parts_v2 + 3] = -100;
					*/

					continue;
				}



				// These coordinates are in clip space, before the perspective divide.
				V4 projected0, projected1, projected2;
				m4_mul_v4(proj, vsp0, &projected0);
				m4_mul_v4(proj, vsp1, &projected1);
				m4_mul_v4(proj, vsp2, &projected2);

				// Do perspective divide etc manually...
				const float inv_w0 = 1.0f / projected0.w;
				const float inv_w1 = 1.0f / projected1.w;
				const float inv_w2 = 1.0f / projected2.w;

				



				// TODO: This is not realllyyy ndc space although kind of is because projection is the same.
				// NDC space is -1:1 for x,y,z.
				V4 ndc0 = {
					projected0.x * inv_w0,
					projected0.y * inv_w0,
					projected0.z * inv_w0,
					inv_w0
				};

				V4 ndc1 = {
					projected1.x * inv_w1,
					projected1.y * inv_w1,
					projected1.z * inv_w1,
					inv_w1
				};

				V4 ndc2 = {
					projected2.x * inv_w2,
					projected2.y * inv_w2,
					projected2.z * inv_w2,
					inv_w2
				};


				// Convert NDC to screen space by first converting to 0-1 in all axis.
				V4 ssp0 = {
					(ndc0.x + 1) * 0.5f * depth_map->width,
					(-ndc0.y + 1) * 0.5f * depth_map->height,
					(ndc0.z + 1) * 0.5f,
					inv_w0
				};

				V4 ssp1 = {
					(ndc1.x + 1) * 0.5f * depth_map->width,
					(-ndc1.y + 1) * 0.5f * depth_map->height,
					(ndc1.z + 1) * 0.5f,
					inv_w1
				};

				V4 ssp2 = {
					(ndc2.x + 1) * 0.5f * depth_map->width,
					(-ndc2.y + 1) * 0.5f * depth_map->height,
					(ndc2.z + 1) * 0.5f,
					inv_w2
				};
				
				/*
				if (projected0.x < -1 || projected0.x > 1 || projected0.y < -1 || projected0.y > 1 || projected0.z < -1 || projected0.z > 1)
				{
					continue;
				}
				if (projected1.x < -1 || projected1.x > 1 || projected1.y < -1 || projected1.y > 1 || projected1.z < -1 || projected1.z > 1)
				{
					continue;
				}
				if (projected2.x < -1 || projected2.x > 1 || projected2.y < -1 || projected2.y > 1 || projected2.z < -1 || projected2.z > 1)
				{
					continue;
				}*/

				// TODO: TEMP? Save the light space positions.
				const int index_lsp_parts_v0 = (models->mbs_face_position_indices[face_index] + positions_offset) * 4;
				const int index_lsp_parts_v1 = (models->mbs_face_position_indices[face_index + 1] + positions_offset) * 4;
				const int index_lsp_parts_v2 = (models->mbs_face_position_indices[face_index + 2] + positions_offset) * 4;

				// Perspective-correct intertpolation of values that have undergone perspective divide dont work?
				models->light_space_positions[index_lsp_parts_v0] = projected0.x;
				models->light_space_positions[index_lsp_parts_v0 + 1] = projected0.y;
				models->light_space_positions[index_lsp_parts_v0 + 2] = projected0.z;
				models->light_space_positions[index_lsp_parts_v0 + 3] = projected0.w; 

				models->light_space_positions[index_lsp_parts_v1] = projected1.x;
				models->light_space_positions[index_lsp_parts_v1 + 1] = projected1.y;
				models->light_space_positions[index_lsp_parts_v1 + 2] = projected1.z;
				models->light_space_positions[index_lsp_parts_v1 + 3] = projected1.w;

				models->light_space_positions[index_lsp_parts_v2] = projected2.x;
				models->light_space_positions[index_lsp_parts_v2 + 1] = projected2.y;
				models->light_space_positions[index_lsp_parts_v2 + 2] = projected2.z;
				models->light_space_positions[index_lsp_parts_v2 + 3] = projected2.w;
				

				draw_depth_triangle(depth_map, ssp0, ssp1, ssp2);
			}
		
			positions_offset += models->mbs_positions_counts[mb_index];

		}
	}
}
