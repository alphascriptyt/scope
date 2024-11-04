# CMakeList.txt : CMake project for scope, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (scope "main.c"   "engine.h" "engine.c" "render.h" "render.c"  "canvas.h" "canvas.c" "utils/logger.h" "ui/text.h" "ui/text.c" "ui/font.h" "ui/font.c" "utils/str_utils.h" "entities.h" "meshes.h" "meshes.c" "utils/str_utils.c" "maths/matrix4.h"  "utils/timer.h"  "maths/vector4.h"  "maths/vector3.h" "maths/matrix4.c" "camera.h" "camera.c"  "colour.h" "render_target.h" "texture.h" "texture.c" "utils/common.h" "frustum_culling.h" "maths/plane.h" "frustum_culling.c" "maths/plane.c" "status.h" "render_settings.h")

# TODO: Add tests and install targets if needed.
