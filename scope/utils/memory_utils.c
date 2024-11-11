#include "memory_utils.h"

#include "logger.h"

#include <stdlib.h>

Status resize_int_buffer(int** out_buffer, const unsigned int len)
{
	int* temp_ptr = realloc(*out_buffer, len * sizeof(int));
	if (0 == temp_ptr)
	{
		log_error("Failed to resize_int_buffer.");
		return ALLOC_ERR;
	}

	*out_buffer = temp_ptr;
	return OK;
}

Status resize_float_buffer(float** out_buffer, const unsigned int len)
{
	float* temp_ptr = realloc(*out_buffer, len * sizeof(float));
	if (0 == temp_ptr)
	{
		log_error("Failed to resize_float_buffer.");
		return ALLOC_ERR;
	}

	*out_buffer = temp_ptr;
	return OK;
}