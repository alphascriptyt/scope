#include "render.h"

#include "render_target.h"
#include "colour.h"

#include "maths/matrix4.h"
#include "maths/vector4.h"
#include "maths/vector3.h"
#include "maths/utils.h"

#include "frustum_culling.h"

#include "utils/timer.h"
#include "utils/common.h"

#include <stdio.h>

void draw_flat_bottom_triangle(RenderTarget* rt, V3 v0, V3 v1, V3 v2, V4 c0, V4 c1, V4 c2)
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

	float dwdy0 = (pv1[2] - v0[2]) * invDy;
	float dwdy1 = (pv2[2] - v0[2]) * invDy;

	V4 dcdy0;
	v4_sub_v4_out(pc1, c0, dcdy0);
	v4_mul_f(dcdy0, invDy);

	V4 dcdy1;
	v4_sub_v4_out(pc2, c0, dcdy1);
	v4_mul_f(dcdy1, invDy);

	// Do I need 
	V4 start_c = {
		c0[0],
		c0[1],
		c0[2],
		c0[3],
	};

	V4 end_c = {
		c0[0],
		c0[1],
		c0[2],
		c0[3],
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

		float wStart = v0[2] + dwdy0 * a;
		float wEnd = v0[2] + dwdy1 * a;

		V4 tempc0;
		v4_mul_f_out(dcdy0, a, tempc0);
		v4_add_v4(tempc0, start_c);

		V4 tempc1;
		v4_mul_f_out(dcdy1, a, tempc1);
		v4_add_v4(tempc1, start_c);

		int xStart = (int)(ceil(x0 - 0.5f));
		int xEnd = (int)(ceil(x1 - 0.5f));

		draw_scanline(rt, xStart, xEnd, y, wStart, wEnd, tempc0, tempc1);
	}
}

void draw_flat_top_triangle(RenderTarget* rt, V3 v0, V3 v1, V3 v2, V4 c0, V4 c1, V4 c2)
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

	float dwdy0 = (v2[2] - pv0[2]) * invDy;
	float dwdy1 = (v2[2] - pv1[2]) * invDy;

	V4 dcdy0;
	v4_sub_v4_out(c2, pc0, dcdy0);
	v4_mul_f(dcdy0, invDy);

	V4 dcdy1;
	v4_sub_v4_out(c2, pc1, dcdy1);
	v4_mul_f(dcdy1, invDy);

	V4 start_c = { 
		pc0[0],
		pc0[1],
		pc0[2],
		pc0[3],
	};

	V4 end_c = {
		pc1[0],
		pc1[1],
		pc1[2],
		pc1[3],
	};

	int yStart = (int)(ceil(pv0[1] - 0.5f));
	int yEnd = (int)(ceil(v2[1] - 0.5f));

	for (int y = yStart; y < yEnd; ++y) {
		// Must lerp for the vertex attributes to get them accurately.
		// TODO: Would be nice to find a way to step not lerp.
		float a = (y + 0.5f - pv0[1]);

		float x0 = pv0[0] + dxdy0 * a;
		float x1 = pv1[0] + dxdy1 * a;

		float wStart = pv0[2] + dwdy0 * a;
		float wEnd = pv1[2] + dwdy1 * a;

		V4 tempc0;
		v4_mul_f_out(dcdy0, a, tempc0);
		v4_add_v4(tempc0, start_c);

		V4 tempc1;
		v4_mul_f_out(dcdy1, a, tempc1);
		v4_add_v4(tempc1, end_c);

		int xStart = (int)(ceil(x0 - 0.5f));
		int xEnd = (int)(ceil(x1 - 0.5f));
		draw_scanline(rt, xStart, xEnd, y, wStart, wEnd, tempc0, tempc1);
	}
}

void draw_triangle(RenderTarget* rt, V3 v0, V3 v1, V3 v2, V4 c0, V4 c1, V4 c2)
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

	V3 pv3 = {
		pv0[0] + (pv2[0] - pv0[0]) * t,
		pv1[1],
		pv0[2] + (pv2[2] - pv0[2]) * t
	};

	// Lerp for the new colour of the vertex.
	V4 pc3;
	v4_sub_v4_out(pc2, pc0, pc3);
	v4_mul_f(pc3, t);
	v4_add_v4(pc3, pc0);

	// TODO: UVs
	// V2 tex4 = tex1 + (tex3 - tex1) * t;
	draw_flat_top_triangle(rt, pv1, pv3, pv2, pc1, pc3, pc2);
	draw_flat_bottom_triangle(rt, pv0, pv1, pv3, pc0, pc1, pc3);
}

float calculate_diffuse_factor(const V3 v, const V3 n, const V3 light_pos, const float a, const float b)
{
	// TODO: Comments, check maths etc.

	// calculate the direction of the light to the vertex
	V3 light_dir; 
	v3_sub_v3_out(light_pos, v, light_dir);

	float light_distance = size(light_dir);

	v3_mul_f(light_dir, 1.f / light_distance);

	// Calculate how much the vertex is lit
	float diffuse_factor = max(0.0f, dot(light_dir, n));

	float attenuation = 1.0f / (1.0f + (a * light_distance) + (b * light_distance * light_distance));
	float dp = diffuse_factor * attenuation;

	// TODO: What is the name for this after attentuation is applied to the 
	// diffsue factor?
	return dp;
}

void draw_line(RenderTarget* rt, float x0, float y0, float x1, float y1, const V3 colour)
{
	if (x0 > x1)
	{
		float temp = x1;
		x1 = x0;
		x0 = temp;

		temp = y1;
		y1 = y0;
		y0 = temp;
	}


	// TODO: Refactor and optimise.

	// Integer Based Bresenham's Algorithm: https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm

	int dlong = abs(x1 - x0);
	int dshort = abs(y1 - y0);
	int slong = x0 < x1 ? 1 : -1; // x direction
	int sshort = y0 < y1 ? 1 : -1; // y direction
	int x = x0;
	int y = y0;

	int y_longer = 0;
	if (dlong <= dshort) 
	{
		int temp = dshort;
		dshort = dlong;
		dlong = dshort;

		temp = sshort;
		sshort = slong;
		slong = temp;

		y_longer = 1;
	}

	int D = 2 * dshort - dlong;

	int colour_int = float_rgb_to_int(colour[0], colour[1], colour[2]);

	for (int i = 0; i <= dlong; ++i) 
	{

		if (y_longer) 
		{
			y += slong;
		}
		else {
			x += slong;
		}


		if (x > -1 && x < rt->canvas->width - 1 && y > -1 && y < rt->canvas->height - 1) 
		{
			int pos = y * rt->canvas->width + x;
			rt->canvas->pixels[pos] = colour_int;
			rt->depth_buffer[pos] = 0;
		}



		if (D >= 0) {
			if (y_longer) {
				x += sshort;
			}
			else {
				y += sshort;
			}

			D += 2 * (dshort - dlong);
		}
		else {
			D += 2 * dshort;
		}
	}

}

