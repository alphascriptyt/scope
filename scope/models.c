#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS // Hide warnings for strok.
#endif

#include "models.h"
#include "status.h"

#include "maths/vector3.h"
#include "maths/matrix4.h"

#include "utils/logger.h"
#include "utils/str_utils.h"
#include "utils/memory_utils.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "models.h"

void init_models(Models* models)
{
	// TODO: Comments

	// Projected faces are clipped one at a time, so the buffer can be allocated now.
	resize_float_buffer(&models->projected_clipped_faces, STRIDE_PROJECTED_FACE * (int)pow(2, 4));
	resize_float_buffer(&models->projected_clipped_faces_temp, STRIDE_PROJECTED_FACE * (int)pow(2, 4));

}

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

/*
void load_mesh_from_obj(Models* models, const char* filename, const V3 position, const V3 orientation, const V3 scale)
{
	// TODO: Fix the way we store transformations, do i want a direction or a pitch yaw roll? Is orientation the right word?

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

	int positions_count, normals_count, uvs_count, face_count;
	parse_obj_counts(file, &positions_count, &uvs_count, &normals_count, &face_count);

	// Calculate the offsets to the next index after the last face in the buffer.
	int face_offset = 0;
	for (int i = 0; i < models->models_count; ++i)
	{
		face_offset += models->mesh_faces_counts[i];
	}

	// Resize the models buffers for storing the new data.
	resize_float_buffer(&models->colours, (models->colours_count + positions_count) * STRIDE_COLOUR);
	resize_float_buffer(&models->uvs, (models->uvs_count + uvs_count) * STRIDE_UV);

	resize_float_buffer(&models->object_space_normals, (models->normals_count + normals_count) * STRIDE_NORMAL);

	resize_float_buffer(&models->world_space_positions, (models->positions_count + positions_count) * STRIDE_POSITION);
	resize_float_buffer(&models->view_space_positions, (models->positions_count + positions_count) * STRIDE_POSITION);

	resize_float_buffer(&models->world_space_normals, (models->normals_count + normals_count) * STRIDE_NORMAL);
	resize_float_buffer(&models->view_space_normals, (models->normals_count + normals_count) * STRIDE_NORMAL);

	// Calculate the new number of faces in the buffer.
	int new_face_len = face_offset + face_count;

	resize_float_buffer(&models->face_attributes, new_face_len * STRIDE_FACE_ATTRIBUTES);
	resize_float_buffer(&models->front_faces, new_face_len * STRIDE_ENTIRE_FACE);
	resize_float_buffer(&models->clipped_faces, new_face_len * STRIDE_ENTIRE_FACE * (int)pow(2, 5)); //MAX_FRUSTUM_PLANES);

	// Recreate the in/out buffers for clipping if we need room for more faces.
	if (models->max_mesh_faces < face_count)
	{
		models->max_mesh_faces = face_count;

		resize_float_buffer(&models->temp_clipped_faces_in, face_count * STRIDE_ENTIRE_FACE * (int)pow(2, 5));
		resize_float_buffer(&models->temp_clipped_faces_out, face_count * STRIDE_ENTIRE_FACE * (int)pow(2, 5)); //MAX_FRUSTUM_PLANES);
		resize_float_buffer(&models->projected_clipped_faces, face_count * STRIDE_PROJECTED_FACE * (int)pow(2, 4));
		resize_float_buffer(&models->projected_clipped_faces_temp, face_count * STRIDE_PROJECTED_FACE * (int)pow(2, 4));
	}

	// A face is defined in three separate buffers, position indices, normal indices and attributes.
	resize_int_buffer(&models->face_position_indices, new_face_len * STRIDE_FACE_VERTICES);
	resize_int_buffer(&models->face_normal_indices, new_face_len * STRIDE_FACE_VERTICES);

	int new_models_count = models->models_count + 1;

	resize_int_buffer(&models->mesh_faces_counts, new_models_count);
	resize_int_buffer(&models->mesh_front_faces_counts, new_models_count);
	resize_int_buffer(&models->mesh_clipped_faces_counts, new_models_count);
	resize_int_buffer(&models->mesh_texture_ids, new_models_count);
	resize_int_buffer(&models->transforms_updated_flags, new_models_count);
	resize_float_buffer(&models->bounding_spheres, new_models_count * STRIDE_SPHERE);

	// Set the face count for the new mesh.
	models->mesh_faces_counts[models->models_count] = face_count;

	resize_float_buffer(&models->object_space_positions, (models->positions_count + positions_count) * STRIDE_POSITION);

	resize_float_buffer(&models->mesh_transforms, (models->models_count + 1) * STRIDE_MI_TRANSFORM);

	int index = models->models_count * STRIDE_MI_TRANSFORM;
	models->mesh_transforms[index] = position[0];
	models->mesh_transforms[++index] = position[1];
	models->mesh_transforms[++index] = position[2];
	models->mesh_transforms[++index] = orientation[0];
	models->mesh_transforms[++index] = orientation[1];
	models->mesh_transforms[++index] = orientation[2];
	models->mesh_transforms[++index] = scale[0];
	models->mesh_transforms[++index] = scale[1];
	models->mesh_transforms[++index] = scale[2];

	resize_int_buffer(&models->mesh_positions_counts, (models->models_count + 1));
	resize_int_buffer(&models->mesh_normals_counts, (models->models_count + 1));

	// Move to the start of the file again so we can read it.
	rewind(file);

	// Define offsets to the start of the new mesh in the array.
	int positions_offset = models->positions_count * STRIDE_POSITION;
	int normals_offset = models->normals_count * STRIDE_NORMAL;
	int uvs_offset = models->uvs_count * STRIDE_UV;
	int colours_index = models->colours_count * STRIDE_COLOUR;
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

			// Store the object space position.
			models->object_space_positions[positions_offset++] = v[0];
			models->object_space_positions[positions_offset++] = v[1];
			models->object_space_positions[positions_offset++] = v[2];
		}

		else if (strcmp(tokens[0], "vn") == 0)
		{
			models->object_space_normals[normals_offset++] = (float)atof(tokens[1]);
			models->object_space_normals[normals_offset++] = (float)atof(tokens[2]);
			models->object_space_normals[normals_offset++] = (float)atof(tokens[3]);
		}

		else if (strcmp(tokens[0], "vt") == 0)
		{
			models->uvs[uvs_offset++] = (float)atof(tokens[1]);
			models->uvs[uvs_offset++] = (float)atof(tokens[2]);
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
			models->face_position_indices[faces_positions_offset++] = models->positions_count + face_indices[0];
			models->face_position_indices[faces_positions_offset++] = models->positions_count + face_indices[3];
			models->face_position_indices[faces_positions_offset++] = models->positions_count + face_indices[6];

			models->face_normal_indices[faces_normals_offset++] = models->normals_count + face_indices[2];
			models->face_normal_indices[faces_normals_offset++] = models->normals_count + face_indices[5];
			models->face_normal_indices[faces_normals_offset++] = models->normals_count + face_indices[8];

			//models->face_attribute_indices[faces_positions_offset++] = models->uvs_count + face_indices[7];
			//models->face_attribute_indices[faces_positions_offset++] = models->normals_count + face_indices[8];
			//models->face_attribute_indices[faces_positions_offset++] = models->colours_count + colours_index;


			//u,v,x,y,z,r,g,b,a
			// Vertex 1
			models->face_attributes[faces_attributes_offset++] = models->uvs[models->uvs_count + face_indices[1]];				// U
			models->face_attributes[faces_attributes_offset++] = models->uvs[models->uvs_count + face_indices[1] + 1];			// V

			// TODO: TEMP: Hardcode colour
			models->face_attributes[faces_attributes_offset++] = 1; // R
			models->face_attributes[faces_attributes_offset++] = 1; // G
			models->face_attributes[faces_attributes_offset++] = 1; // B
			models->face_attributes[faces_attributes_offset++] = 1; // A
			
			// Vertex 2
			models->face_attributes[faces_attributes_offset++] = models->uvs[models->uvs_count + face_indices[4]];				// U
			models->face_attributes[faces_attributes_offset++] = models->uvs[models->uvs_count + face_indices[4] + 1];			// V
			
			// TODO: TEMP: Hardcode colour
			models->face_attributes[faces_attributes_offset++] = 1; // R
			models->face_attributes[faces_attributes_offset++] = 1; // G
			models->face_attributes[faces_attributes_offset++] = 1; // B
			models->face_attributes[faces_attributes_offset++] = 1; // A

			// Vertex 2
			models->face_attributes[faces_attributes_offset++] = models->uvs[models->uvs_count + face_indices[7]];				// U
			models->face_attributes[faces_attributes_offset++] = models->uvs[models->uvs_count + face_indices[7] + 1];			// V

			// TODO: TEMP: Hardcode colour
			models->face_attributes[faces_attributes_offset++] = 1; // R
			models->face_attributes[faces_attributes_offset++] = 1; // G
			models->face_attributes[faces_attributes_offset++] = 1; // B
			models->face_attributes[faces_attributes_offset++] = 1; // A
		}
	}

	// Update the buffer lengths.
	models->colours_count += positions_count;
	models->positions_count += positions_count;
	models->normals_count += normals_count;
	models->uvs_count += uvs_count;

	// Flag that the mesh's world space data needs to be calculated.
	models->transforms_updated_flags[models->models_count] = 1;

	models->mesh_positions_counts[models->models_count] = positions_count;
	models->mesh_normals_counts[models->models_count] = normals_count;

	// Update the number of models.
	++models->models_count;

	// Close the file.
	if (fclose(file) != 0)
	{
		// TODO: Error closing.
		log_error("Failed to close file after loading .obj file.");
	}
}
*/

