#ifndef RENDER_H
#define RENDER_H

// This stuff feels quite renderer specific, maybe except from resources.
// TODO: Think about global-stuff.
#include "scene.h"
#include "models.h"
#include "lights.h"
#include "resources.h"

#include "render_target.h"
#include "render_settings.h"
#include "canvas.h"

#include "frustum_culling.h"

#include "maths/vector2.h"
#include "maths/vector3.h"
#include "maths/vector4.h"
#include "maths/matrix4.h"

// TODO: Organise this all.

// SECTION: Debug tools.
// TODO: Refactor and comments etc.
void debug_draw_point_lights(Canvas* canvas, const RenderSettings* settings, PointLights* point_lights);
void debug_draw_bounding_spheres(Canvas* canvas, const RenderSettings* settings, const Models* models, const M4 view_matrix);
void debug_draw_world_space_point(Canvas* canvas, const RenderSettings* settings, const V3 point, const M4 view_matrix, int colour);
void debug_draw_view_space_point(Canvas* canvas, const RenderSettings* settings, const V3 point, int colour);
void debug_draw_world_space_line(Canvas* canvas, const RenderSettings* settings, const M4 view_matrix, const V3 v0, const V3 v1, const V3 colour);
void debug_draw_mi_normals(Canvas* canvas, const RenderSettings* settings, const Models* models, int mi_index);

// TODO: Not sure where to put this?

// TODO: Split these functions into sections.

float calculate_diffuse_factor(const V3 v, const V3 n, const V3 light_pos, float a, float b);

// SECTION: Triangle rasterisation.
// TODO: Could have a vertex struct for all these.
void draw_scanline(RenderTarget* rt, int x0, int x1, int y, float z0, float z1, float w0, float w1, const V3 c0, const V3 c1);
void draw_flat_bottom_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2);
void draw_flat_top_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2);
void draw_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2);

// TODO: Rename?
void draw_textured_scanline(RenderTarget* rt, int x0, int x1, int y, float z0, float z1, float w0, float w1, const V3 c0, const V3 c1, const V2 uv0, const V2 uv1, const Canvas* texture);
void draw_textured_flat_bottom_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2, V2 uv0, V2 uv1, V2 uv2, const Canvas* texture);
void draw_textured_flat_top_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V3 c0, V3 c1, V3 c2, V2 uv0, V2 uv1, V2 uv2, const Canvas* texture);
void draw_textured_triangle(RenderTarget* rt, V4 v0, V4 v1, V4 v2, V4 c0, V3 c1, V3 c2, V3 uv0, V2 uv1, V2 uv2, const Canvas* texture);

// SECTION: Render loop.
void project(const Canvas* canvas, const M4 projection_matrix, const V4 v, V4 o);

void model_to_view_space(Models* models, const M4 view_matrix);

void lights_world_to_view_space(PointLights* point_lights, const M4 view_matrix);

void broad_phase_frustum_culling(Models* models, const ViewFrustum* view_frustum);

void cull_backfaces(Models* models);

void light_front_faces(Scene* scene);

void clip_to_screen(RenderTarget* rt, const M4 projection_matrix, const ViewFrustum* view_frustum, const M4 view_matrix, Models* models, const Resources* resources);

void project_and_draw_clipped(RenderTarget* rt, const M4 projection_matrix, const Models* models, int mi_index, int clipped_face_count, const Resources* resources);

void render(RenderTarget* rt, const RenderSettings* settings, Scene* scene, const Resources* resources, const M4 view_matrix);

#endif