#include "render.h"

#include "renderer.h"

#include "render_target.h"
#include "draw_2d.h"

#include "common/colour.h"

#include "maths/matrix4.h"
#include "maths/vector_maths.h"
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
	// TODO: Globals could be used for the render target pixels and depth buffer to make faster? Maybe? Would need to profile idk.

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

	// Precalculate screen space scaling factors.
	const float hw = 0.5f * db.width;
	const float hh = 0.5f * db.height;

	// TODO: Potentially. Could Calculating things in one go be faster? For example, loop through each and 
	//		 calculate w, then loop through each one and calculate the shadow coords for each? No idea.
	//		 NOTE: This could make querying multiple shadow maps faster as only reading one buffer at a time?
	//		 Only think about this if multiple shadow maps cause a performance issue.
	

	for (unsigned int i = 0; i < dx; ++i)
	{
		// Depth test, only draw closer values.
		if (*depth_buffer > z)
		{
			// Recover w
			const float w = 1.0f / inv_w;

			float r = (c.x * w);
			float g = (c.y * w);
			float b = (c.z * w);

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
				if (pixel_light_depth > light_min_depth) // Constant bias to avoid peter panning.
				{
					if (g_debug_shadows)
					{
						rt->canvas.pixels[index] = COLOUR_LIME;
						*pixels = COLOUR_LIME;
					}
					else
					{
						// We're in shadow, we want to only show ambient light.

						// Could we just lerp the albedo and the 

						// TODO: TEMP: Hardcoded.
						float ambient_r = 0.0f;
						float ambient_g = 0.1f;
						float ambient_b = 0.1f;

						// TODO: Make albedo and diffuse lighting parts vertex attributes to lerp.

						*pixels = float_rgb_to_int(ambient_r, ambient_g, ambient_b);
						/*
						
						shadow colour = ambient * albedo 
						
						
						otherwise the shadow will just be black, and that's not okay because
						the shadows will look wrong..
						
						*/

						/*
						if (r > 0)
						{
							r = ambient_r;
						}
						else
						{
							r = 0;
						}
						if (g > 0)
						{
							g = ambient_g;
						}
						else
						{
							g = 0;
						}
						if (b > 0)
						{
							b = ambient_b;
						}
						else
						{
							b = 0;
						}
						
						
						
						// TODO: Ambient/ combine all lights.
						//printf("%f %f\n", pixel_light_depth, light_min_depth);

						*pixels = float_rgb_to_int(r, g, b);*/
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

void draw_flat_bottom_triangle(RenderTarget* rt, float* vc0, float* vc1, float* vc2, int vertex_stride, int lights_count, DepthBuffer* depth_maps)
{
	// Sort the flat vertices left to right.
	if (vc1[0] > vc2[0])
	{  
		float* temp = vc1;
		vc1 = vc2;
		vc2 = temp;
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
		(lsp1[0] - lsp0[0]) * inv_dy,
		(lsp1[1] - lsp0[1]) * inv_dy,
		(lsp1[2] - lsp0[2]) * inv_dy,
		(lsp1[3] - lsp0[3]) * inv_dy
	};

	float lsp1dy[4] = {
		(lsp2[0] - lsp0[0]) * inv_dy,
		(lsp2[1] - lsp0[1]) * inv_dy,
		(lsp2[2] - lsp0[2]) * inv_dy,
		(lsp2[3] - lsp0[3]) * inv_dy
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

void draw_flat_top_triangle(RenderTarget* rt, float* vc0, float* vc1, float* vc2, int vertex_stride, int lights_count, DepthBuffer* depth_maps)
{
	// Sort the flat vertices left to right.
	if (v0.x > v1.x) 
	{	
		v4_swap(&v0, &v1);
		v3_swap(&c0, &c1);

		float* temp = lsp0;
		lsp0 = lsp1;
		lsp1 = temp;
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
		(lsp2[0] - lsp0[0]) * inv_dy,
		(lsp2[1] - lsp0[1]) * inv_dy,
		(lsp2[2] - lsp0[2]) * inv_dy,
		(lsp2[3] - lsp0[3]) * inv_dy
	};

	float lsp1dy[4] = {
		(lsp2[0] - lsp1[0]) * inv_dy,
		(lsp2[1] - lsp1[1]) * inv_dy,
		(lsp2[2] - lsp1[2]) * inv_dy,
		(lsp2[3] - lsp1[3]) * inv_dy
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
			lsp0[0] + lsp0dy[0] * a, 
			lsp0[1] + lsp0dy[1] * a, 
			lsp0[2] + lsp0dy[2] * a, 
			lsp0[3] + lsp0dy[3] * a };

		float lsp_temp1[4] = { 
			lsp1[0] + lsp1dy[0] * a, 
			lsp1[1] + lsp1dy[1] * a, 
			lsp1[2] + lsp1dy[2] * a, 
			lsp1[3] + lsp1dy[3] * a };

		draw_scanline(rt, xStart, xEnd, y, z0, z1, wStart, wEnd, tempc0, tempc1, lsp_temp0, lsp_temp1, lights_count, depth_maps);
	}
}

void draw_triangle(RenderTarget* rt, float* vc0, float* vc1, float* vc2, float* vc3, int vertex_stride, int lights_count, DepthBuffer* depth_maps)
{
	// vc = vertex components

	// TODO: These refactors. Although, once I get to the scanline, not sure how that will work.
	// TODO: Gotta think about this args, do I want it done in an array like this..... no clue. Not sure what is best.

	// TODO: Pass in a stride for the size of the vertex?
	
	// TODO: Store like: pos, albedo, diffuse, light_pos0, light_pos1, ... for each vertex. 
	// So stride is: 4 + 3 + 3 + 4 * lights_count?


	// A lot of information... oh well.
	
	V4 v0 = v4_read(vc0);
	V4 v1 = v4_read(vc1);
	V4 v2 = v4_read(vc1);

	// Sort vertices in ascending order.
	if (v0.y > v1.y) 
	{ 
		float* temp = vc0;
		vc0 = vc1;
		vc1 = temp;
	}
	if (v0.y > v2.y)
	{
		float* temp = vc0;
		vc0 = vc2;
		vc2 = temp;
	}
	if (v1.y > v2.y) 
	{ 
		float* temp = vc1;
		vc1 = vc2;
		vc2 = temp;
	}
	
	// Handle if the triangle is already flat.
	if (v0.y == v1.y)
	{
		draw_flat_top_triangle(rt, vc0, vc1, vc2, vc3, vertex_stride, lights_count, depth_maps);
		return;
	}

	if (v1.y == v2.y)
	{
		draw_flat_bottom_triangle(rt, vc0, vc1, vc2, vc3, vertex_stride, lights_count, depth_maps);
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

	v4_write(vc3, v3);


	// TODO: LERP FOR ALBEDO ETC INSTEAD OF COLOUR. this should be worth it because the plan is more ambient less lights.

	// Lerp for the new colour of the vertex.
	V3 c3 = v3_add_v3(c0, v3_mul_f(v3_sub_v3(c2, c0), t));


	// Where can we get space for a v4 array..... pass one in???

	
	// TODO: TEMP HARDCODED FOR 1
	

	
	vc3[0] = vc0[0] + (vc2[0] - vc0[0]) * t;
	vc3[1] = vc0[1] + (vc2[1] - vc0[1]) * t;
	vc3[2] = vc0[2] + (vc2[2] - vc0[2]) * t;
	vc3[3] = vc0[3] + (vc2[3] - vc0[3]) * t;

	draw_flat_top_triangle(rt, vc1, vc3, vc2, vertex_stride, lights_count, depth_maps);
	draw_flat_bottom_triangle(rt, vc0, vc1, vc3, vertex_stride, lights_count, depth_maps);
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

		V3 position = v3_read(mis_transforms + transform_index);
		V3 eulers = v3_read(mis_transforms + transform_index + 3);
		V3 scale = v3_read(mis_transforms + transform_index + 6);

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

			V4 object_space_position = v3_read_to_v4(object_space_positions + index_object_space_position, 1.f);

			V4 view_space_position; 
			m4_mul_v4(model_view_matrix, object_space_position, &view_space_position);

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

			V4 object_space_normal = v3_read_to_v4(object_space_normals + index_object_space_normals, 0.f);

			// TODO: Kinda messy.
			V4 view_space_normal;
			m4_mul_v4(view_normal_matrix, object_space_normal, &view_space_normal);

			v3_write(view_space_normals + vsn_out_index, normalised(v4_xyz(view_space_normal)));
			vsn_out_index += 3;
		}

		// Update the mi's bounding sphere.
		V4 centre = v3_read_to_v4(object_space_centres + mb_index * STRIDE_POSITION, 1.f);

		// Convert the model base centre to view space for the instance.
		V4 vs_centre;
		m4_mul_v4(model_view_matrix, centre, &vs_centre);
		V3 vs_centre_v3 = v4_xyz(vs_centre);
		
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
				V3 v = v3_read(view_space_positions + j);

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
		V4 v_world_space = v3_read_to_v4(world_space_positions + i, 1.f);

		V4 v_view_space;
		m4_mul_v4(view_matrix, v_world_space, &v_view_space);

		// There is no need to save the w component as it is always 1 until 
		// after projection.
		v4_write_xyz(view_space_positions + i, v_view_space);
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

void cull_backfaces(Renderer* renderer, Scene* scene)
{
	// TODO: Would it be quicker to copy or not.
	Models* models = &scene->models;
	int shadow_maps_count = scene->point_lights.count;


	// TODO: Also, does a step like backface culling gain anything from doing it all at once?

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

	float* light_space_front_faces = renderer->buffers.front_face_light_space_positions;
	const float* light_space_positions = renderer->buffers.light_space_positions;


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
			const V3 v0 = v3_read(view_space_positions + index_parts_v0);
			const V3 v1 = v3_read(view_space_positions + index_parts_v1);
			const V3 v2 = v3_read(view_space_positions + index_parts_v2);

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

				// Light space positions are wrote out light by light.
				const int index_lsp_parts_v0 = index_v0 * STRIDE_V4;
				const int index_lsp_parts_v1 = index_v1 * STRIDE_V4;
				const int index_lsp_parts_v2 = index_v2 * STRIDE_V4;

				// Copy all the face vertex data.
				// We copy the attributes over here as well because when clipping we need the data
				// all together for lerping.
				
				// TODO: fix the order of writing out.s

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

				// Diffuse contribution
				front_faces[front_face_out++] = 0;
				front_faces[front_face_out++] = 0;
				front_faces[front_face_out++] = 0;

				for (int k = 0; k < shadow_maps_count; ++k)
				{
					// TODO: Is this right...
					int lsp_index = index_lsp_parts_v0 + models->mis_total_faces * STRIDE_FACE_VERTICES * STRIDE_V4;
					front_faces[front_face_out++] = light_space_positions[lsp_index];
					front_faces[front_face_out++] = light_space_positions[lsp_index + 1];
					front_faces[front_face_out++] = light_space_positions[lsp_index + 2];
					front_faces[front_face_out++] = light_space_positions[lsp_index + 3];
				}

				// TODO: Write out the light space positions.

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

				// Diffuse contribution
				front_faces[front_face_out++] = 0;
				front_faces[front_face_out++] = 0;
				front_faces[front_face_out++] = 0;

				for (int k = 0; k < shadow_maps_count; ++k)
				{
					// TODO: Is this right...
					int lsp_index = index_lsp_parts_v1 + models->mis_total_faces * STRIDE_FACE_VERTICES * STRIDE_V4;
					front_faces[front_face_out++] = light_space_positions[lsp_index];
					front_faces[front_face_out++] = light_space_positions[lsp_index + 1];
					front_faces[front_face_out++] = light_space_positions[lsp_index + 2];
					front_faces[front_face_out++] = light_space_positions[lsp_index + 3];
				}

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

				// Diffuse contribution
				front_faces[front_face_out++] = 0;
				front_faces[front_face_out++] = 0;
				front_faces[front_face_out++] = 0;

				for (int k = 0; k < shadow_maps_count; ++k)
				{
					// TODO: Is this right...
					int lsp_index = index_lsp_parts_v2 + models->mis_total_faces * STRIDE_FACE_VERTICES * STRIDE_V4;
					front_faces[front_face_out++] = light_space_positions[lsp_index];
					front_faces[front_face_out++] = light_space_positions[lsp_index + 1];
					front_faces[front_face_out++] = light_space_positions[lsp_index + 2];
					front_faces[front_face_out++] = light_space_positions[lsp_index + 3];
				}

				/*
				// TODO: TODO: TODO:
				// * 4 because we need x,y,z,w.
				

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
				*/

				
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
			int index_face = j * STRIDE_BASE_FRONT_FACE;

			// For each vertex calculate the diffuse contribution.
			for (int k = index_face; k < index_face + STRIDE_BASE_FRONT_FACE; k += STRIDE_BASE_FRONT_VERTEX)
			{
				const V3 pos = v3_read(front_faces + k);
				const V3 normal = v3_read(front_faces + k + 5);

				// The base colour of the surface under diffuse lighting.
				V3 albedo = v3_read(front_faces + k + 8);
				
				// The total diffuse light the vertex receives.
				V3 diffuse_part = { 0, 0, 0 };

				// For each light
				for (int i_light = 0; i_light < point_lights_count; ++i_light)
				{
					// Read the light's properties.
					const V3 light_pos = v3_read(pls_view_space_positions + i_light * STRIDE_POSITION);

					int i_light_attr = i_light * STRIDE_POINT_LIGHT_ATTRIBUTES;
					V3 light_colour = v3_read(pls_attributes + i_light_attr);
					float strength = pls_attributes[i_light_attr + 3];

					float a = 0.1f / strength;
					float b = 0.01f / strength;

					float df = calculate_diffuse_factor(pos, normal, light_pos, a, b);

					v3_mul_eq_f(&light_colour, df);
					v3_add_eq_v3(&diffuse_part, light_colour);
				}

				// Clamp diffuse contribution to a valid range 0-1. 
				diffuse_part.x = min(max(diffuse_part.x, 0.f), 1.f);
				diffuse_part.y = min(max(diffuse_part.y, 0.f), 1.f);
				diffuse_part.z = min(max(diffuse_part.z, 0.f), 1.f);

				/*
				v3_mul_eq_v3(&diffuse_part, albedo);
				
				// Combine the ambient and diffuse contributions to calculate the vertex colour.
				V3 colour = v3_add_v3(diffuse_part, v3_mul_v3(albedo, ambient_light));
				
				colour.x = min(colour.x, 1.f);
				colour.y = min(colour.y, 1.f);
				colour.z = min(colour.z, 1.f);
				*/

				// Write out the calculated diffuse part of the vertex.
				// If we introduce specular, this can include that.
				v3_write(front_faces + k + 11, diffuse_part);
			}
		}
	
		face_offset += front_faces_count;
	}
}

void clip_to_screen(
	Renderer* renderer,
	const M4 view_matrix, 
	Scene* scene,
	const Resources* resources)
{
	// TODO: Rename this.

	
	// TODO: Look into this: https://zeux.io/2009/01/31/view-frustum-culling-optimization-introduction/

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

		// Calculate the number of components per vertex.
		// TODO: This will also be calculated for the front_faces and when drawing, 
		//		 should we share?

		// TODO: Atm we don't really want the normal after this. 
		const int VERTEX_COMPONENTS = STRIDE_BASE_FRONT_VERTEX *
			scene->point_lights.count *
			STRIDE_V4;

		// Skip the mesh if it's not visible at all.
		if (0 == num_planes_to_clip_against)
		{
			// Entire mesh is visible so just copy the vertices over.
			int index_face = face_offset * VERTEX_COMPONENTS;
			int front_faces_count = models->front_faces_counts[i];

			// TODO: How do we avoid copying the normals.
			memcpy(clipped_faces, front_faces + index_face, (size_t)front_faces_count * VERTEX_COMPONENTS * sizeof(float));
		
			// Draw the clipped face
			project_and_draw_clipped(renderer, scene, i, front_faces_count, resources);
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
				const Plane* plane = &renderer->settings.view_frustum.planes[intersected_planes[intersected_planes_index++]];
				
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
					int index_face = j * VERTEX_COMPONENTS * STRIDE_FACE_VERTICES;

					int num_inside_points = 0;
					int num_outside_points = 0;

					int inside_points_indices[3] = { 0 };
					int outside_points_indices[3] = { 0 };

					const int index_v0 = index_face;
					const int index_v1 = index_face + VERTEX_COMPONENTS;
					const int index_v2 = index_face + VERTEX_COMPONENTS + VERTEX_COMPONENTS;

					const V3 v0 = v3_read(temp_clipped_faces_in + index_v0);
					const V3 v1 = v3_read(temp_clipped_faces_in + index_v1);
					const V3 v2 = v3_read(temp_clipped_faces_in + index_v2);

					float d0 = signed_distance(plane, v0);
					float d1 = signed_distance(plane, v1);
					float d2 = signed_distance(plane, v2);

					// Determine what points are inside and outside the plane.
					if (d0 >= 0)
					{
						inside_points_indices[num_inside_points++] = index_v0;
					}
					else
					{
						outside_points_indices[num_outside_points++] = index_v0;
					}
					if (d1 >= 0)
					{
						inside_points_indices[num_inside_points++] = index_v1;
					}
					else
					{
						outside_points_indices[num_outside_points++] = index_v1;
					}
					if (d2 >= 0)
					{
						inside_points_indices[num_inside_points++] = index_v2;
					}
					else
					{
						outside_points_indices[num_outside_points++] = index_v2;
					}

					if (num_inside_points == 3)
					{
						// The whole triangle is inside the plane, so copy the face.
						int index_face = j * VERTEX_COMPONENTS;

						// TODO: How do we avoid copying the normal.
						memcpy(temp_clipped_faces_out + index_out, temp_clipped_faces_in + index_face, VERTEX_COMPONENTS * sizeof(float));

						index_out += VERTEX_COMPONENTS;
						++temp_visible_faces_count;
					}
					else if (num_inside_points == 1 && num_outside_points == 2)
					{
						// Form a new triangle with the plane edge.
						// Unpack the points.
						const int index_ip0 = inside_points_indices[0];
						const int index_op0 = outside_points_indices[0];
						const int index_op1 = outside_points_indices[1];

						const V3 ip0 = v3_read(temp_clipped_faces_in + index_ip0);
						const V3 op0 = v3_read(temp_clipped_faces_in + index_op0);
						const V3 op1 = v3_read(temp_clipped_faces_in + index_op1);
						
						// Calculate how many components in the total vertex, including
						// light space positions.
						const int vertex_components = STRIDE_FRONT_FACE_VERTEX *
							scene->point_lights.count *
							STRIDE_POSITION;

						// There are 14 attributes total... 
						// without light positions.

						// Copy the inside vertex.
						// TODO: Memcpy might not be faster here should profile.
						memcpy(
							temp_clipped_faces_out + index_out, 
							temp_clipped_faces_in + index_ip0, 
							vertex_components * sizeof(float)
						);
						index_out += vertex_components;

						// Lerp for the first new vertex.
						V3 p0;
						float t = line_intersect_plane(ip0, op0, plane, &p0);

						temp_clipped_faces_out[index_out++] = p0.x;
						temp_clipped_faces_out[index_out++] = p0.y;
						temp_clipped_faces_out[index_out++] = p0.z;

						// Lerp the vertex components straight into the out buffer.
						const int COMPS_TO_LERP = 11 + scene->point_lights.count * STRIDE_POSITION;
						for (int k = 0; k < COMPS_TO_LERP; ++k)
						{
							temp_clipped_faces_out[index_out++] = lerp(temp_clipped_faces_in[index_ip0 + STRIDE_POSITION + k], temp_clipped_faces_in[index_op0 + STRIDE_POSITION + k], t);
						}

						// Lerp for the second vertex.
						V3 p1;
						t = line_intersect_plane(ip0, op1, plane, &p1);

						temp_clipped_faces_out[index_out++] = p1.x;
						temp_clipped_faces_out[index_out++] = p1.y;
						temp_clipped_faces_out[index_out++] = p1.z;

						for (int k = 0; k < COMPS_TO_LERP; ++k)
						{
							temp_clipped_faces_out[index_out++] = lerp(temp_clipped_faces_in[index_ip0 + STRIDE_POSITION + k], temp_clipped_faces_in[index_op1 + STRIDE_POSITION + k], t);
						}
						
						++temp_visible_faces_count;
					}
					else if (num_inside_points == 2 && num_outside_points == 1)
					{
						// Form two new triangles with the plane edge.
						// Unpack the points.
						const int index_ip0 = inside_points_indices[0];
						const int index_ip1 = inside_points_indices[1];
						const int index_op0 = outside_points_indices[0];

						const V3 ip0 = v3_read(temp_clipped_faces_in + index_ip0);
						const V3 ip1 = v3_read(temp_clipped_faces_in + index_ip1);
						const V3 op0 = v3_read(temp_clipped_faces_in + index_op0);

						const int vertex_components = STRIDE_FRONT_FACE_VERTEX *
							scene->point_lights.count *
							STRIDE_POSITION;

						// Copy the first inside vertex.
						memcpy(temp_clipped_faces_out + index_out,
							temp_clipped_faces_in + index_ip0,
							vertex_components * sizeof(float)
						);
						index_out += vertex_components;

						// Copy the second inside vertex.
						memcpy(
							temp_clipped_faces_out + index_out,
							temp_clipped_faces_in + index_ip1,
							vertex_components * sizeof(float)
						);
						index_out += vertex_components;

						// Lerp for the first new vertex.
						V3 p0;
						float t = line_intersect_plane(ip0, op0, plane, &p0);

						temp_clipped_faces_out[index_out++] = p0.x;
						temp_clipped_faces_out[index_out++] = p0.y;
						temp_clipped_faces_out[index_out++] = p0.z;

						// Copy the index for where we write the lerped components to, so
						// we can copy them for the second triangle.
						int new_v0_index = index_out;
						
						const int COMPS_TO_LERP = 11 + scene->point_lights.count * STRIDE_POSITION;
						for (int k = 0; k < COMPS_TO_LERP; ++k)
						{
							temp_clipped_faces_out[index_out++] = lerp(temp_clipped_faces_in[index_ip0 + STRIDE_POSITION + k], temp_clipped_faces_in[index_op0 + STRIDE_POSITION + k], t);
						}
						
						++temp_visible_faces_count;

						// First triangle done.
						
						// Copy the first new vertex for the second triangle.
						for (int k = new_v0_index; k < new_v0_index + vertex_components; ++k)
						{
							temp_clipped_faces_out[index_out++] = temp_clipped_faces_out[k];
						}

						// Lerp for the second new point.
						V3 p1;
						t = line_intersect_plane(ip1, op0, plane, &p1);
						
						temp_clipped_faces_out[index_out++] = p1.x;
						temp_clipped_faces_out[index_out++] = p1.y;
						temp_clipped_faces_out[index_out++] = p1.z;

						for (int k = 0; k < COMPS_TO_LERP; ++k)
						{
							temp_clipped_faces_out[index_out++] = lerp(temp_clipped_faces_in[index_ip1 + STRIDE_POSITION + k], temp_clipped_faces_in[index_op0 + STRIDE_POSITION + k], t);
						}

						// Copy over the inside vertex for this triangle.
						memcpy(
							temp_clipped_faces_out + index_out,
							temp_clipped_faces_in + index_ip1,
							vertex_components * sizeof(float)
						);

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
				project_and_draw_clipped(renderer, scene, i, num_faces_to_process, resources);
			}
		}

		// Move to the next model instance.
		face_offset += front_faces_counts[i];
	}
}

void project_and_draw_clipped(
	Renderer* renderer,
	Scene* scene,
	int mi_index, 
	int clipped_face_count,
	const Resources* resources)
{
	// TODO: We don't need normals here. Don't write them in clipping stage?
	// TODO: This means that in draw triangle etc we have to ignore the normal
	//		 which is making the logic annoying. How can I remove the normal
	//		 in the clipping stage? - I guess for now it's okay. but definitely needs
	//		 to be fixed.


	// TODO: This should take in a vertex buffer pointer. This would mean
	//		 if we don't do any clipping, we don't need to copy data and 
	//		 can draw straight from the buffer


	RenderTarget* rt = &renderer->target;
	Models* models = &scene->models;
	PointLights* point_lights = &scene->point_lights;

	// TODO: Comments. This function renders out the triangles in the clipped face buffer. 
	const float* clipped_faces = models->clipped_faces;

	// TODO: Refactor, how do I get rid of this duplicated code.

	const int CLIPPED_VERTEX_COMPONENTS = STRIDE_BASE_CLIPPED_VERTEX *
		scene->point_lights.count *
		STRIDE_POSITION;

	const int texture_index = models->mis_texture_ids[mi_index];
	if (texture_index == -1)
	{
		for (int i = 0; i < clipped_face_count; ++i)
		{
			int clipped_face_index = i * CLIPPED_VERTEX_COMPONENTS * STRIDE_FACE_VERTICES;

			// Project the vertex positions.
			const V4 v0 = v3_read_to_v4(clipped_faces + clipped_face_index, 1.f);
			const V4 v1 = v3_read_to_v4(clipped_faces + clipped_face_index + CLIPPED_VERTEX_COMPONENTS, 1.f);
			const V4 v2 = v3_read_to_v4(clipped_faces + clipped_face_index + CLIPPED_VERTEX_COMPONENTS + CLIPPED_VERTEX_COMPONENTS, 1.f);

			V4 pv0, pv1, pv2;
			project(&rt->canvas, renderer->settings.projection_matrix, v0, &pv0);
			project(&rt->canvas, renderer->settings.projection_matrix, v1, &pv1);
			project(&rt->canvas, renderer->settings.projection_matrix, v2, &pv2);

			// TODO: Just write straight into buffer probably.
			V3 albedo0 = v3_read(clipped_faces + clipped_face_index + 8);
			V3 albedo1 = v3_read(clipped_faces + clipped_face_index + CLIPPED_VERTEX_COMPONENTS + 8);
			V3 albedo2 = v3_read(clipped_faces + clipped_face_index + CLIPPED_VERTEX_COMPONENTS + CLIPPED_VERTEX_COMPONENTS + 8);

			V3 diffuse0 = v3_read(clipped_faces + clipped_face_index + 11);
			V3 diffuse1 = v3_read(clipped_faces + clipped_face_index + CLIPPED_VERTEX_COMPONENTS + 11);
			V3 diffuse2 = v3_read(clipped_faces + clipped_face_index + CLIPPED_VERTEX_COMPONENTS + CLIPPED_VERTEX_COMPONENTS + 11);

			/* TODO: I think this is actually setup correctly.
			//		 Just a few more steps:
			//			2) Update flat triangle methods
			//			3) Update scanline methods
			*/ 
			// Calculate pointers to vertex data.
			float* tri_data = renderer->buffers.triangle_vertices;

			// Load in data for each vertex.

			// TODO: Could be nice to have this as a stride?
			// pos, albedo, diffuse, light space pos * count
			const int STRIDE = 4 + 3 + 3 + 4 * point_lights->count;

			// Load the data into the triangle buffer.
			float* vc0 = tri_data;
			float* vc1 = tri_data + STRIDE;
			float* vc2 = tri_data + STRIDE * 2;
			float* vc3 = tri_data + STRIDE * 3;


			// Front faces need a pos (V3), UV (V2), normal (V3), albedo (V3), diffuse (V3)
			vc0[0] = pv0.x;
			vc0[1] = pv0.y;
			vc0[2] = pv0.z;
			vc0[3] = pv0.w;

			vc0[4] = albedo0.x;
			vc0[5] = albedo0.y;
			vc0[6] = albedo0.z;

			vc0[7] = diffuse0.x;
			vc0[8] = diffuse0.y;
			vc0[9] = diffuse0.z;

			int offset = 10;
			for (int j = 0; j < point_lights->count; ++j)
			{
				int lsp_index = clipped_face_index + STRIDE_BASE_CLIPPED_VERTEX;
				vc0[offset++] = clipped_faces[lsp_index];
				vc0[offset++] = clipped_faces[lsp_index + 1];
				vc0[offset++] = clipped_faces[lsp_index + 2];
				vc0[offset++] = clipped_faces[lsp_index + 3];
			}

			// Apply perspective divide to all components.
			for (int j = 4; j < STRIDE; ++j)
			{
				vc0[j] *= pv0.w;
			}

			// TODO: Next vertices.

			



			/*
			// Perspective divide for all light space positions.
			// Hardcoded offset to light space position.
			const int offset_to_lsp = 10; 

			for (int j = 0; j < point_lights->count; ++j)
			{
				int index = STRIDE * j + offset_to_lsp;

				// Perform perspective divide for camera perspective.
				vc0[index] *= pv0.w;
				vc0[index + 1] *= pv0.w;
				vc0[index + 2] *= pv0.w;
				vc0[index + 3] *= pv0.w;

				vc1[index] *= pv1.w;
				vc1[index + 1] *= pv1.w;
				vc1[index + 2] *= pv1.w;
				vc1[index + 3] *= pv1.w;

				vc2[index] *= pv2.w;
				vc2[index + 1] *= pv2.w;
				vc2[index + 2] *= pv2.w;
				vc2[index + 3] *= pv2.w;
			}
			*/

			




			// 
			draw_triangle(rt, vc0, vc1, vc2, vc3, STRIDE, point_lights->count, point_lights->depth_maps);
		}
	}
	else
	{
		/*
		Canvas* texture = &resources->textures[texture_index];
		for (int i = 0; i < clipped_face_count; ++i)
		{
			int clipped_face_index = i * STRIDE_BASE_CLIPPED_FACE;

			// Project the vertex positions.
			const V4 v0 = v3_read_to_v4(clipped_faces + clipped_face_index, 1.f);
			const V4 v1 = v3_read_to_v4(clipped_faces + clipped_face_index + STRIDE_BASE_CLIPPED_FACE_VERTEX, 1.f);
			const V4 v2 = v3_read_to_v4(clipped_faces + clipped_face_index + STRIDE_BASE_CLIPPED_FACE_VERTEX + STRIDE_BASE_CLIPPED_FACE_VERTEX, 1.f);

			V4 pv0, pv1, pv2;
			project(&rt->canvas, renderer->settings.projection_matrix, v0, &pv0);
			project(&rt->canvas, renderer->settings.projection_matrix, v1, &pv1);
			project(&rt->canvas, renderer->settings.projection_matrix, v2, &pv2);

			V3 colour0 = v3_mul_f(v3_read(clipped_faces + clipped_face_index + 8), pv0.w);
			V3 colour1 = v3_mul_f(v3_read(clipped_faces + clipped_face_index + STRIDE_BASE_CLIPPED_FACE_VERTEX + 8), pv1.w);
			V3 colour2 = v3_mul_f(v3_read(clipped_faces + clipped_face_index + STRIDE_BASE_CLIPPED_FACE_VERTEX + STRIDE_BASE_CLIPPED_FACE_VERTEX + 8), pv2.w);

			V2 uv0 =
			{
				clipped_faces[clipped_face_index + 3] * pv0.w,
				clipped_faces[clipped_face_index + 4] * pv0.w,
			};

			V2 uv1 =
			{
				clipped_faces[clipped_face_index + STRIDE_BASE_CLIPPED_FACE_VERTEX + 3] * pv1.w,
				clipped_faces[clipped_face_index + STRIDE_ENTIRE_VERTEX + 4] * pv1.w,
			};

			V2 uv2 =
			{
				clipped_faces[clipped_face_index + STRIDE_BASE_CLIPPED_VERTEX + STRIDE_BASE_CLIPPED_VERTEX + 3] * pv2.w,
				clipped_faces[clipped_face_index + STRIDE_BASE_CLIPPED_VERTEX + STRIDE_BASE_CLIPPED_VERTEX + 4] * pv2.w,
			};

			draw_textured_triangle(rt, pv0, pv1, pv2, colour0, colour1, colour2, uv0, uv1, uv2, texture);
		}*/
	}
}

void render(
	Renderer* renderer, 
	Scene* scene, 
	const Resources* resources,
	const M4 view_matrix)
{
	// TODO: Renderer has camera, but view matrix is passed separate? Refactor.


	update_depth_maps(renderer, scene);

	// Draw the depth map temporarily.
	//depth_buffer_draw(&scene->point_lights.depth_maps[0], &rt->canvas, rt->canvas.width - scene->point_lights.depth_maps[0].width, 0);

	Timer t = timer_start();
	
	// Transform object space positions to view space.
	model_to_view_space(&scene->models, view_matrix);
	//printf("model_to_view_space took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);

	lights_world_to_view_space(&scene->point_lights, view_matrix);
	//printf("lights_world_to_view_space took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);

	// Perform broad phase frustum culling to avoid unnecessary backface culling.
	broad_phase_frustum_culling(&scene->models, &renderer->settings.view_frustum);
	//printf("broad_phase_frustum_culling took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);

	// Perform backface culling.
	cull_backfaces(renderer, &scene->models);
	//printf("cull_backfaces took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);

	// Apply lighting here so that the if a vertex is clipped closer
	// to the light, the lighing doesn't change.
	light_front_faces(scene);
	//printf("light_front_faces took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);

	// Draws the front faces by performing the narrow phase of frustum culling
	// and then projecting and rasterising the faces.
	clip_to_screen(renderer, view_matrix, scene, resources);
	//printf("clip_to_screen took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);



	// TEMP: Debugging

	debug_draw_point_lights(&renderer->target.canvas, &renderer->settings, &scene->point_lights);
	//printf("debug_draw_point_lights took: %d\n", timer_get_elapsed(&t));
	timer_restart(&t);
	
	// Draw crosshair temporarily cause looks cool.
	int r = 2;
	int cx = (int)(renderer->target.canvas.width / 2.f);
	int cy = (int)(renderer->target.canvas.height / 2.f);
	draw_rect(&renderer->target.canvas, cx - r, cy - r, cx + r, cy + r, COLOUR_WHITE);

	if (g_draw_normals)
	{
		debug_draw_mi_normals(&renderer->target.canvas, &renderer->settings, &scene->models, 0);
	}
}

void update_depth_maps(Renderer* renderer, const Scene* scene)
{

	// TODO: To avoid recalculating shadows for each mesh. We should only do it for ones that have moved.
	//		 This should save us a lot of computation I believe. But also means we need a static and dynamic
	//		 depth buffer. Something like this anyways too tired to think of now. 
	// 
	//		 Need to look into this properly.
	//		 But for example, for the main map, that's not moving, we might have a single directional (or other) light as the 
	//		 sun, we could generate a depth map from all the static geometry, we would have to draw dynamic to a separate. But
	//		 this would give us proper shadows and essentially just make it a texture lookup.

	// TODO: At some point we definitely want to be able to render a directional light for the sun/moon, 
	//		 then the environment can have a static shadow map. Then the dynamic stuff can be renderered to a separate map.
	
	// TODO: Rename shadow maps?

	// TODO: Instead of 6 different textures for a point light, have one big/long cubemap?

	const Models* models = &scene->models;
	const PointLights* pls = &scene->point_lights;

	for (int i = 0; i < pls->count; ++i)
	{
		DepthBuffer* depth_map = &pls->depth_maps[i];
		depth_buffer_fill(depth_map, 1.f);

		int pos_i = i * STRIDE_POSITION;

		V3 pos = v3_read(pls->world_space_positions + pos_i);
		
		// TODO: TEMP, hardcoded.
		V3 dir = { 0, 0, -1 };

		// Create MV matrix for light.
		M4 view;
		look_at(v3_mul_f(pos, -1.f), v3_mul_f(dir, -1.f), view);

		// 90 degrees will give us a face of the cube map we want.
		float fov = 90.f;
		float aspect_ratio = depth_map->width / (float)depth_map->height;

		// TODO: TEMP: Hardcoded settings

		float near_plane = 1.f;
		float far_plane = 100.f; // TODO: Defined from strength of point light? with attenuation taken into account?

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
			
			// Calculate the new model matrix from the mi's transform.
			int transform_index = j * STRIDE_MI_TRANSFORM;
			
			M4 model_matrix;
			m4_model_matrix(
				v3_read(mis_transforms + transform_index), 
				v3_read(mis_transforms + transform_index + 3), 
				v3_read(mis_transforms + transform_index + 6), 
				model_matrix
			);

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
				V4 osp0 = v3_read_to_v4(object_space_positions + index_parts_v0, 1.f);
				V4 osp1 = v3_read_to_v4(object_space_positions + index_parts_v1, 1.f);
				V4 osp2 = v3_read_to_v4(object_space_positions + index_parts_v2, 1.f);

				// TODO: For this, refactor to do model view transformation first per vertex
				//		 so we're not doing it multiple times with the indexed rendering. 
				//		
				// TODO: This should all work the same as the normal rendering really, frustum
				//		 culling and clipping etc.

				V4 vsp0, vsp1, vsp2;
				m4_mul_v4(model_view, osp0, &vsp0);
				m4_mul_v4(model_view, osp1, &vsp1);
				m4_mul_v4(model_view, osp2, &vsp2);

				V3 vsp0_v3 = v4_xyz(vsp0);
				V3 vsp1_v3 = v4_xyz(vsp1);
				V3 vsp2_v3 = v4_xyz(vsp2);

				V3 face_normal = normalised(cross(v3_sub_v3(vsp1_v3, vsp0_v3), v3_sub_v3(vsp2_v3, vsp0_v3)));

				// Only fill depth map from back faces, need the normal so doing this manually.
				if (dot(vsp0_v3, face_normal) <= 0)
				{
					// TODO: What do we write out now???
					continue;
				}

				// The perspective projection transforms the coordinates into clip space, before the perpsective divide.
				// which just converts the homogeneous coordinates to cartesian ones. 
				V4 clip0, clip1, clip2;
				m4_mul_v4(proj, vsp0, &clip0);
				m4_mul_v4(proj, vsp1, &clip1);
				m4_mul_v4(proj, vsp2, &clip2);

				// Save the light space positions for each vertex.
				const int index_lsp_parts_v0 = (models->mbs_face_position_indices[face_index] + positions_offset) * 4;
				const int index_lsp_parts_v1 = (models->mbs_face_position_indices[face_index + 1] + positions_offset) * 4;
				const int index_lsp_parts_v2 = (models->mbs_face_position_indices[face_index + 2] + positions_offset) * 4;

				// Perspective-correct intertpolation of values that have undergone perspective divide dont work?
				// TODO: Comments and understand all this a bit more.
				v4_write(renderer->buffers.light_space_positions + index_lsp_parts_v0, clip0);
				v4_write(renderer->buffers.light_space_positions + index_lsp_parts_v1, clip1);
				v4_write(renderer->buffers.light_space_positions + index_lsp_parts_v2, clip2);

				// Perform perspective divide to convert Clip Space to NDC space (-1:1 for x,y,z).
				const float inv_w0 = 1.0f / clip0.w;
				const float inv_w1 = 1.0f / clip1.w;
				const float inv_w2 = 1.0f / clip2.w;
				
				V4 ndc0 = {
					clip0.x * inv_w0,
					clip0.y * inv_w0,
					clip0.z * inv_w0,
					inv_w0
				};

				V4 ndc1 = {
					clip1.x * inv_w1,
					clip1.y * inv_w1,
					clip1.z * inv_w1,
					inv_w1
				};

				V4 ndc2 = {
					clip2.x * inv_w2,
					clip2.y * inv_w2,
					clip2.z * inv_w2,
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
				
				// Apply slope scaled depth bias to fix shadow acne and peter panning.
				
				// Don't allow the cos angle to be negative, the bias should push the shadow away from the light.
				float cos_theta = fabsf(dot(face_normal, v3_mul_f(dir, -1.f)));

				// TODO: This will have to be changed for each scene i think.
				const float constant_bias = 0.0001f;

				// Clamp the cos_theta to a value near 0 so we don't divide by 0.
				cos_theta = max(cos_theta, 0.00001f);
				
				// We want a large bias when the light dir and surface dir are perpendicular
				// because shadow acne is most common there.
				float slope_bias = constant_bias * sqrtf(1.f - cos_theta * cos_theta) / cos_theta;
				ssp0.z += slope_bias;
				ssp1.z += slope_bias;
				ssp2.z += slope_bias;

				// Draw to the depth buffer.
				draw_depth_triangle(depth_map, ssp0, ssp1, ssp2);
			}
		
			positions_offset += models->mbs_positions_counts[mb_index];
		}
	}
}
