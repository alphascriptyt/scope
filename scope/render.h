#ifndef RENDER_H
#define RENDER_H

#include "meshes.h"

#include "render_target.h"
#include "canvas.h"

#include "maths/vector3.h"
#include "maths/vector4.h"
#include "maths/matrix4.h"

// We will want static meshes that have their model matrices stored only and dynamic ones that the model matrices will have to be generated

void clip_and_draw_triangle(RenderTarget* rt, StaticMeshes* static_meshes, V3 v0, V3 v1, V3 v2, V4 c0, V4 c1, V4 c2);
void draw_flat_bottom_triangle(RenderTarget* rt, V3 v0, V3 v1, V3 v2, V4 c0, V4 c1, V4 c2);
void draw_flat_top_triangle(RenderTarget* rt, V3 v0, V3 v1, V3 v2, V4 c0, V4 c1, V4 c2);
void draw_triangle(RenderTarget* rt, V3 v0, V3 v1, V3 v2, V4 c0, V4 c1, V4 c2);
void draw_scanline(RenderTarget* rt, const int x0, const int x1, const int y, const float w0, const float w1, const V4 c0, const V4 c1);

void project(Canvas* canvas, const M4 projection_matrix, const V4 v, V3 o);


// Render Pipeline Stages:
// 
// Jump around using 'RENDER_STAGE:'
// 
// 1. RENDER_STAGE: World Space -> View Space  
// 2. RENDER_STAGE: Backface Culling
// 3. RENDER_STAGE: Frustum Culling
// 4. RENDER_STAGE: Lighting
// 5. RENDER_STAGE: Projection
// 6. RENDER_STAGE: Clipping & Drawing Triangles
void render(RenderTarget* rt, StaticMeshes* static_meshes, const M4 view_matrix);

#endif