void clip_and_draw_triangle(RenderTarget* rt, Models* models, V3 v0, V3 v1, V3 v2, V4 c0, V4 c1, V4 c2)
{
	// TODO: Broad phase for this.


	// Clips the given triangle against the 4 screen edges.
	Plane left = {
		.point = {0, 0, 0},
		.normal = {1, 0, 0}
	};

	Plane right = {
		.point = {(float)rt->canvas->width, 0, 0},
		.normal = {-1, 0, 0}
	};

	Plane bottom = {
		.point = {0, (float)rt->canvas->height, 0},
		.normal = {0, -1, 0}
	};

	Plane top = {
		.point = { 0, 0, 0 },
		.normal = { 0, 1, 0 }
	};

	const int NUM_PLANES = 4;
	Plane planes[] = { left, right, top, bottom};

	// Flip between in/out buffers each plane.
	float* projected_clipped_faces_in = models->projected_clipped_faces;
	float* projected_clipped_faces_out = models->projected_clipped_faces_temp;
	
	// Copy the face argument into the in buffer.
	projected_clipped_faces_in[0] = v0[0];
	projected_clipped_faces_in[1] = v0[1];
	projected_clipped_faces_in[2] = v0[2];
	projected_clipped_faces_in[3] = c0[0];
	projected_clipped_faces_in[4] = c0[1];
	projected_clipped_faces_in[5] = c0[2];
	projected_clipped_faces_in[6] = c0[3];

	projected_clipped_faces_in[7] = v1[0];
	projected_clipped_faces_in[8] = v1[1];
	projected_clipped_faces_in[9] = v1[2];
	projected_clipped_faces_in[10] = c1[0];
	projected_clipped_faces_in[11] = c1[1];
	projected_clipped_faces_in[12] = c1[2];
	projected_clipped_faces_in[13] = c1[3];

	projected_clipped_faces_in[14] = v2[0];
	projected_clipped_faces_in[15] = v2[1];
	projected_clipped_faces_in[16] = v2[2];
	projected_clipped_faces_in[17] = c2[0];
	projected_clipped_faces_in[18] = c2[1];
	projected_clipped_faces_in[19] = c2[2];
	projected_clipped_faces_in[20] = c2[3];

	// Store the number of triangles after clipping for a plane.
	int num_faces_to_process = 1;
	int visible_faces_count = 0;

	for (int index_plane = 0; index_plane < NUM_PLANES; ++index_plane)
	{
		// Store the index for where to write to the out buffer.
		int index_out = 0;

		// Store how many faces are visible after clipping against this plane.
		int temp_num_faces_to_process = 0;

		// For faces to process.
		for (int j = 0; j < num_faces_to_process; ++j)
		{
			const Plane* plane = &planes[index_plane];

			int index_face = j * STRIDE_PROJECTED_FACE;

			int num_inside_points = 0;
			int num_outside_points = 0;

			int inside_points_indices[3] = { 0 };
			int outside_points_indices[3] = { 0 };

			V3 v0 = {
				projected_clipped_faces_in[index_face],
				projected_clipped_faces_in[index_face + 1],
				0
			};

			V3 v1 = {
				projected_clipped_faces_in[index_face + 7],
				projected_clipped_faces_in[index_face + 8],
				0
			};

			V3 v2 = {
				projected_clipped_faces_in[index_face + 14],
				projected_clipped_faces_in[index_face + 15],
				0
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
				inside_points_indices[num_inside_points++] = index_face + 7; // index_v1
			}
			else
			{
				outside_points_indices[num_outside_points++] = index_face + 7; // index_v1
			}
			if (d2 >= 0)
			{
				inside_points_indices[num_inside_points++] = index_face + 14; // index_v2
			}
			else
			{
				outside_points_indices[num_outside_points++] = index_face + 14; // index_v2
			}

			// The whole triangle is inside the plane.
			if (num_inside_points == 3)
			{
				int index_face = j * STRIDE_PROJECTED_FACE;

				// Simply copy the entire face. TODO: Could unroll. Not sure if it is beneficial, should test this,
				// in another project.
				for (int k = index_face; k < index_face + STRIDE_PROJECTED_FACE; ++k)
				{
					projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[k];
				}

				++temp_num_faces_to_process;
			}		
			else if (num_inside_points == 1 && num_outside_points == 2)
			{
				// Form a new triangle with the plane edge.
				// Unpack the points.
				int index_ip0 = inside_points_indices[0];
				int index_op0 = outside_points_indices[0];
				int index_op1 = outside_points_indices[1];

				const V3 ip0 = {
					projected_clipped_faces_in[index_ip0],
					projected_clipped_faces_in[index_ip0 + 1],
					0
				};

				const V3 op0 = {
					projected_clipped_faces_in[index_op0],
					projected_clipped_faces_in[index_op0 + 1],
					0
				};

				const V3 op1 = {
					projected_clipped_faces_in[index_op1],
					projected_clipped_faces_in[index_op1 + 1],
					0
				};

				// Lerp for the new points.
				V3 p0;
				float t = line_intersect_plane(ip0, op0, plane, p0);

				// Lerp for the attributes.
				float w0 = lerp(projected_clipped_faces_in[index_ip0 + 2], projected_clipped_faces_in[index_op0 + 2], t);
				float r0 = lerp(projected_clipped_faces_in[index_ip0 + 3], projected_clipped_faces_in[index_op0 + 3], t);
				float g0 = lerp(projected_clipped_faces_in[index_ip0 + 4], projected_clipped_faces_in[index_op0 + 4], t);
				float b0 = lerp(projected_clipped_faces_in[index_ip0 + 5], projected_clipped_faces_in[index_op0 + 5], t);
				float a0 = lerp(projected_clipped_faces_in[index_ip0 + 6], projected_clipped_faces_in[index_op0 + 6], t);

				V3 p1;
				t = line_intersect_plane(ip0, op1, plane, p1);

				// Lerp for the attributes.
				float w1 = lerp(projected_clipped_faces_in[index_ip0 + 2], projected_clipped_faces_in[index_op1 + 2], t);
				float r1 = lerp(projected_clipped_faces_in[index_ip0 + 3], projected_clipped_faces_in[index_op1 + 3], t);
				float g1 = lerp(projected_clipped_faces_in[index_ip0 + 4], projected_clipped_faces_in[index_op1 + 4], t);
				float b1 = lerp(projected_clipped_faces_in[index_ip0 + 5], projected_clipped_faces_in[index_op1 + 5], t);
				float a1 = lerp(projected_clipped_faces_in[index_ip0 + 6], projected_clipped_faces_in[index_op1 + 6], t);

				// Copy the attributes into the new face.
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[index_ip0];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip0];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip0];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip0];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip0];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip0];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip0];

				projected_clipped_faces_out[index_out++] = p0[0];
				projected_clipped_faces_out[index_out++] = p0[1];
				projected_clipped_faces_out[index_out++] = w0;
				projected_clipped_faces_out[index_out++] = r0;
				projected_clipped_faces_out[index_out++] = g0;
				projected_clipped_faces_out[index_out++] = b0;
				projected_clipped_faces_out[index_out++] = a0;

				projected_clipped_faces_out[index_out++] = p1[0];
				projected_clipped_faces_out[index_out++] = p1[1];
				projected_clipped_faces_out[index_out++] = w1;
				projected_clipped_faces_out[index_out++] = r1;
				projected_clipped_faces_out[index_out++] = g1;
				projected_clipped_faces_out[index_out++] = b1;
				projected_clipped_faces_out[index_out++] = a1;
				
				++temp_num_faces_to_process;
			}
			else if (num_inside_points == 2 && num_outside_points == 1)
			{
				// Form two new triangles with the plane edge.
				// Unpack the points.
				int index_ip0 = inside_points_indices[0];
				int index_ip1 = inside_points_indices[1];
				int index_op0 = outside_points_indices[0];

				const V3 ip0 = {
					projected_clipped_faces_in[index_ip0],
					projected_clipped_faces_in[index_ip0 + 1],
					0
				};

				const V3 ip1 = {
					projected_clipped_faces_in[index_ip1],
					projected_clipped_faces_in[index_ip1 + 1],
					0
				};

				const V3 op0 = {
					projected_clipped_faces_in[index_op0],
					projected_clipped_faces_in[index_op0 + 1],
					0
				};

				// Lerp for the new points.
				V3 p0;
				float t = line_intersect_plane(ip0, op0, plane, p0);

				// Lerp for the attributes.
				float w0 = lerp(projected_clipped_faces_in[index_ip0 + 2], projected_clipped_faces_in[index_op0 + 2], t);
				float r0 = lerp(projected_clipped_faces_in[index_ip0 + 3], projected_clipped_faces_in[index_op0 + 3], t);
				float g0 = lerp(projected_clipped_faces_in[index_ip0 + 4], projected_clipped_faces_in[index_op0 + 4], t);
				float b0 = lerp(projected_clipped_faces_in[index_ip0 + 5], projected_clipped_faces_in[index_op0 + 5], t);
				float a0 = lerp(projected_clipped_faces_in[index_ip0 + 6], projected_clipped_faces_in[index_op0 + 6], t);

				// Copy the attributes into the new face.
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[index_ip0];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip0];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip0];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip0];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip0];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip0];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip0];

				int index_ip1_copy = index_ip1; // We need this for the next face also.

				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[index_ip1];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip1];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip1];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip1];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip1];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip1];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip1];

				projected_clipped_faces_out[index_out++] = p0[0];
				projected_clipped_faces_out[index_out++] = p0[1];
				projected_clipped_faces_out[index_out++] = w0;
				projected_clipped_faces_out[index_out++] = r0;
				projected_clipped_faces_out[index_out++] = g0;
				projected_clipped_faces_out[index_out++] = b0;
				projected_clipped_faces_out[index_out++] = a0;
				
				++temp_num_faces_to_process;

				V3 p1;
				t = line_intersect_plane(ip1, op0, plane, p1);

				// Lerp for the attributes.
				float w1 = lerp(projected_clipped_faces_in[index_ip1_copy + 2], projected_clipped_faces_in[index_op0 + 2], t);
				float r1 = lerp(projected_clipped_faces_in[index_ip1_copy + 3], projected_clipped_faces_in[index_op0 + 3], t);
				float g1 = lerp(projected_clipped_faces_in[index_ip1_copy + 4], projected_clipped_faces_in[index_op0 + 4], t);
				float b1 = lerp(projected_clipped_faces_in[index_ip1_copy + 5], projected_clipped_faces_in[index_op0 + 5], t);
				float a1 = lerp(projected_clipped_faces_in[index_ip1_copy + 6], projected_clipped_faces_in[index_op0 + 6], t);

				// Copy the attributes into the new face.
				projected_clipped_faces_out[index_out++] = p0[0];
				projected_clipped_faces_out[index_out++] = p0[1];
				projected_clipped_faces_out[index_out++] = w0;
				projected_clipped_faces_out[index_out++] = r0;
				projected_clipped_faces_out[index_out++] = g0;
				projected_clipped_faces_out[index_out++] = b0;
				projected_clipped_faces_out[index_out++] = a0;
				
				projected_clipped_faces_out[index_out++] = p1[0];
				projected_clipped_faces_out[index_out++] = p1[1];
				projected_clipped_faces_out[index_out++] = w1;
				projected_clipped_faces_out[index_out++] = r1;
				projected_clipped_faces_out[index_out++] = g1;
				projected_clipped_faces_out[index_out++] = b1;
				projected_clipped_faces_out[index_out++] = a1;
				
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[index_ip1_copy];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip1_copy];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip1_copy];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip1_copy];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip1_copy];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip1_copy];
				projected_clipped_faces_out[index_out++] = projected_clipped_faces_in[++index_ip1_copy];

				++temp_num_faces_to_process;
			}
		}

		num_faces_to_process = temp_num_faces_to_process;

		// Swap the buffers so that for the next iteration, we read from
		// the previously clipped faces.
		float* temp = projected_clipped_faces_in;
		projected_clipped_faces_in = projected_clipped_faces_out;
		projected_clipped_faces_out = temp;
	}

	// All faces have been clipped, so draw them.
	visible_faces_count = num_faces_to_process;
	projected_clipped_faces_out = models->projected_clipped_faces;

	for (int i = 0; i < visible_faces_count; ++i)
	{
		int index = i * STRIDE_PROJECTED_FACE;

		const V3 p0 = {
			projected_clipped_faces_out[index],
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index]
		};

		const V4 colour0 = {
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index]
		};

		const V3 p1 = {
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index]
		};

		const V4 colour1 = {
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index]
		};

		const V3 p2 = {
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index]
		};

		const V4 colour2 = {
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index],
			projected_clipped_faces_out[++index]
		};

		draw_triangle(rt, p0, p1, p2, colour0, colour1, colour2);
	}
}

