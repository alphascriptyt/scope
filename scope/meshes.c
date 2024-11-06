#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS // Hide warnings for strok.
#endif

#include "meshes.h"

#include "maths/vector3.h"
#include "maths/matrix4.h"

#include "utils/logger.h"
#include "utils/str_utils.h"
#include "utils/memory_utils.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void parse_obj_counts(FILE* file, int* num_positions, int* num_uvs, int* num_normals, int* num_faces)
{
	// Find the number of vertex components, faces etc so we can define the memory in one go.
	char line[256];

	*num_positions = 0;
	*num_uvs = 0;
	*num_normals = 0;
	*num_faces = 0;

	while (fgets(line, sizeof(line), file))
	{
		char* token;

		token = strtok(line, " ");

		if (strcmp(token, "v") == 0)
		{
			++*num_positions;
		}
		else if (strcmp(token, "vn") == 0)
		{
			++*num_normals;
		}
		else if (strcmp(token, "vt") == 0)
		{
			++*num_uvs;
		}
		else if (strcmp(token, "f") == 0)
		{
			++*num_faces;
		}
	}

	// Temp debugging information.
	printf("Positions: %d\n", *num_positions);
	printf("Normals: %d\n", *num_normals);
	printf("UVS: %d\n", *num_uvs);
	printf("Faces: %d\n", *num_faces);
}

