#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS // Hide warnings for strok.
#endif

#include "meshes.h"

#include "maths/vector3.h"
#include "maths/matrix4.h"

#include "utils/logger.h"
#include "utils/str_utils.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void load_static_mesh_from_obj(StaticMeshes* static_meshes, const char* filename, const V3 position, const V3 orientation, const V3 scale)
{
	// TODO: Would be nice to share some code between the load_dynamic mesh etc.
	//		 Or I could load a mesh, then.... convert it to static? THen the logic would be the same.

	// TODO: Eventually could check the filetype.
	FILE* file = fopen(filename, "r");

	if (NULL == file)
	{
		char* msg = format_str("Failed to open '%c' when loading .obj file.", filename);
		log_error(msg);
		free(msg);

		// TODO: Return error code. Could make a file for this.
		return;
	}

	// Find the number of vertex components, faces etc so we can define the memory in 
	// one go.
	char line[256];

	int positions_count = 0;
	int uvs_count = 0;
	int normals_count = 0;
	int face_count = 0;

	while (fgets(line, sizeof(line), file))
	{
		char* token;

		token = strtok(line, " ");

		if (strcmp(token, "v") == 0)
		{
			++positions_count;
		}
		else if (strcmp(token, "vn") == 0)
		{
			++normals_count;
		}
		else if (strcmp(token, "vt") == 0)
		{
			++uvs_count;
		}
		else if (strcmp(token, "f") == 0)
		{
			++face_count;
		}
	}

	// Temp debugging information.
	printf("Positions: %d\n", positions_count);
	printf("Normals: %d\n", normals_count);
	printf("UVS: %d\n", uvs_count);
	printf("Faces: %d\n", face_count);

	// Calculate the offsets to the next index after the last face in the buffer.
	int face_offset = 0;
	for (int i = 0; i < static_meshes->mesh_count; ++i)
	{
		face_offset += static_meshes->face_counts[i];
	}

	// Allocate memory for the new data.
	// Currently just RGB, we can pack the A when writing to canvas.
	// For now we will have a colour for each vertex not sharing like the texture coordinates. 
	// TODO: WIll have to come back to this.
	float* temp_ptr = realloc(static_meshes->colours, sizeof(float) * (size_t)(static_meshes->colours_count + positions_count) * STRIDE_COLOUR);
	if (temp_ptr == NULL)
	{
		log_error("Failed to realloc for mesh vertices colours.");
	}
	else
	{
		static_meshes->colours = temp_ptr;
	}

	temp_ptr = realloc(static_meshes->normals, sizeof(float) * (size_t)(static_meshes->normals_count + normals_count) * STRIDE_NORMAL);
	if (temp_ptr == NULL)
	{
		log_error("Failed to realloc for mesh vertices normals.");
	}
	else
	{
		static_meshes->normals = temp_ptr;
	}

	temp_ptr = realloc(static_meshes->uvs, sizeof(float) * (size_t)(static_meshes->uvs_count + uvs_count) * STRIDE_UV);
	if (temp_ptr == NULL)
	{
		log_error("Failed to realloc for mesh vertices uvs.");
	}
	else
	{
		static_meshes->uvs = temp_ptr;
	}

	temp_ptr = realloc(static_meshes->world_space_positions, sizeof(float) * (size_t)(static_meshes->positions_count + positions_count) * STRIDE_POSITION);
	if (temp_ptr == NULL)
	{
		log_error("Failed to realloc for mesh vertex world space positions.");
	}
	else
	{
		static_meshes->world_space_positions = temp_ptr;
	}

	temp_ptr = realloc(static_meshes->view_space_positions, sizeof(float) * (size_t)(static_meshes->positions_count + positions_count) * STRIDE_POSITION);
	if (temp_ptr == NULL)
	{
		log_error("Failed to realloc for mesh view_space_positions.");
	}
	else
	{
		static_meshes->view_space_positions = temp_ptr;
	}

	// Calculate the new number of faces in the buffer.
	int new_face_len = face_offset + face_count;

	temp_ptr = realloc(static_meshes->face_attributes, sizeof(float) * new_face_len * STRIDE_FACE_ATTRIBUTES);
	if (temp_ptr == NULL)
	{
		log_error("Failed to realloc for mesh face_attributes.");
	}
	else
	{
		static_meshes->face_attributes = temp_ptr;
	}

	temp_ptr = realloc(static_meshes->front_faces, sizeof(float) * new_face_len * STRIDE_ENTIRE_FACE);
	if (temp_ptr == NULL)
	{
		log_error("Failed to realloc for mesh front_faces.");
	}
	else
	{
		static_meshes->front_faces = temp_ptr;
	}
	
	temp_ptr = realloc(static_meshes->clipped_faces, sizeof(float) * new_face_len * STRIDE_ENTIRE_FACE * 5); //MAX_FRUSTUM_PLANES);
	if (temp_ptr == NULL)
	{
		log_error("Failed to realloc for mesh clipped_faces.");
	}
	else
	{
		static_meshes->clipped_faces = temp_ptr;
	}

	// Recreate the in/out buffers for clipping if we need room for more faces.
	if (static_meshes->max_mesh_faces < face_count)
	{
		static_meshes->max_mesh_faces = face_count;

		temp_ptr = realloc(static_meshes->temp_clipped_faces_in, sizeof(float) * face_count * STRIDE_ENTIRE_FACE * 5); //MAX_FRUSTUM_PLANES);
		if (temp_ptr == NULL)
		{
			log_error("Failed to realloc for mesh temp_clipped_faces_in.");
		}
		else
		{
			static_meshes->temp_clipped_faces_in = temp_ptr;
		}

		temp_ptr = realloc(static_meshes->temp_clipped_faces_out, sizeof(float) * face_count * STRIDE_ENTIRE_FACE * 5); //MAX_FRUSTUM_PLANES);
		if (temp_ptr == NULL)
		{
			log_error("Failed to realloc for mesh temp_clipped_faces_out.");
		}
		else
		{
			static_meshes->temp_clipped_faces_out = temp_ptr;
		}

		temp_ptr = realloc(static_meshes->projected_clipped_faces, sizeof(float) * face_count * STRIDE_PROJECTED_FACE * 4);
		if (temp_ptr == NULL)
		{
			log_error("Failed to realloc for mesh projected_clipped_faces.");
		}
		else
		{
			static_meshes->projected_clipped_faces = temp_ptr;
		}

		temp_ptr = realloc(static_meshes->projected_clipped_faces_temp, sizeof(float) * face_count * STRIDE_PROJECTED_FACE * 4);
		if (temp_ptr == NULL)
		{
			log_error("Failed to realloc for mesh projected_clipped_faces_temp.");
		}
		else
		{
			static_meshes->projected_clipped_faces_temp = temp_ptr;
		}
	}

	// A face is defined in two separate buffers, position indices and 'attribute' indices.
	int* temp_ptr_int = realloc(static_meshes->face_position_indices, sizeof(int) * new_face_len * STRIDE_FACE_POSITIONS);
	if (temp_ptr_int == NULL)
	{
		log_error("Failed to realloc for mesh geometries faces.");
	}
	else
	{
		static_meshes->face_position_indices = temp_ptr_int;
	}

	int new_meshes_count = static_meshes->mesh_count + 1;

	temp_ptr_int = realloc(static_meshes->face_counts, sizeof(int) * new_meshes_count);
	if (temp_ptr_int == NULL)
	{
		log_error("Failed to realloc for face_counts.");
	}
	else
	{
		static_meshes->face_counts = temp_ptr_int;
	}

	temp_ptr_int = realloc(static_meshes->front_face_counts, sizeof(int) * new_meshes_count);
	if (temp_ptr_int == NULL)
	{
		log_error("Failed to realloc for front_face_counts.");
	}
	else
	{
		static_meshes->front_face_counts = temp_ptr_int;
	}

	temp_ptr_int = realloc(static_meshes->clipped_face_counts, sizeof(int) * new_meshes_count);
	if (temp_ptr_int == NULL)
	{
		log_error("Failed to realloc for clipped_face_counts.");
	}
	else
	{
		static_meshes->clipped_face_counts = temp_ptr_int;
	}

	
	temp_ptr_int = realloc(static_meshes->mesh_texture_ids, sizeof(int) * new_meshes_count);
	if (temp_ptr_int == NULL)
	{
		log_error("Failed to realloc for mesh_texture_ids.");
	}
	else
	{
		static_meshes->mesh_texture_ids = temp_ptr_int;
	}

	temp_ptr = realloc(static_meshes->mesh_bounding_spheres, sizeof(float) * new_meshes_count * STRIDE_SPHERE);
	if (temp_ptr == NULL)
	{
		log_error("Failed to realloc for mesh_bounding_spheres.");
	}
	else
	{
		static_meshes->mesh_bounding_spheres = temp_ptr;
	}

	/*
	// TODO: TEMP: Set the vertex colours to red as the default. TODO: Set this somewhere
	for (int i = 0; i < positions_count; ++i)
	{
		// TODO: Storing colours as ints would be better for performance, however,
		// I struggled to get interpolation to work with this last time. I think I 
		// could do it now.

		int j = i * STRIDE_COLOUR;
		static_meshes->colours[static_meshes->colours_count + j] = 1;
		static_meshes->colours[static_meshes->colours_count + j + 1] = 0;
		static_meshes->colours[static_meshes->colours_count + j + 2] = 0;
		static_meshes->colours[static_meshes->colours_count + j + 3] = 1;
	}*/

	// Set the face count for the new mesh.
	static_meshes->face_counts[static_meshes->mesh_count] = face_count;

	// Make the model matrix so we can pre-convert the model space to world space.

	// TODO: Currently im passing in orientation not pitch yaw roll...
	M4 model_matrix;
	make_model_m4(position, orientation, scale, model_matrix);

	// Move to the start of the file again so we can read it.
	rewind(file);

	// Define offsets to the start of the new mesh in the array.
	int positions_offset = static_meshes->positions_count * STRIDE_POSITION;
	int normals_offset = static_meshes->normals_count * STRIDE_NORMAL;
	int uvs_offset = static_meshes->uvs_count * STRIDE_UV;
	int colours_index = static_meshes->colours_count * STRIDE_COLOUR;
	int faces_positions_offset = face_offset * STRIDE_FACE_POSITIONS;
	int faces_attributes_offset = face_offset * STRIDE_FACE_ATTRIBUTES;
	
	// Fill the buffers from the file.
	while (fgets(line, sizeof(line), file))
	{
		// Split the line into its tokens, we only need 4.
		char* tokens[4] = { "", "", "", "" };
		char* token = strtok(line, " ");

		int i = 0;
		while (token != NULL && i < 4)
		{
			tokens[i++] = token;
			token = strtok(NULL, " ");
		}

		if (strcmp(tokens[0], "v") == 0)
		{
			// Apply the model matrix to put the vertices in world space.
			V4 v = {
				(float)atof(tokens[1]),
				(float)atof(tokens[2]),
				(float)atof(tokens[3]),
				1
			};

			V4 world_v;
			m4_mul_v4(model_matrix, v, world_v);

			static_meshes->world_space_positions[positions_offset++] = world_v[0];
			static_meshes->world_space_positions[positions_offset++] = world_v[1];
			static_meshes->world_space_positions[positions_offset++] = world_v[2];
		}

		else if (strcmp(tokens[0], "vn") == 0)
		{
			static_meshes->normals[normals_offset++] = (float)atof(tokens[1]);
			static_meshes->normals[normals_offset++] = (float)atof(tokens[2]);
			static_meshes->normals[normals_offset++] = (float)atof(tokens[3]);
		}

		else if (strcmp(tokens[0], "vt") == 0)
		{
			static_meshes->uvs[uvs_offset++] = (float)atof(tokens[1]);
			static_meshes->uvs[uvs_offset++] = (float)atof(tokens[2]);
		}

		else if (strcmp(tokens[0], "f") == 0)
		{
			// A face from the obj file is vertex index, uv index, normal index.
			int face_indices[9] = { 0 };

			// Faces must be triangulated
			for (int i = 0; i < 3; ++i)
			{
				// Split the face into its 3 indices pos, uv, normal
				char* face_token = strtok(tokens[1 + i], "/");

				int j = 0;
				while (face_token != NULL && j < 3)
				{
					face_indices[i * 3 + j] = atoi(face_token) - 1; // All indices are 1 based.
					face_token = strtok(NULL, "/");

					j++;
				}
			}

			// A face is made up of indices like: position, uv, normal, colour.
			// We split the position and attribute indices up as they aren't always used together.
			// TODO: Should the attribute indices be split up.
			static_meshes->face_position_indices[faces_positions_offset++] = static_meshes->positions_count + face_indices[0];

			//static_meshes->face_attribute_indices[faces_positions_offset++] = static_meshes->uvs_count + face_indices[1];
			//static_meshes->face_attribute_indices[faces_positions_offset++] = static_meshes->normals_count + face_indices[2];
			//static_meshes->face_attribute_indices[faces_positions_offset++] = static_meshes->colours_count + colours_index;

			static_meshes->face_position_indices[faces_positions_offset++] = static_meshes->positions_count + face_indices[3];

			//static_meshes->face_attribute_indices[faces_positions_offset++] = static_meshes->uvs_count + face_indices[4];
			//static_meshes->face_attribute_indices[faces_positions_offset++] = static_meshes->normals_count + face_indices[5];
			//static_meshes->face_attribute_indices[faces_positions_offset++] = static_meshes->colours_count + colours_index;

			static_meshes->face_position_indices[faces_positions_offset++] = static_meshes->positions_count + face_indices[6];

			//static_meshes->face_attribute_indices[faces_positions_offset++] = static_meshes->uvs_count + face_indices[7];
			//static_meshes->face_attribute_indices[faces_positions_offset++] = static_meshes->normals_count + face_indices[8];
			//static_meshes->face_attribute_indices[faces_positions_offset++] = static_meshes->colours_count + colours_index;


			//u,v,x,y,z,r,g,b,a
			// Vertex 1
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->uvs[static_meshes->uvs_count + face_indices[1]];				// U
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->uvs[static_meshes->uvs_count + face_indices[1] + 1];			// V
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->normals[static_meshes->normals_count + face_indices[2]];		// X
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->normals[static_meshes->normals_count + face_indices[2] + 1]; // Y
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->normals[static_meshes->normals_count + face_indices[2] + 2]; // Z

			// TODO: TEMP: Hardcode colour
			static_meshes->face_attributes[faces_attributes_offset++] = 1; // R
			static_meshes->face_attributes[faces_attributes_offset++] = 0; // G
			static_meshes->face_attributes[faces_attributes_offset++] = 0; // B
			static_meshes->face_attributes[faces_attributes_offset++] = 1; // A
			
			// Vertex 2
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->uvs[static_meshes->uvs_count + face_indices[4]];				// U
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->uvs[static_meshes->uvs_count + face_indices[4] + 1];			// V
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->normals[static_meshes->normals_count + face_indices[5]];		// X
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->normals[static_meshes->normals_count + face_indices[5] + 1]; // Y
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->normals[static_meshes->normals_count + face_indices[5] + 2]; // Z

			// TODO: TEMP: Hardcode colour
			static_meshes->face_attributes[faces_attributes_offset++] = 0; // R
			static_meshes->face_attributes[faces_attributes_offset++] = 1; // G
			static_meshes->face_attributes[faces_attributes_offset++] = 0; // B
			static_meshes->face_attributes[faces_attributes_offset++] = 1; // A

			// Vertex 2
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->uvs[static_meshes->uvs_count + face_indices[7]];				// U
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->uvs[static_meshes->uvs_count + face_indices[7] + 1];			// V
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->normals[static_meshes->normals_count + face_indices[8]];		// X
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->normals[static_meshes->normals_count + face_indices[8] + 1]; // Y
			static_meshes->face_attributes[faces_attributes_offset++] = static_meshes->normals[static_meshes->normals_count + face_indices[8] + 2]; // Z

			// TODO: TEMP: Hardcode colour
			static_meshes->face_attributes[faces_attributes_offset++] = 0; // R
			static_meshes->face_attributes[faces_attributes_offset++] = 0; // G
			static_meshes->face_attributes[faces_attributes_offset++] = 1; // B
			static_meshes->face_attributes[faces_attributes_offset++] = 1; // A
		}
	}

	// Calculate the bounding sphere of the mesh.
	V3 center = { 0, 0, 0 };

	// TODO: Would be cool to have something to render the bounding spherre......
	int n = 0;
	for (int i = face_offset * STRIDE_FACE_POSITIONS; i < new_face_len * STRIDE_FACE_POSITIONS; ++i)
	{
		int index = static_meshes->face_position_indices[i] * STRIDE_POSITION;
		V3 v = {
			static_meshes->world_space_positions[index],
			static_meshes->world_space_positions[index + 1],
			static_meshes->world_space_positions[index + 2]
		};

		v3_add_v3(center, v);
		n++;
	}
	
	v3_mul_f(center, 1.f / n);

	// Find farthest vertex from the center to calculate the radius.
	float radius_squared = 0;
	for (int i = face_offset * STRIDE_FACE_POSITIONS; i < new_face_len * STRIDE_FACE_POSITIONS; ++i)
	{
		int index = static_meshes->face_position_indices[i] * STRIDE_POSITION;
		V3 v = {
			static_meshes->world_space_positions[index],
			static_meshes->world_space_positions[index + 1],
			static_meshes->world_space_positions[index + 2]
		};

		// Calculate the length of the line between the center and the vertex.
		v3_sub_v3(v, center);
		radius_squared = max(size_squared(v), radius_squared); 
	}

	// Store the bounding sphere.
	int i = static_meshes->mesh_count * STRIDE_SPHERE;
	static_meshes->mesh_bounding_spheres[i++] = center[0];
	static_meshes->mesh_bounding_spheres[i++] = center[1];
	static_meshes->mesh_bounding_spheres[i++] = center[2];
	// We cannot compare using the squared dist as we take the signed
	// dist to the plane, so we would have to square the dist, losing
	// the sign information.
	static_meshes->mesh_bounding_spheres[i] = sqrtf(radius_squared);

	// Update the buffer lengths.
	static_meshes->colours_count += positions_count;
	static_meshes->positions_count += positions_count;
	static_meshes->normals_count += normals_count;
	static_meshes->uvs_count += uvs_count;

	// Update the number of meshes.
	++static_meshes->mesh_count;

	// Close the file.
	if (fclose(file) != 0)
	{
		// TODO: Error closing.
		log_error("Failed to close file after loading .obj file.");
	}
}



void free_static_meshes(StaticMeshes* static_meshes)
{
	// TODO: If i ever dynamically allocate those.
	//free(static_meshes->descriptions.face_counts);
	//free(static_meshes->descriptions.vertex_counts);

	/*
	free(static_meshes->face_position_indices);
	free(static_meshes->face_attribute_indices);

	free(static_meshes->front_face_indices);
	free(static_meshes->world_space_positions);
	free(static_meshes->view_space_positions);
	free(static_meshes->screen_space_positions);

	free(static_meshes->colours);
	free(static_meshes->normals);
	free(static_meshes->uvs);*/

}


#undef _CRT_SECURE_NO_WARNINGS