void draw_scanline(RenderTarget* rt, const int x0, const int x1, const int y, const float w0, const float w1, const V4 c0, const V4 c1)
{
	// TODO: Refactor function args.

	// Ignore if the line is offscreen.
	if (y > rt->canvas->height - 1 || y < 0 ||
		x0 > rt->canvas->width - 1 || x1 < 0)
	{
		return;
	}

	// Precalculate deltas.
	float invDx = 1.f / (x1 - x0);

	//Colour colourStep = (colour1 - colour0) * invDx;
	float wStep = (w1 - w0) * invDx;

	// Offset x by the given y.
	int row_offset = rt->canvas->width * y;

	int start_x = x0 + row_offset;
	int end_x = x1 + row_offset;

	// TODO: Converting to int every frame ruins fps.
	//colourStep = (Colour::Red() - Colour::Lime()) * invDx;

	V4 c_step;
	v4_sub_v4_out(c1, c0, c_step);
	v4_mul_f(c_step, invDx);

	V4 c = {
		c0[0],
		c0[1],
		c0[2],
		c0[3],
	};

	// Render the scanline
	unsigned int* pixels = rt->canvas->pixels + start_x;
	unsigned int i = end_x - start_x;
	
	float* depth_buffer = rt->depth_buffer + start_x;
	V3 white = { 1,1,1 };

	float w = w0;
	
	while (i)
	{
		// Get the actual depth value.
		float z = 1.0f / w;

		// Depth test, only draw closer values. -1 (near plane) to 1 (far plane).
		if (*depth_buffer > z)
		{
			float r = (c[0] * z);
			float g = (c[1] * z);
			float b = (c[2] * z);

			*pixels = float_rgb_to_int(r, g ,b);
			*depth_buffer = z;
		}
		
		--i;
		++pixels;
		++depth_buffer;
		w += wStep;
		v4_add_v4(c, c_step);
	}
}