void load_model_base_from_obj(Models* models, const char* filename)
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

	// Read the sizes that we will need to allocate to accomodate for.
	int positions_count, normals_count, uvs_count, face_count;
	parse_obj_counts(file, &positions_count, &uvs_count, &normals_count, &face_count);

	const int mb_index = models->mbs_count;
	const int new_mbs_count = models->mbs_count + 1;

	// Write the number of vertex components in the mesh.
	resize_int_buffer(&models->mbs_positions_counts, new_mbs_count);
	models->mbs_positions_counts[mb_index] = positions_count;

	resize_int_buffer(&models->mbs_normals_counts, new_mbs_count);
	models->mbs_normals_counts[mb_index] = normals_count;

	resize_int_buffer(&models->mbs_faces_counts, new_mbs_count);
	models->mbs_faces_counts[mb_index] = face_count;

	resize_int_buffer(&models->mbs_uvs_counts, new_mbs_count);
	models->mbs_uvs_counts[mb_index] = uvs_count;

	// Resize the offset buffers.
	resize_int_buffer(&models->mbs_positions_offsets, new_mbs_count);
	models->mbs_positions_offsets[mb_index] = models->mbs_total_positions;

	resize_int_buffer(&models->mbs_normals_offsets, new_mbs_count);
	models->mbs_normals_offsets[mb_index] = models->mbs_total_normals;

	resize_int_buffer(&models->mbs_faces_offsets, new_mbs_count);
	models->mbs_faces_offsets[mb_index] = models->mbs_total_faces;

	resize_int_buffer(&models->mbs_uvs_offsets, new_mbs_count);
	models->mbs_uvs_offsets[mb_index] = models->mbs_total_uvs;

	// Resize the indexing buffers.
	const int new_total_faces = models->mbs_total_faces + face_count;
	const int new_total_vertices = new_total_faces * STRIDE_FACE_VERTICES;

	resize_int_buffer(&models->mbs_face_position_indices, new_total_vertices);
	resize_int_buffer(&models->mbs_face_normal_indices, new_total_vertices);
	resize_int_buffer(&models->mbs_face_uvs_indices, new_total_vertices);

	// Resize the actual object space data buffers.
	const int new_total_positions = models->mbs_total_positions + positions_count;
	const int new_total_normals = models->mbs_total_normals + normals_count;
	const int new_total_uvs = models->mbs_total_uvs + uvs_count;

	resize_float_buffer(&models->mbs_object_space_positions, new_total_positions * STRIDE_POSITION);
	resize_float_buffer(&models->mbs_object_space_normals, new_total_normals * STRIDE_NORMAL);
	resize_float_buffer(&models->mbs_uvs, new_total_uvs * STRIDE_UV);

	// Recreate the in/out buffers for clipping if this model base has the most
	// faces yet.
	if (models->max_mb_faces < face_count)
	{
		models->max_mb_faces = face_count;

		resize_float_buffer(&models->temp_clipped_faces_in, face_count * STRIDE_ENTIRE_FACE * (int)pow(2, 1));
		resize_float_buffer(&models->temp_clipped_faces_out, face_count * STRIDE_ENTIRE_FACE * (int)pow(2, 1)); //MAX_FRUSTUM_PLANES);
	}

	// Move to the start of the file again so we can read it.
	rewind(file);

	// Define offsets to the start of the new mesh in the array.
	int positions_offset = models->mbs_total_positions * STRIDE_POSITION;
	int normals_offset = models->mbs_total_normals * STRIDE_NORMAL;
	int uvs_offset = models->mbs_total_uvs * STRIDE_UV;
	int faces_positions_offset = models->mbs_total_faces * STRIDE_FACE_VERTICES;
	int faces_normals_offset = models->mbs_total_faces * STRIDE_FACE_VERTICES;
	int faces_uvs_offset = models->mbs_total_faces * STRIDE_FACE_VERTICES;

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

			// Store the object space position.
			models->mbs_object_space_positions[positions_offset++] = v[0];
			models->mbs_object_space_positions[positions_offset++] = v[1];
			models->mbs_object_space_positions[positions_offset++] = v[2];
		}

		else if (strcmp(tokens[0], "vn") == 0)
		{
			models->mbs_object_space_normals[normals_offset++] = (float)atof(tokens[1]);
			models->mbs_object_space_normals[normals_offset++] = (float)atof(tokens[2]);
			models->mbs_object_space_normals[normals_offset++] = (float)atof(tokens[3]);
		}

		else if (strcmp(tokens[0], "vt") == 0)
		{
			models->mbs_uvs[uvs_offset++] = (float)atof(tokens[1]);
			models->mbs_uvs[uvs_offset++] = (float)atof(tokens[2]);
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

			models->mbs_face_position_indices[faces_positions_offset++] = face_indices[0];
			models->mbs_face_position_indices[faces_positions_offset++] = face_indices[3];
			models->mbs_face_position_indices[faces_positions_offset++] = face_indices[6];

			models->mbs_face_normal_indices[faces_normals_offset++] = face_indices[2];
			models->mbs_face_normal_indices[faces_normals_offset++] = face_indices[5];
			models->mbs_face_normal_indices[faces_normals_offset++] = face_indices[8];

			models->mbs_face_uvs_indices[faces_uvs_offset++] = face_indices[1];
			models->mbs_face_uvs_indices[faces_uvs_offset++] = face_indices[4];
			models->mbs_face_uvs_indices[faces_uvs_offset++] = face_indices[7];
		}
	}

	// Close the file.
	if (fclose(file) != 0)
	{
		// TODO: Error closing.
		log_error("Failed to close file after loading .obj file.");
	}

	// Update totals.
	models->mbs_count = new_mbs_count;
	models->mbs_total_faces = new_total_faces;
	models->mbs_total_positions = new_total_positions;
	models->mbs_total_normals = new_total_normals;
	models->mbs_total_uvs = new_total_uvs;
}

