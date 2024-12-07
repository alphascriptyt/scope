#ifndef RENDER_H
#define RENDER_H

#include "scene.h"
#include "models.h"
#include "lights.h"

#include "render_target.h"
#include "render_settings.h"
#include "canvas.h"

#include "frustum_culling.h"

#include "maths/vector3.h"
#include "maths/vector4.h"
#include "maths/matrix4.h"

// TODO: Organise this all.

// SECTION: Debug tools.
void debug_draw_point_lights(RenderTarget* rt, const RenderSettings* settings, PointLights* point_lights);
void debug_draw_bounding_spheres(RenderTarget* rt, const RenderSettings* settings, const Models* models, const M4 view_matrix);
void debug_draw_world_space_point(RenderTarget* rt, const RenderSettings* settings, const V3 point, const M4 view_matrix, int colour);
void debug_draw_view_space_point(RenderTarget* rt, const RenderSettings* settings, const V3 point, int colour);

// SECTION: 2D drawing functions.
void draw_line(RenderTarget* rt, int x0, int y0, int x1, int y1, const V3 colour);
void draw_circle(RenderTarget* rt, int cx, int cy, int r, const V3 colour);
void draw_rect(RenderTarget* rt, int x0, int y0, int x1, int y1, int colour);

// TODO: Not sure where to put this?

// TODO: Split these functions into sections.

float calculate_diffuse_factor(const V3 v, const V3 n, const V3 light_pos, float a, float b);

void draw_flat_bottom_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V4 c0, V4 c1, V4 c2);
void draw_flat_top_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V4 c0, V4 c1, V4 c2);
void draw_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V4 c0, V4 c1, V4 c2);
void draw_scanline(RenderTarget* rt, int x0, int x1, int y, float z0, float z1, float w0, float w1, const V4 c0, const V4 c1);

void project(const Canvas* canvas, const M4 projection_matrix, const V4 v, V4 o);

void model_to_view_space(Models* models, const M4 view_matrix);

void lights_world_to_view_space(PointLights* point_lights, const M4 view_matrix);

void broad_phase_frustum_culling(Models* models, const ViewFrustum* view_frustum);

void cull_backfaces(Models* models);

void frustum_culling(RenderTarget* rt, const M4 projection_matrix, const ViewFrustum* view_frustum, const M4 view_matrix, Models* models);

// ^^^ This also needs renaming. Not sure how I can name this?
// TODO: project_and_draw_clipped_triangles?
// TODO: After clipping one mesh, draw all it's triangles or do we just draw all the triangles after clippign ONE triangle?

void project_and_draw_clipped_triangles(RenderTarget* rt, const M4 projection_matrix, const Models* models, int mi_index, int clipped_face_count);

void render(RenderTarget* rt, const RenderSettings* settings, Scene* scene, const M4 view_matrix);

#endif