void project(const Canvas* canvas, const M4 projection_matrix, const V4 v, V3 o)
{
	V4 v_projected;
	m4_mul_v4(projection_matrix, v, v_projected);

	// Precalculate the perspective divide.
	// The depth is between 0 - 1.
	float invW = 1.0f / v_projected[3];

	// Perform perspective divide to bring to NDC space.
	// NDC space is -1 to 1 in all axis.
	v_projected[0] *= invW;
	v_projected[1] *= invW;
	v_projected[2] *= invW;

	// Convert to screen space.
	o[0] = (v_projected[0] + 1) * 0.5f * canvas->width;
	o[1] = (-v_projected[1] + 1) * 0.5f * canvas->height;

	// TODO: Opengl perspective projection matrix NDC z is -1 to 1,
	//		 do we want it 0 to 1? Not sure.
	o[2] = v_projected[2]; // Z/W
}

void model_to_world_space(Models* models)
{
	// TODO: Rename some vars.

	// Locally store to avoid dereferencing the pointers constantly.
	// Not sure if this is a speedup or not. TODO: Time at some point.
	const int models_count = models->mis_count;
	const int* mesh_positions_counts = models->mbs_positions_counts;
	const int mis_base_ids = models->mis_base_ids;
	const float* mesh_transforms = models->mis_transforms;
	const float* object_space_positions = models->mbs_object_space_positions;

	int* transforms_updated_flags = models->mis_transforms_updated_flags;
	float* world_space_positions = models->world_space_positions;
	float* bounding_spheres = models->mis_bounding_spheres;

	//int position_offset = 0;
	//int normal_offset = 0;
	int index_world_space_position_out = 0;
	int index_world_space_normal_out = 0;

	for (int i = 0; i < models_count; ++i)
	{
		// Only update if the mesh's transform has changed.
		if (transforms_updated_flags[i] == 0)
			continue;

		const int mb_index = models->mis_base_ids[i];
		const int mb_positions_offset = models->mbs_positions_offsets[mb_index];
		const int mb_normals_offset = models->mbs_normals_offsets[mb_index];

		// Unpack the mesh's transform and make a model matrix out of it.
		// Make a normal matrix as well.
		int index_mesh_transform = i * STRIDE_MI_TRANSFORM;

		const V3 position = {
			mesh_transforms[index_mesh_transform],
			mesh_transforms[++index_mesh_transform],
			mesh_transforms[++index_mesh_transform]
		};

		const V3 direction = {
			mesh_transforms[++index_mesh_transform],
			mesh_transforms[++index_mesh_transform],
			mesh_transforms[++index_mesh_transform]
		};

		const V3 scale = {
			mesh_transforms[++index_mesh_transform],
			mesh_transforms[++index_mesh_transform],
			mesh_transforms[++index_mesh_transform]
		};

		M4 model_matrix;
		make_model_m4(position, direction, scale, model_matrix);


		// Define the model's normal matrix.
		// Essentially no translation, keep the rotation, and inverse scale.
		M4 model_normal_matrix;
		M4 rotation_m4;
		make_rotation_m4(direction[0], direction[1], direction[2], rotation_m4);

		M4 scale_m4;
		identity(scale_m4);
		scale_m4[0] = 1.f / scale[0];
		scale_m4[5] = 1.f / scale[1];
		scale_m4[10] = 1.f / scale[2];

		m4_mul_m4(rotation_m4, scale_m4, model_normal_matrix);

		V3 center = { 0 };

		// We want to read each position in the model base.
		// How do we get that index.

		int wsp_start_index = index_world_space_position_out;

		// Convert object space to world space positions whilst also calculating the new bounding sphere.
		for (int j = 0; j < mesh_positions_counts[mb_index]; ++j)
		{			
			int index_object_space_position = (j + mb_positions_offset) * STRIDE_POSITION;

			// TODO: A function like inline read_v4(float* out, float* in, int offset);
			// V4 object_space_position;
			// read_v4(object_space_position, object_space_positions, index_object_space_position)
			V4 object_space_position = {
				object_space_positions[index_object_space_position],
				object_space_positions[index_object_space_position + 1],
				object_space_positions[index_object_space_position + 2],
				1
			};

			V4 world_space_position;
			m4_mul_v4(model_matrix, object_space_position, world_space_position);

			// inline write_v4()?
			world_space_positions[index_world_space_position_out++] = world_space_position[0];
			world_space_positions[index_world_space_position_out++] = world_space_position[1];
			world_space_positions[index_world_space_position_out++] = world_space_position[2];

			// TODO: Did calculating this using the indices not give the correct results??
			v3_add_v3(center, world_space_position);
		}

		// TODO: Make a function to compute the bounding sphere?.
		v3_mul_f(center, 1.f / mesh_positions_counts[mb_index]);

		float radius_squared = 0;
		for (int j = wsp_start_index; j < index_world_space_position_out; j += STRIDE_POSITION)
		{
			/// TOODODODODODO: TODO: All this logic is so scuffed with the new mi and mb.
			// Need to think about how the iteration should actually work so i can keep it 
			// consistent and validate the logic in my head first because this is just so 
			// dumb and not working. It should work, think through....
			//int index_world_space_position = (j + mb_positions_offset) * STRIDE_POSITION;

			V4 v = {
				world_space_positions[wsp_start_index],
				world_space_positions[wsp_start_index + 1],
				world_space_positions[wsp_start_index + 2],
				1
			};

			// Calculate the length of the line between the center and the vertex.
			v3_sub_v3(v, center);
			radius_squared = max(size_squared(v), radius_squared);
		}

		// Store the bounding sphere.
		int index_bs = i * STRIDE_SPHERE;
		bounding_spheres[index_bs] = center[0];
		bounding_spheres[++index_bs] = center[1];
		bounding_spheres[++index_bs] = center[2];
		bounding_spheres[++index_bs] = sqrtf(radius_squared);

		// Calculate the world space normals.
		const int* mesh_normals_counts = models->mbs_normals_counts;
		const float* object_space_normals = models->mbs_object_space_normals;

		float* world_space_normals = models->world_space_normals;

		for (int j = 0; j < mesh_normals_counts[mb_index]; ++j)
		{
			int index_object_space_normal = (j + mb_normals_offset) * STRIDE_NORMAL;

			V4 object_space_normal = {
				object_space_normals[index_object_space_normal],
				object_space_normals[index_object_space_normal + 1],
				object_space_normals[index_object_space_normal + 2],
				0 // We don't want any translation, although the normal matrix has none anyways.
			};

			V4 world_space_dir;
			m4_mul_v4(model_normal_matrix, object_space_normal, world_space_dir);

			V3 world_space_normal = {
				world_space_dir[0],
				world_space_dir[1],
				world_space_dir[2] 
			};
			normalise(world_space_normal);

			// inline write_v4()?
			world_space_normals[index_world_space_normal_out++] = world_space_normal[0];
			world_space_normals[index_world_space_normal_out++] = world_space_normal[1];
			world_space_normals[index_world_space_normal_out++] = world_space_normal[2];
		}

		// Reset the flag to show we've updated the world space positions
		// with respect to the most recent transform.
		transforms_updated_flags[i] = 0;
	}
}