void create_model_instances(Models* models, int mb_index, int n)
{
	if (mb_index > models->mbs_count - 1)
	{
		log_error("mb_index out of range.");
		return;
	}

	const int new_instances_count = models->mis_count + n;

	// Resize buffers
	resize_int_buffer(&models->mis_base_ids, new_instances_count);
	resize_int_buffer(&models->mis_transforms_updated_flags, new_instances_count);
	resize_int_buffer(&models->mis_texture_ids, new_instances_count);
	
	for (int i = models->mis_count; i < new_instances_count; ++i)
	{
		models->mis_base_ids[i] = mb_index;
		models->mis_transforms_updated_flags[i] = 0;

		// TODO: Textures. Should be a parameter?
		models->mis_texture_ids[i] = -69;
	}

	resize_float_buffer(&models->mis_transforms, new_instances_count * STRIDE_MI_TRANSFORM);
	
	// Make space for the bounding sphere, this will be generated in the next
	// render call.
	resize_float_buffer(&models->mis_bounding_spheres, new_instances_count * STRIDE_SPHERE);

	// Get some of the model base data sizes.
	// TODO: HOW DO I GET THE TOTALS?

	// Increase the totals to get the new buffer sizes.
	const int faces_count = models->mbs_faces_counts[mb_index] * n;
	const int new_total_vertices = faces_count * STRIDE_FACE_VERTICES;

	models->mis_total_faces += faces_count;
	models->mis_total_positions += models->mbs_positions_counts[mb_index] * n;
	models->mis_total_normals += models->mbs_normals_counts[mb_index] * n;

	// TODO: Initialise these colours to something?
	//		 Remember, this is actually the diffuse value of the surface?
	//		 Should look into the actual terminology. But essentially how
	//		 much of each RGB component the vertex absorbs.
	resize_float_buffer(&models->mis_vertex_colours, new_total_vertices * STRIDE_COLOUR);

	// Resize buffers used for indexed rendering.
	resize_float_buffer(&models->world_space_positions, models->mis_total_positions * STRIDE_POSITION);
	resize_float_buffer(&models->world_space_normals, models->mis_total_normals * STRIDE_NORMAL);
	resize_float_buffer(&models->view_space_positions, models->mis_total_positions * STRIDE_POSITION);
	resize_float_buffer(&models->view_space_normals, models->mis_total_normals * STRIDE_NORMAL);

	// Resize intermediate/temporary rendering buffers.
	resize_int_buffer(&models->front_faces_counts, new_instances_count);
	resize_float_buffer(&models->front_faces, models->mis_total_faces * STRIDE_ENTIRE_FACE);

	resize_int_buffer(&models->clipped_faces_counts, new_instances_count);
	resize_float_buffer(&models->clipped_faces, models->mis_total_faces * STRIDE_ENTIRE_FACE * (int)pow(2, 1)); // TODO: * pow(2, ENABLED_FRUSTUM_PLANES_COUNT.....)

	// Update the number of instances.
	// TODO: Record how many instances of a ModelBase there are?
	models->mis_count = new_instances_count;
}

