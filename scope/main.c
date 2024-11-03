#include <stdio.h>

#include "engine.h"

// TODO: Look into switching to an actual c compiler, not sure how to check that its compiled with c or if it even matters.

/*

I've decided that recoding range in the same way as before was getting old, i was just copying and editing code.
Also, performance is going to be a major issue, so I want to fix this by using Data Oriented Design, essentially
for entities we will have a struct of arrays instead of an array of structs.

Also, why am I bothering with stuff like ui frames for now...

All win32 stuff should be the same.
*/

int main()
{
	Engine* engine = init_engine();	
	start_engine(engine);
	
	return 0;
}