void world_to_view_space(Models* models, PointLights* point_lights, const M4 view_matrix)
{
	// Transform the world space mesh positions.
	const float* world_space_positions = models->world_space_positions;
	float* view_space_positions = models->view_space_positions;

	// For each world space position, convert it to view space.
	int num_position_components = models->mis_total_positions * STRIDE_POSITION;
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

	// Transform the world space light positions.
	world_space_positions = point_lights->world_space_positions;
	view_space_positions = point_lights->view_space_positions;

	// For each world space position, convert it to view space.
	num_position_components = point_lights->count * STRIDE_POSITION;
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


	// For each world space normal, convert it to view space.
	int num_normal_components = models->mis_total_normals * STRIDE_NORMAL;
	const float* world_space_normals = models->world_space_normals;
	float* view_space_normals = models->view_space_normals;

	// View matrix has no scale, so just get the top left 3x3 containing,
	// the rotation.
	M4 view_normal_matrix;
	m4_copy_m3(view_matrix, view_normal_matrix);

	//printf("vm\n%s\n\n\n", m4_to_str(view_matrix));
	//printf("vnm\n%s", m4_to_str(view_normal_matrix));

	for (int i = 0; i < num_normal_components; i += STRIDE_NORMAL)
	{
		V4 dir = {
			world_space_normals[i],
			world_space_normals[i + 1],
			world_space_normals[i + 2],
			0
		};

		V4 dir_view_space;
		m4_mul_v4(view_normal_matrix, dir, dir_view_space);

		V3 n_view_space = {
			dir_view_space[0],
			dir_view_space[1],
			dir_view_space[2]
		};

		// TODO: Test, can we pass a v4 as a v3 to ignore w ?
		normalise(n_view_space);

		// There is no need to save the w component as it is always 1 until 
		// after projection.
		view_space_normals[i]	  = n_view_space[0];
		view_space_normals[i + 1] = n_view_space[1];
		view_space_normals[i + 2] = n_view_space[2];
	}
}