void free_models(Models* models)
{
	// Free all mesh buffers.
	// TODO: Why crash here?????
	free(models->mbs_positions_counts);
	free(models->mbs_normals_counts);
	free(models->mbs_faces_counts);
	free(models->mbs_face_position_indices);
	free(models->mbs_face_normal_indices);
	free(models->mbs_object_space_positions);
	free(models->mbs_object_space_normals);
	free(models->mbs_uvs);

	free(models->mbs_faces_offsets);
	free(models->mbs_positions_offsets);
	free(models->mbs_normals_offsets);

	free(models->mis_base_ids);
	free(models->mis_transforms_updated_flags);
	free(models->mis_texture_ids);
	free(models->mis_vertex_colours);
	free(models->mis_transforms);
	free(models->mis_bounding_spheres);

	free(models->world_space_positions);
	free(models->world_space_normals);

	free(models->view_space_positions);
	free(models->view_space_normals);

	free(models->front_faces_counts);
	free(models->front_faces);

	free(models->clipped_faces_counts);
	free(models->clipped_faces);

	free(models->temp_clipped_faces_in);
	free(models->temp_clipped_faces_out);

	free(models->projected_clipped_faces);
	free(models->projected_clipped_faces_temp);
}

#undef _CRT_SECURE_NO_WARNINGS