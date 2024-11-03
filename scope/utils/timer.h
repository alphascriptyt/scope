#ifndef TIMER_H
#define TIMER_H

#include <time.h>

typedef struct 
{
	clock_t start;

} Timer;

inline Timer start_timer()
{
	Timer timer =
	{
		.start = clock()
	};
	
	return timer;
}

inline clock_t get_elapsed(Timer* timer)
{

	return (clock() - timer->start) * 1000 / CLOCKS_PER_SEC;
}

inline void restart_timer(Timer* timer)
{
	timer->start = clock();
}

#endif