void render(RenderTarget* rt, const RenderSettings* settings, Models* models, PointLights* point_lights, const M4 view_matrix)
{
	// TODO: Can these transformations ever be combined so we make a model view matrix, then we are doing less matrix multiplications overall?
	//		 Not sure because we need the world coordinates anyways for physics.

	// TODO: Calculate normal matrix by inverting scale and applying rotation. No translation.

	// TODO: Need to calculate view matrix and model matrix by passing the args in here, then I can just straight take the 
	// Transform world space positions to view space.
	model_to_world_space(models);
	 
	// Transforms lights as well as models.
	world_to_view_space(models, point_lights, view_matrix);

	const int* face_position_indices = models->mbs_face_position_indices;
	const int* face_normal_indices = models->mbs_face_normal_indices;
	const int* face_uvs_indices = models->mbs_face_uvs_indices;

	float* uvs = models->mbs_uvs;

	float* view_space_positions = models->view_space_positions;
	float* view_space_normals = models->view_space_normals;

	// Perform backface culling.
	int face_offset = 0;
	int front_face_offset = 0;

	float* front_faces = models->front_faces;

	int positions_offset = 0;
	int normals_offset = 0;
	int uvs_offset = 0;

	for (int i = 0; i < models->mis_count; ++i)
	{
		const int mb_index = models->mis_base_ids[i];
		const int mb_faces_offset = models->mbs_faces_offsets[mb_index];
		const int mb_uvs_offset = models->mbs_uvs_offsets[mb_index];

		int front_face_count = 0;

		//const int mesh_faces_end = face_offset + models->mbs_faces_counts[mb_index];
		
		//for (int j = face_offset; j < mesh_faces_end; ++j)
		for (int j = 0; j < models->mbs_faces_counts[mb_index]; ++j)
		{
			const int face_index = (mb_faces_offset + j) * STRIDE_FACE_VERTICES;

			// Get the indices to the first component of each vertex position.
			const int index_v0 = face_position_indices[face_index] + positions_offset;
			const int index_v1 = face_position_indices[face_index + 1] + positions_offset;
			const int index_v2 = face_position_indices[face_index + 2] + positions_offset;

			int index_parts_v0 = index_v0 * STRIDE_POSITION;
			int index_parts_v1 = index_v1 * STRIDE_POSITION;
			int index_parts_v2 = index_v2 * STRIDE_POSITION;

			// Get the vertices from the face indices.
			V3 v0 = {
				view_space_positions[index_parts_v0],
				view_space_positions[index_parts_v0 + 1],
				view_space_positions[index_parts_v0 + 2]
			};

			V3 v1 = {
				view_space_positions[index_parts_v1],
				view_space_positions[index_parts_v1 + 1],
				view_space_positions[index_parts_v1 + 2]
			};

			V3 v2 = {
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

				const int index_uv0 = face_uvs_indices[face_index] ;
				const int index_uv1 = face_uvs_indices[face_index + 1] ;
				const int index_uv2 = face_uvs_indices[face_index + 2] ;

				int index_parts_uv0 = index_uv0 * STRIDE_UV;
				int index_parts_uv1 = index_uv1 * STRIDE_UV;
				int index_parts_uv2 = index_uv2 * STRIDE_UV;


				// Copy all the face vertex data.
				// We copy the attributes over here as well because when clipping we need the data
				// all together for lerping.
				front_faces[front_face_offset++] = v0[0];
				front_faces[front_face_offset++] = v0[1];
				front_faces[front_face_offset++] = v0[2];

				front_faces[front_face_offset++] = uvs[index_parts_uv0];
				front_faces[front_face_offset++] = uvs[index_parts_uv0 + 1];

				front_faces[front_face_offset++] = view_space_normals[index_parts_n0];
				front_faces[front_face_offset++] = view_space_normals[index_parts_n0 + 1];
				front_faces[front_face_offset++] = view_space_normals[index_parts_n0 + 2];

				//front_faces[front_face_offset++] = face_attributes[index_face_attributes++];
				//front_faces[front_face_offset++] = face_attributes[index_face_attributes++];
				//front_faces[front_face_offset++] = face_attributes[index_face_attributes++];
				//front_faces[front_face_offset++] = face_attributes[index_face_attributes++];

				// TEMP: HARDCODE COLOURS
				front_faces[front_face_offset++] = 1;
				front_faces[front_face_offset++] = 0;
				front_faces[front_face_offset++] = 0;
				front_faces[front_face_offset++] = 1;


				front_faces[front_face_offset++] = v1[0];
				front_faces[front_face_offset++] = v1[1];
				front_faces[front_face_offset++] = v1[2];

				front_faces[front_face_offset++] = uvs[index_parts_uv1];
				front_faces[front_face_offset++] = uvs[index_parts_uv1 + 1];

				front_faces[front_face_offset++] = view_space_normals[index_parts_n1];
				front_faces[front_face_offset++] = view_space_normals[index_parts_n1 + 1];
				front_faces[front_face_offset++] = view_space_normals[index_parts_n1 + 2];

				front_faces[front_face_offset++] = 1;
				front_faces[front_face_offset++] = 0;
				front_faces[front_face_offset++] = 0;
				front_faces[front_face_offset++] = 1;

				front_faces[front_face_offset++] = v2[0];
				front_faces[front_face_offset++] = v2[1];
				front_faces[front_face_offset++] = v2[2];

				front_faces[front_face_offset++] = uvs[index_parts_uv2];
				front_faces[front_face_offset++] = uvs[index_parts_uv2 + 1];

				front_faces[front_face_offset++] = view_space_normals[index_parts_n2];
				front_faces[front_face_offset++] = view_space_normals[index_parts_n2 + 1];
				front_faces[front_face_offset++] = view_space_normals[index_parts_n2 + 2];

				front_faces[front_face_offset++] = 1;
				front_faces[front_face_offset++] = 0;
				front_faces[front_face_offset++] = 0;
				front_faces[front_face_offset++] = 1;

				++front_face_count;
			}
		}

		// Update the number of front faces for the current mesh.
		// This is needed for frustum culling.
		models->front_faces_counts[i] = front_face_count;

		// Update the offsets for the next mesh.
		face_offset += models->mbs_faces_counts[mb_index];
		positions_offset += models->mbs_positions_counts[mb_index];
		normals_offset += models->mbs_normals_counts[mb_index];
		uvs_offset += models->mbs_uvs_counts[mb_index];
	}

	
	//TODO: 
	//Going forward I am going to do broad phase frustum culling,
	//then if the mesh passes, we must perform the lighting, this
	//way we won't get inconsistent lighting with clipped vertices
	//being closer to the light.
	//After lighting is performed, we do the actual clipping,
	//this way the colours are lerped so it stays consistent.


	// Frustum culling
	float* clipped_faces = models->clipped_faces;
	int* mesh_clipped_faces_counts = models->clipped_faces_counts;

	float* bounding_spheres = models->mis_bounding_spheres;

	ViewFrustum view_frustum; 
	create_clipping_view_frustum(settings->near_plane, settings->fov, 
		rt->canvas->width / (float)(rt->canvas->height), &view_frustum);

	int clipped_faces_index = 0; // Store the index to write the clipped faces out to.
	
	face_offset = 0;
	positions_offset = 0;
	for (int i = 0; i < models->mis_count; ++i)
	{
		int visible_faces_count = 0;

		// Perform board phase bounding sphere check.
		int index_bounding_sphere = i * STRIDE_SPHERE;

		// We must convert the center to view space
		V4 center =
		{
			bounding_spheres[index_bounding_sphere++],
			bounding_spheres[index_bounding_sphere++],
			bounding_spheres[index_bounding_sphere++],
			1
		};

		V4 view_space_center;
		m4_mul_v4(view_matrix, center, view_space_center);

		// Radius stays the same as the view matrix does not scale.
		float radius = bounding_spheres[index_bounding_sphere];

		// Store what planes need clipping against.
		int clip_against_plane[MAX_FRUSTUM_PLANES] = { 0 };
		int num_planes_to_clip_against = 0;

		// Broad phase test.
		int mesh_visible = 1;
		for (int j = 0; j < view_frustum.num_planes; ++j)
		{
			float dist = signed_distance(&view_frustum.planes[j], view_space_center);
			if (dist < -radius)
			{
				// Completely outside the plane, therefore, no need to check against the others.
				mesh_visible = 0;
				break;
			}
			else if (dist < radius)
			{
				// The mesh intersects with this plane, so the mesh could be partially visible.
				mesh_visible = 1;
				 
				// Mark that we need to clip against this plane.
				clip_against_plane[j] = 1;
				++num_planes_to_clip_against;
			}
		}
		
		if (0 == mesh_visible)
		{
			continue;
		}

		// At this point we know the mesh is partially visible at least.
		// Apply lighting here so that the if a vertex is clipped closer
		// to the light, the lighing doesn't change.

		// TODO: COMMENTS.

		// For each face.
		for (int j = face_offset; j < face_offset + models->front_faces_counts[i]; ++j)
		{
			int index_face = j * STRIDE_ENTIRE_FACE;

			// For each vertex apply lighting directly to the colour.

			// 12 attributes per vertex. TODO: STRIDE for this?
			for (int k = index_face; k < index_face + STRIDE_ENTIRE_FACE; k += 12)
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

				const V4 start = {
					front_faces[k],
					front_faces[k + 1],
					front_faces[k + 2],
					1
				};

				V3 end = {
					front_faces[k],
					front_faces[k + 1],
					front_faces[k + 2],
				};

				
				V3 dir;
				v3_mul_f_out(normal, 3.f, dir);
				v3_add_v3(end, dir);


				V4 end4 = {
					end[0],
					end[1],
					end[2],
					1
				};

				V3 s, e;



				project(rt->canvas, settings->projection_matrix, start, s);
				project(rt->canvas, settings->projection_matrix, end, e);

				V3 col = { 1,0,1 };

				// TODO: THIs is awful haha
				//draw_line(rt, s[0], s[1], e[0], e[1], col);

				// TODO: Normals must also be rotated. This should be done in 
				// the view space step. I believe there was an actual matrix
				// for this.
				// TODO: NORMALS NEED TO HAVE MODEL MATRIX APPLIED, AND THEN
				// VIEW MATRIX AS WELL. WILL NEED TO BE DONE BOTH STEPS.
				
				// Only need RGB, only need A for combining with texture???

				// This is how much light it absorbs?
				V3 colour = {
					front_faces[k + 8],
					front_faces[k + 9],
					front_faces[k + 10],
				};

				// TODO: THIS IS ALL TEMPORARY FOR ONE LIGHT NO SPECIAL MATHS.

				// TODO: I'm pretty sure the per pixel interpolation is broken or maybe that's just the lighting.

				// For each light
				for (int i_light = 0; i_light < point_lights->count; ++i_light)
				{
					int i_light_pos = i_light * 3;
					int i_light_attr = i_light * STRIDE_POINT_LIGHT_ATTRIBUTES;

					const V3 light_pos =
					{
						point_lights->view_space_positions[i_light_pos],
						point_lights->view_space_positions[i_light_pos + 1],
						point_lights->view_space_positions[i_light_pos + 2]
					};

					const V3 light_colour =
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

					// TODO: ALL TEMPPPP
					if (df > 1) df = 1;
					if (df < 0) df = 0;

					front_faces[k + 8] = df;
					front_faces[k + 9] = df;
					front_faces[k + 10] = df;


				}



			}

			
		}








		if (1 == mesh_visible && 0 == num_planes_to_clip_against)
		{
			// Just copy the vertices over.
			for (int j = face_offset; j < face_offset + models->front_faces_counts[i]; ++j)
			{
				int index_face = j * STRIDE_ENTIRE_FACE;
					
				// Simply copy the entire face. TODO: Could unroll. Not sure if it is beneficial, should test this,
				// in another project.
				for (int k = index_face; k < index_face + STRIDE_ENTIRE_FACE; ++k)
				{
					clipped_faces[clipped_faces_index++] = front_faces[k];
				}

				++visible_faces_count;
			}
		}
		else
		{
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

			for (int index_plane = 0; index_plane < view_frustum.num_planes; ++index_plane)
			{
				// Only clip against the plane if the broad phase flagged it. 
				if (clip_against_plane[index_plane] == 0) 
					continue;

				const Plane* plane = &view_frustum.planes[index_plane];

				// Reset the index to write out to.
				index_out = 0;

				// Store how many triangles were wrote to the out buffer.				
				int temp_visible_faces_count = 0;

				// After the first plane we want to read from the in buffer.
				if (num_planes_clipped_against == 1)
				{
					// Initially we used the in front faces buffer,
					// after the first iteration we have wrote to the out
					// buffer, so that can now be our in buffer.
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
					index_out = clipped_faces_index;
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
						temp_clipped_faces_in[index_face + 12],
						temp_clipped_faces_in[index_face + 13],
						temp_clipped_faces_in[index_face + 14]
					};

					const V3 v2 = {
						temp_clipped_faces_in[index_face + 24],
						temp_clipped_faces_in[index_face + 25],
						temp_clipped_faces_in[index_face + 26]
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
						inside_points_indices[num_inside_points++] = index_face + 12; // index_v1
					}
					else
					{
						outside_points_indices[num_outside_points++] = index_face + 12; // index_v1
					}
					if (d2 >= 0)
					{
						inside_points_indices[num_inside_points++] = index_face + 24; // index_v2
					}
					else
					{
						outside_points_indices[num_outside_points++] = index_face + 24; // index_v2
					}

					if (num_inside_points == 3)
					{
						// The whole triangle is inside the plane.
						int index_face = j * STRIDE_ENTIRE_FACE;

						// Simply copy the entire face. TODO: Could unroll. Not sure if it is beneficial, should test this,
						// in another project.
						for (int k = index_face; k < index_face + STRIDE_ENTIRE_FACE; ++k)
						{
							temp_clipped_faces_out[index_out++] = temp_clipped_faces_in[k];
						}

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
						float u0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_U], temp_clipped_faces_in[index_op0 + INDEX_U], t);
						float v0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_V], temp_clipped_faces_in[index_op0 + INDEX_V], t);
						float nx0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NX], temp_clipped_faces_in[index_op0 + INDEX_NX], t);
						float ny0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NY], temp_clipped_faces_in[index_op0 + INDEX_NY], t);
						float nz0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NZ], temp_clipped_faces_in[index_op0 + INDEX_NZ], t);
						float r0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_R], temp_clipped_faces_in[index_op0 + INDEX_R], t);
						float g0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_G], temp_clipped_faces_in[index_op0 + INDEX_G], t);
						float b0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_B], temp_clipped_faces_in[index_op0 + INDEX_B], t);
						float a0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_A], temp_clipped_faces_in[index_op0 + INDEX_A], t);

						V3 p1;
						t = line_intersect_plane(ip0, op1, plane, p1);

						// Lerp for the attributes.
						float u1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_U], temp_clipped_faces_in[index_op1 + INDEX_U], t);
						float v1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_V], temp_clipped_faces_in[index_op1 + INDEX_V], t);
						float nx1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NX], temp_clipped_faces_in[index_op1 + INDEX_NX], t);
						float ny1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NY], temp_clipped_faces_in[index_op1 + INDEX_NY], t);
						float nz1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NZ], temp_clipped_faces_in[index_op1 + INDEX_NZ], t);
						float r1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_R], temp_clipped_faces_in[index_op1 + INDEX_R], t);
						float g1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_G], temp_clipped_faces_in[index_op1 + INDEX_G], t);
						float b1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_B], temp_clipped_faces_in[index_op1 + INDEX_B], t);
						float a1 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_A], temp_clipped_faces_in[index_op1 + INDEX_A], t);

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
						temp_clipped_faces_out[index_out++] = a0;

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
						temp_clipped_faces_out[index_out++] = a1;

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
						float u0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_U], temp_clipped_faces_in[index_op0 + INDEX_U], t);
						float v0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_V], temp_clipped_faces_in[index_op0 + INDEX_V], t);
						float nx0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NX], temp_clipped_faces_in[index_op0 + INDEX_NX], t);
						float ny0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NY], temp_clipped_faces_in[index_op0 + INDEX_NY], t);
						float nz0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_NZ], temp_clipped_faces_in[index_op0 + INDEX_NZ], t);
						float r0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_R], temp_clipped_faces_in[index_op0 + INDEX_R], t);
						float g0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_G], temp_clipped_faces_in[index_op0 + INDEX_G], t);
						float b0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_B], temp_clipped_faces_in[index_op0 + INDEX_B], t);
						float a0 = lerp(temp_clipped_faces_in[index_ip0 + INDEX_A], temp_clipped_faces_in[index_op0 + INDEX_A], t);

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
						temp_clipped_faces_out[index_out++] = a0;

						++temp_visible_faces_count;

						V3 p1;
						t = line_intersect_plane(ip1, op0, plane, p1);

						// Lerp for the attributes.
						float u1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_U], temp_clipped_faces_in[index_op0 + INDEX_U], t);
						float v1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_V], temp_clipped_faces_in[index_op0 + INDEX_V], t);
						float nx1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_NX], temp_clipped_faces_in[index_op0 + INDEX_NX], t);
						float ny1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_NY], temp_clipped_faces_in[index_op0 + INDEX_NY], t);
						float nz1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_NZ], temp_clipped_faces_in[index_op0 + INDEX_NZ], t);
						float r1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_R], temp_clipped_faces_in[index_op0 + INDEX_R], t);
						float g1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_G], temp_clipped_faces_in[index_op0 + INDEX_G], t);
						float b1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_B], temp_clipped_faces_in[index_op0 + INDEX_B], t);
						float a1 = lerp(temp_clipped_faces_in[index_ip1_copy + INDEX_A], temp_clipped_faces_in[index_op0 + INDEX_A], t);

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
						temp_clipped_faces_out[index_out++] = a0;

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
						temp_clipped_faces_out[index_out++] = a1;

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

			// Update the clipped faces index.
			clipped_faces_index = index_out;
			visible_faces_count = num_faces_to_process;
		}

		// Store the number of visible faces of the mesh after clipping.
		models->clipped_faces_counts[i] = visible_faces_count;

		// Move to the next mesh.
		face_offset += models->front_faces_counts[i];
	}

	// TODO: Lighting
	// Lighting involves the colour and normal of the vertex
	// I could do a loop of lighting calculations using this.


	// TODO: Drawing only needs the vertex colour and uv. I want the colour to act as a tint on the uv does that mean colour needs an alpha.
	//		 I would only want the alpha if the vertex had a colour and uv?

	// Project the view space vertices.
	//float min_w = 0;
	//float max_w = 0;
	
	// This must be done mesh by mesh so we know what texture to use.
	face_offset = 0;
	for (int i = 0; i < models->mis_count; ++i)
	{
		for (int j = face_offset; j < face_offset + mesh_clipped_faces_counts[i]; ++j)
		{
			int clipped_face_index = j * STRIDE_ENTIRE_FACE;

			// Project the vertex position.
			V4 v0 = {
				clipped_faces[clipped_face_index],
				clipped_faces[clipped_face_index + 1],
				clipped_faces[clipped_face_index + 2],
				1
			};

			V4 v1 = {
				clipped_faces[clipped_face_index + 12],
				clipped_faces[clipped_face_index + 13],
				clipped_faces[clipped_face_index + 14],
				1
			};

			V4 v2 = {
				clipped_faces[clipped_face_index + 24],
				clipped_faces[clipped_face_index + 25],
				clipped_faces[clipped_face_index + 26],
				1
			};

			V3 projected_v0, projected_v1, projected_v2;
			project(rt->canvas, settings->projection_matrix, v0, projected_v0);
			project(rt->canvas, settings->projection_matrix, v1, projected_v1);
			project(rt->canvas, settings->projection_matrix, v2, projected_v2);

			V4 colour0 = {
				clipped_faces[clipped_face_index + 8] * projected_v0[2],
				clipped_faces[clipped_face_index + 9] * projected_v0[2],
				clipped_faces[clipped_face_index + 10] * projected_v0[2],
				clipped_faces[clipped_face_index + 11] * projected_v0[2],
			};

			V4 colour1 = {
				clipped_faces[clipped_face_index + 20] * projected_v1[2],
				clipped_faces[clipped_face_index + 21] * projected_v1[2],
				clipped_faces[clipped_face_index + 22] * projected_v1[2],
				clipped_faces[clipped_face_index + 23] * projected_v1[2],
			};

			V4 colour2 = {
				clipped_faces[clipped_face_index + 32] * projected_v2[2],
				clipped_faces[clipped_face_index + 33] * projected_v2[2],
				clipped_faces[clipped_face_index + 34] * projected_v2[2],
				clipped_faces[clipped_face_index + 35] * projected_v2[2],
			};

			clip_and_draw_triangle(rt, models, projected_v0, projected_v1, projected_v2, colour0, colour1, colour2);
		}

		face_offset += mesh_clipped_faces_counts[i];
	}
}