void load_mesh_from_obj(Meshes* meshes, const char* filename, const V3 position, const V3 orientation, const V3 scale)
{
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

	//int num_positions, num_uvs, num_normals, num_faces;
	int positions_count, normals_count, uvs_count, face_count;
	parse_obj_counts(file, &positions_count, &uvs_count, &normals_count, &face_count);

	// Calculate the offsets to the next index after the last face in the buffer.
	int face_offset = 0;
	for (int i = 0; i < meshes->meshes_count; ++i)
	{
		face_offset += meshes->mesh_faces_counts[i];
	}

	// Resize the meshes buffers for storing the new data.
	resize_float_buffer(&meshes->colours, (meshes->colours_count + positions_count) * STRIDE_COLOUR);
	resize_float_buffer(&meshes->uvs, (meshes->uvs_count + uvs_count) * STRIDE_UV);

	resize_float_buffer(&meshes->model_space_normals, (meshes->normals_count + normals_count) * STRIDE_NORMAL);

	resize_float_buffer(&meshes->world_space_positions, (meshes->positions_count + positions_count) * STRIDE_POSITION);
	resize_float_buffer(&meshes->view_space_positions, (meshes->positions_count + positions_count) * STRIDE_POSITION);

	resize_float_buffer(&meshes->world_space_normals, (meshes->normals_count + normals_count) * STRIDE_NORMAL);
	resize_float_buffer(&meshes->view_space_normals, (meshes->normals_count + normals_count) * STRIDE_NORMAL);

	// Calculate the new number of faces in the buffer.
	int new_face_len = face_offset + face_count;

	resize_float_buffer(&meshes->face_attributes, new_face_len * STRIDE_FACE_ATTRIBUTES);
	resize_float_buffer(&meshes->front_faces, new_face_len * STRIDE_ENTIRE_FACE);
	resize_float_buffer(&meshes->clipped_faces, new_face_len * STRIDE_ENTIRE_FACE * (int)pow(2, 5)); //MAX_FRUSTUM_PLANES);

	// Recreate the in/out buffers for clipping if we need room for more faces.
	if (meshes->max_mesh_faces < face_count)
	{
		meshes->max_mesh_faces = face_count;

		resize_float_buffer(&meshes->temp_clipped_faces_in, face_count * STRIDE_ENTIRE_FACE * (int)pow(2, 5));
		resize_float_buffer(&meshes->temp_clipped_faces_out, face_count * STRIDE_ENTIRE_FACE * (int)pow(2, 5)); //MAX_FRUSTUM_PLANES);
		resize_float_buffer(&meshes->projected_clipped_faces, face_count * STRIDE_PROJECTED_FACE * (int)pow(2, 4));
		resize_float_buffer(&meshes->projected_clipped_faces_temp, face_count * STRIDE_PROJECTED_FACE * (int)pow(2, 4));
	}

	// A face is defined in three separate buffers, position indices, normal indices and attributes.
	resize_int_buffer(&meshes->face_position_indices, new_face_len * STRIDE_FACE_VERTICES);
	resize_int_buffer(&meshes->face_normal_indices, new_face_len * STRIDE_FACE_VERTICES);

	int new_meshes_count = meshes->meshes_count + 1;

	resize_int_buffer(&meshes->mesh_faces_counts, new_meshes_count);
	resize_int_buffer(&meshes->mesh_front_faces_counts, new_meshes_count);
	resize_int_buffer(&meshes->mesh_clipped_faces_counts, new_meshes_count);
	resize_int_buffer(&meshes->mesh_texture_ids, new_meshes_count);
	resize_int_buffer(&meshes->mesh_transforms_updated_flags, new_meshes_count);
	resize_float_buffer(&meshes->mesh_bounding_spheres, new_meshes_count * STRIDE_SPHERE);

	// Set the face count for the new mesh.
	meshes->mesh_faces_counts[meshes->meshes_count] = face_count;

	// Make the model matrix so we can pre-convert the model space to world space.

	// TODO: Currently im passing in orientation not pitch yaw roll...
	M4 model_matrix;
	make_model_m4(position, orientation, scale, model_matrix);

	resize_float_buffer(&meshes->model_space_positions, (meshes->positions_count + positions_count) * STRIDE_POSITION);

	resize_float_buffer(&meshes->mesh_transforms, (meshes->meshes_count + 1) * STRIDE_MESH_TRANSFORM);

	int index = meshes->meshes_count * STRIDE_MESH_TRANSFORM;
	meshes->mesh_transforms[index] = position[0];
	meshes->mesh_transforms[++index] = position[1];
	meshes->mesh_transforms[++index] = position[2];
	meshes->mesh_transforms[++index] = orientation[0];
	meshes->mesh_transforms[++index] = orientation[1];
	meshes->mesh_transforms[++index] = orientation[2];
	meshes->mesh_transforms[++index] = scale[0];
	meshes->mesh_transforms[++index] = scale[1];
	meshes->mesh_transforms[++index] = scale[2];

	resize_int_buffer(&meshes->mesh_positions_counts, (meshes->meshes_count + 1));
	resize_int_buffer(&meshes->mesh_normals_counts, (meshes->meshes_count + 1));

	// Move to the start of the file again so we can read it.
	rewind(file);

	// Define offsets to the start of the new mesh in the array.
	int positions_offset = meshes->positions_count * STRIDE_POSITION;
	int normals_offset = meshes->normals_count * STRIDE_NORMAL;
	int uvs_offset = meshes->uvs_count * STRIDE_UV;
	int colours_index = meshes->colours_count * STRIDE_COLOUR;
	int faces_positions_offset = face_offset * STRIDE_FACE_VERTICES;
	int faces_normals_offset = face_offset * STRIDE_FACE_VERTICES;
	int faces_attributes_offset = face_offset * STRIDE_FACE_ATTRIBUTES;
	
	// Fill the buffers from the file.
	char line[256];
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

			// Store the model space position.
			meshes->model_space_positions[positions_offset] = v[0];
			meshes->model_space_positions[positions_offset + 1] = v[1];
			meshes->model_space_positions[positions_offset + 2] = v[2];

			// Pre-calculate the world space positions.
			V4 world_v;
			m4_mul_v4(model_matrix, v, world_v);

			meshes->world_space_positions[positions_offset++] = world_v[0];
			meshes->world_space_positions[positions_offset++] = world_v[1];
			meshes->world_space_positions[positions_offset++] = world_v[2];
		}

		else if (strcmp(tokens[0], "vn") == 0)
		{
			meshes->model_space_normals[normals_offset++] = (float)atof(tokens[1]);
			meshes->model_space_normals[normals_offset++] = (float)atof(tokens[2]);
			meshes->model_space_normals[normals_offset++] = (float)atof(tokens[3]);
		}

		else if (strcmp(tokens[0], "vt") == 0)
		{
			meshes->uvs[uvs_offset++] = (float)atof(tokens[1]);
			meshes->uvs[uvs_offset++] = (float)atof(tokens[2]);
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
			meshes->face_position_indices[faces_positions_offset++] = meshes->positions_count + face_indices[0];
			meshes->face_position_indices[faces_positions_offset++] = meshes->positions_count + face_indices[3];
			meshes->face_position_indices[faces_positions_offset++] = meshes->positions_count + face_indices[6];

			meshes->face_normal_indices[faces_normals_offset++] = meshes->normals_count + face_indices[2];
			meshes->face_normal_indices[faces_normals_offset++] = meshes->normals_count + face_indices[5];
			meshes->face_normal_indices[faces_normals_offset++] = meshes->normals_count + face_indices[8];

			//meshes->face_attribute_indices[faces_positions_offset++] = meshes->uvs_count + face_indices[7];
			//meshes->face_attribute_indices[faces_positions_offset++] = meshes->normals_count + face_indices[8];
			//meshes->face_attribute_indices[faces_positions_offset++] = meshes->colours_count + colours_index;


			//u,v,x,y,z,r,g,b,a
			// Vertex 1
			meshes->face_attributes[faces_attributes_offset++] = meshes->uvs[meshes->uvs_count + face_indices[1]];				// U
			meshes->face_attributes[faces_attributes_offset++] = meshes->uvs[meshes->uvs_count + face_indices[1] + 1];			// V

			// TODO: TEMP: Hardcode colour
			meshes->face_attributes[faces_attributes_offset++] = 1; // R
			meshes->face_attributes[faces_attributes_offset++] = 0; // G
			meshes->face_attributes[faces_attributes_offset++] = 0; // B
			meshes->face_attributes[faces_attributes_offset++] = 1; // A
			
			// Vertex 2
			meshes->face_attributes[faces_attributes_offset++] = meshes->uvs[meshes->uvs_count + face_indices[4]];				// U
			meshes->face_attributes[faces_attributes_offset++] = meshes->uvs[meshes->uvs_count + face_indices[4] + 1];			// V
			
			// TODO: TEMP: Hardcode colour
			meshes->face_attributes[faces_attributes_offset++] = 1; // R
			meshes->face_attributes[faces_attributes_offset++] = 0; // G
			meshes->face_attributes[faces_attributes_offset++] = 0; // B
			meshes->face_attributes[faces_attributes_offset++] = 1; // A

			// Vertex 2
			meshes->face_attributes[faces_attributes_offset++] = meshes->uvs[meshes->uvs_count + face_indices[7]];				// U
			meshes->face_attributes[faces_attributes_offset++] = meshes->uvs[meshes->uvs_count + face_indices[7] + 1];			// V

			// TODO: TEMP: Hardcode colour
			meshes->face_attributes[faces_attributes_offset++] = 1; // R
			meshes->face_attributes[faces_attributes_offset++] = 0; // G
			meshes->face_attributes[faces_attributes_offset++] = 0; // B
			meshes->face_attributes[faces_attributes_offset++] = 1; // A
		}
	}

	// Calculate the bounding sphere of the mesh.
	V3 center = { 0, 0, 0 };

	// TODO: Would be cool to have something to render the bounding spherre......
	int n = 0;
	for (int i = face_offset * STRIDE_FACE_VERTICES; i < new_face_len * STRIDE_FACE_VERTICES; ++i)
	{
		int index = meshes->face_position_indices[i] * STRIDE_POSITION;
		V3 v = {
			meshes->world_space_positions[index],
			meshes->world_space_positions[index + 1],
			meshes->world_space_positions[index + 2]
		};

		v3_add_v3(center, v);
		n++;
	}
	
	v3_mul_f(center, 1.f / n);

	// Find farthest vertex from the center to calculate the radius.
	float radius_squared = 0;
	for (int i = face_offset * STRIDE_FACE_VERTICES; i < new_face_len * STRIDE_FACE_VERTICES; ++i)
	{
		int index = meshes->face_position_indices[i] * STRIDE_POSITION;
		V3 v = {
			meshes->world_space_positions[index],
			meshes->world_space_positions[index + 1],
			meshes->world_space_positions[index + 2]
		};

		// Calculate the length of the line between the center and the vertex.
		v3_sub_v3(v, center);
		radius_squared = max(size_squared(v), radius_squared); 
	}

	// Store the bounding sphere.
	int i = meshes->meshes_count * STRIDE_SPHERE;
	meshes->mesh_bounding_spheres[i++] = center[0];
	meshes->mesh_bounding_spheres[i++] = center[1];
	meshes->mesh_bounding_spheres[i++] = center[2];

	// We cannot compare using the squared dist as we take the signed
	// dist to the plane, so we would have to square the dist, losing
	// the sign information.
	meshes->mesh_bounding_spheres[i] = sqrtf(radius_squared);

	// Update the buffer lengths.
	meshes->colours_count += positions_count;
	meshes->positions_count += positions_count;
	meshes->normals_count += normals_count;
	meshes->uvs_count += uvs_count;

	// Flag that the mesh's world space positions doesn't need to be re-calculated.
	// TODO: Is there any need to do this here???
	// NOTE: CURRENTLY SET TO RECALC WHILST UPDATING CODE
	meshes->mesh_transforms_updated_flags[meshes->meshes_count] = 1;

	meshes->mesh_positions_counts[meshes->meshes_count] = positions_count;
	meshes->mesh_normals_counts[meshes->meshes_count] = normals_count;

	// Update the number of meshes.
	++meshes->meshes_count;

	// Close the file.
	if (fclose(file) != 0)
	{
		// TODO: Error closing.
		log_error("Failed to close file after loading .obj file.");
	}
}

void free_meshes(Meshes* meshes)
{
	// TODO: If i ever dynamically allocate those.
	//free(meshes->descriptions.mesh_faces_counts);
	//free(meshes->descriptions.vertex_counts);

	/*
	free(meshes->face_position_indices);
	free(meshes->face_attribute_indices);

	free(meshes->front_face_indices);
	free(meshes->world_space_positions);
	free(meshes->view_space_positions);
	free(meshes->screen_space_positions);

	free(meshes->colours);
	free(meshes->normals);
	free(meshes->uvs);*/

}


#undef _CRT_SECURE_NO_WARNINGS