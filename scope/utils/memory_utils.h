#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include "status.h"

// Helpers for resizing the meshes buffers.
Status resize_int_buffer(int** out_buffer, const int len);
Status resize_float_buffer(float** out_buffer, const int len);

#endif