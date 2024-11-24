#ifndef LIGHTS_H
#define LIGHTS_H

#include "maths/vector3.h"

#define STRIDE_POINT_LIGHT_ATTRIBUTES 4 // r,g,b,n
// TODO: I would like to also have STRIDE_POSITION here. Should strides be defined in a separate header to model.h?

/*
A point light can be defined by a:
- Position
- Colour
- Strength

A vertex can absorb a certain amount of colour, this is it's colour,
therefore, if a white light shines on it, it will absorb more red if its
a red colour. 

A light does not need an alpha.

*/


// TODO: When I have scenes, I think this will just be in the scene.
//		 Will we have a single models/lights struct per scene?
 
typedef struct
{
	int count;

	// Data
	// Keep separate for when we convert from world space
	// to view space to avoid loading unncessary data into
	// the cache. Also as we cache the view space positions, 
	// we would not be accessing the world space positions again anyways.
	float* world_space_positions;
	float* attributes; // Strength and colour.
	
	// Cache the point light's view space position.
	float* view_space_positions; 

} PointLights;


void point_lights_init(PointLights* point_lights);

void point_lights_create(PointLights* point_lights, const V3 position, const V3 colour, float strength);



#endif