#ifndef ENTITIES_H
#define ENTITIES_H

#define MAX_ENTITIES = 1000;

typedef struct
{
	float* mesh_vertex_offsets;
	float* mesh_vertex_counts;


} Entities;


// When we render a scene, we need vertices, model position, direction and size,.

/*
* 
* Come back to this once ive finsihed the talk. 


- A mesh can be re-used, it contains vertex data essentially.

- the model matrix data is per entity.

- an entity can have flags, visible?


entity
	position
	direction
	size
	visible



From 

*/


#endif