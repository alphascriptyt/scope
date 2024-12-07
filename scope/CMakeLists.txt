# CMakeList.txt : CMake project for scope, include source and define
# project specific logic here.
#

# Add source to this project's executable.

# TODO: For now just compiling all code here. Could compile engine as a lib.
add_executable (scope "main.c"

"engine/canvas.h" "engine/canvas.c"
"engine/engine.h" "engine/engine.c"
"engine/lights.h" "engine/lights.c"
"engine/models.h" "engine/models.c"
"engine/texture.h" "engine/texture.c"
"engine/window.h" "engine/window.c"
"engine/scene.h" "engine/scene.c"

"engine/common/colour.h"
"engine/common/status.h"

"engine/maths/matrix4.h" "engine/maths/matrix4.c"
"engine/maths/plane.h" "engine/maths/plane.c"
"engine/maths/utils.h"
"engine/maths/vector3.h" 
"engine/maths/vector4.h"

"engine/renderer/camera.h" "engine/renderer/camera.c"
"engine/renderer/frustum_culling.h" "engine/renderer/frustum_culling.c"
"engine/renderer/render.h" "engine/renderer/render.c"
"engine/renderer/render_settings.h" 
"engine/renderer/render_target.h"
"engine/renderer/renderer.h" "engine/renderer/renderer.c"

"engine/ui/font.h" "engine/ui/font.c"
"engine/ui/text.h" "engine/ui/text.c"
"engine/ui/ui.h" "engine/ui/ui.c"

"engine/utils/common.h"
"engine/utils/logger.h"
"engine/utils/memory_utils.h" "engine/utils/memory_utils.c"
"engine/utils/str_utils.h" "engine/utils/str_utils.c"
"engine/utils/timer.h"
  "engine/globals.h")

include_directories ("engine")
include_directories ("engine/common")

# TODO: Add tests and install targets if needed.
