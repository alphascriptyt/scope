#ifndef RENDER_SETTINGS_H
#define RENDER_SETTINGS_H

#include "canvas.h"
#include "frustum_culling.h"

#include "maths/matrix4.h"
#include "maths/utils.h"

// TODO: RenderSettings is a bit off. We also want to store the view frustum
//		 as this won't change unless fov changes. This stuff is slightly 
//		 different to the fov/near/farplane.
//			
typedef struct
{
	float fov;
	float near_plane; 

	// TODO: ^^^ Will have to experiment. We want this as large as possible without clipping too close. Apparently
	//	   Large outdoor scene: nearplane 0.1-1, far 500-1000/
	//	   Small indoor scene: nearplane 0.1-0.01, 50-100. 
	//float nearPlane = near_plane_dist * -1.f;

	float far_plane;

	// TODO: Should these go to the Renderer?
	M4 projection_matrix;
	ViewFrustum view_frustum; // TODO: Definitely should go in the renderer.

} RenderSettings;

inline void update_projection_m4(RenderSettings* rs, float aspect_ratio)
{
	// TODO: Fov is vertical fov here.
	// TODO: Comment all this properly to show I actually understand it all.

	// Currently the opengl perspective projection matrix.

	float y_scale = 1.f / tanf(radians(rs->fov) / 2.f);
	float x_scale = y_scale / aspect_ratio;

	rs->projection_matrix[0] = x_scale;
	rs->projection_matrix[1] = 0;
	rs->projection_matrix[2] = 0;
	rs->projection_matrix[3] = 0;
	rs->projection_matrix[4] = 0;
	rs->projection_matrix[5] = y_scale;
	rs->projection_matrix[6] = 0;
	rs->projection_matrix[7] = 0;
	rs->projection_matrix[8] = 0;
	rs->projection_matrix[9] = 0;
	rs->projection_matrix[10] = -(rs->far_plane + rs->near_plane) / (rs->far_plane - rs->near_plane);
	rs->projection_matrix[11] = -1; // This negation our right handed coordinate system into a left handed coordinate system in ndc space?
	rs->projection_matrix[12] = 0;
	rs->projection_matrix[13] = 0;
	rs->projection_matrix[14] = -2 * rs->far_plane * rs->near_plane / (rs->far_plane - rs->near_plane);
	rs->projection_matrix[15] = 0;
}

#endif