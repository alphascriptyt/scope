#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

// TODO: Allow for formatting like with printf.
inline void log_info(const char* msg)
{
	printf("[INFO] : %s\n", msg);
}

inline void log_warn(const char* msg)
{
	printf("[WARN] : %s\n", msg);
}

inline void log_error(const char* msg)
{
	fprintf(stderr, "[ERROR] : %s\n", msg);
}

#endif