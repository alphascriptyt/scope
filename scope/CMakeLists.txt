# CMakeList.txt : CMake project for scope, include source and define
# project specific logic here.
#

# Add source to this project's executable.

# TODO: For now just compiling all code here. Could compile engine as a lib.
add_executable (scope "main.c"

"engine/canvas.c"
"engine/engine.c"
"engine/lights.c"
"engine/models.c" 
"engine/window.c"
"engine/scene.c"
"engine/strides.c"

"engine/maths/matrix4.c"
"engine/maths/plane.c"

"engine/renderer/camera.c"
"engine/renderer/frustum_culling.c"
"engine/renderer/render.c"
"engine/renderer/renderer.c"
"engine/renderer/draw_2d.c"
"engine/renderer/depth_buffer.c"


"engine/ui/font.c"
"engine/ui/text.c"
"engine/ui/ui.c"

"engine/utils/memory_utils.c"
"engine/utils/str_utils.c"
 
)

include_directories ("engine")
include_directories ("engine/common")

# TODO: Add tests and install targets if needed.
