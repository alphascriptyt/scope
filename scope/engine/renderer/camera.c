#include "camera.h"

#include "maths/matrix4.h"
#include "maths/vector3.h"

void calculate_view_matrix(const Camera* camera, M4 out)
{
	// TODO: Should position not be -1?
	// TODO: This isn't an issue for now, however,
	//		 should test if the LookAt is working correctly.
	//		 Could do this using the camera and messh.!!
	// Look at is correct according to opengl docs.

	V3 inv_dir, inv_pos;
	v3_mul_f_out(camera->direction, -1, inv_dir);
	v3_mul_f_out(camera->position, -1, inv_pos);

	//look_at(inv_pos, inv_dir, out);
	look_at(camera->position, inv_dir, out);
}
