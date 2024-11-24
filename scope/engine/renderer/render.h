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

// SECTION: Debug tools.
void draw_debug_point_lights(RenderTarget* rt, const RenderSettings* settings, PointLights* point_lights);

// TODO: Not sure where to put this?

// TODO: Split these functions into sections.

float calculate_diffuse_factor(const V3 v, const V3 n, const V3 light_pos, const float a, const float b);

void draw_line(RenderTarget* rt, float x0, float y0, float x1, float y1, const V3 colour);

void clip_and_draw_triangle(RenderTarget* rt, Models* models, V3 v0, V3 v1, V3 v2, V4 c0, V4 c1, V4 c2);
void draw_flat_bottom_triangle(RenderTarget* rt, V3 v0, V3 v1, V3 v2, V4 c0, V4 c1, V4 c2);
void draw_flat_top_triangle(RenderTarget* rt, V3 v0, V3 v1, V3 v2, V4 c0, V4 c1, V4 c2);
void draw_triangle(RenderTarget* rt, V3 v0, V3 v1, V3 v2, V4 c0, V4 c1, V4 c2);
void draw_scanline(RenderTarget* rt, const int x0, const int x1, const int y, const float w0, const float w1, const V4 c0, const V4 c1);

void project(const Canvas* canvas, const M4 projection_matrix, const V4 v, V3 o);

void model_to_world_space(Models* models);
void world_to_view_space(Models* models, PointLights* point_lights, const M4 view_matrix);

void cull_backfaces(Models* models);

void frustum_culling_and_lighting(RenderTarget* rt, const M4 projection_matrix, const ViewFrustum* view_frustum, const M4 view_matrix, Scene* scene);

void project_and_draw_triangles(RenderTarget* rt, const M4 projection_matrix, Models* models);

void render(RenderTarget* rt, const RenderSettings* settings, Scene* scene, const M4 view_matrix);

#endif