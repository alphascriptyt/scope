#include "engine/engine.h"

#include "utils/common.h"

#include "globals.h"

#include "canvas.h"

float* directions;

void engine_on_init(Engine* engine)
{    
    if (STATUS_OK != resources_load_texture(&engine->resources, "C:/Users/olive/source/repos/scope/scope/res/textures/rickreal.bmp"))
    {
        log_error("FAiled to load\n");
    }

    g_draw_normals = 0;

    // Create a scene
    Scene* scene = &engine->scenes[0];
    Status status = scene_init(scene);
    if (STATUS_OK != status)
    {
        log_error("Failed to scene_init because of %s", status_to_str(status));
        return;
    }

    engine->current_scene_id = 0;
    ++engine->scenes_count;
    
    // Setup scene for shadow testing.
    load_model_base_from_obj(&scene->models, "C:/Users/olive/source/repos/scope/scope/res/models/cube.obj");
    load_model_base_from_obj(&scene->models, "C:/Users/olive/source/repos/scope/scope/res/models/suzanne.obj");
    create_model_instances(&scene->models, 0, 3);

    V3 pos0 = { -1, 1, 3 };
    V3 pos1 = { 1, 1, 3 };
    V3 eulers = { 0,0, 0};
    V3 scale = { 0.5, 1, 0.5 };
    mi_set_transform(&scene->models, 0, pos0, eulers, scale);
    mi_set_transform(&scene->models, 1, pos1, eulers, scale);

    V3 plane_pos = { 0, 0, -4 };
    V3 plane_scale = { 5.f, 0.1f, 10.f };
    mi_set_transform(&scene->models, 2, plane_pos, eulers, plane_scale);

    V3 pl_pos0 = { 0, 2, 14 };
    V3 pl_col0 = { 1, 1, 1 };
    point_lights_create(&scene->point_lights, pl_pos0, pl_col0, 50.f);

    engine->renderer.camera.position.z = 20;
}

void engine_on_update(Engine* engine, float dt)
{
 
    Scene* scene = &engine->scenes[engine->current_scene_id];
 
    //scene->point_lights.world_space_positions[2] += dt;

    return;

    /*
    int c = 3;

    int end = scene->models.mis_count * STRIDE_MI_TRANSFORM + c;
    for (int i = c; i < end; i += STRIDE_MI_TRANSFORM)
    {
        if (scene->models.mis_transforms[i] > PI * 2)
        {
            scene->models.mis_transforms[i] = 0;
        }
        else
        {
            scene->models.mis_transforms[i] -= dt;
        }
    }

    if (c > 6)
    {
        for (int i = 0; i < scene->models.mis_count; ++i)
        {
            scene->models.mis_dirty_bounding_sphere_flags[i] = 1;
        }
    }
    */
    
    const int speed = 1;
    for (int i = 0; i < scene->point_lights.count; ++i)
    {
        int j = i * 3;
        scene->point_lights.world_space_positions[j] += directions[j] * dt * speed;
        scene->point_lights.world_space_positions[j + 1] += directions[j + 1] * dt * speed;
        scene->point_lights.world_space_positions[j + 2] += directions[j + 2] * dt * speed;
    }
    


    
    
    /*
    for (int i = 0; i < scene->models.mis_count; ++i)
    {
        int index = i * STRIDE_MI_TRANSFORM + 3;

        scene->models.mis_transforms[index] -= dt;

        if (scene->models.mis_transforms[index] > PI * 2)
        {
            scene->models.mis_transforms[index] = 0;
        }

        scene->models.mis_dirty_ids[scene->models.mis_total_dirty++] = i;

    }*/
        
    
    /*
    // TEMP: Model always facing camera.
    V3 pos;
    v3_init(pos, scene->models.mis_transforms[0], scene->models.mis_transforms[1], scene->models.mis_transforms[2]);

    V3 dir;
    v3_sub_v3_out(pos, engine->renderer.camera.position, dir);ww
    normalise(dir);
    printf("dir: %s\n", v3_to_str(dir));

    float yaw = atan2f(dir[0], dir[2]);
    float pitch = -asinf(dir[1]); // TODO: Why does the pitch need to be inverted here. Interesting.
    // TODO: This is fine for now, but eventually should move to quarternions.   
    
    scene->models.mis_transforms[3] = pitch; 
    scene->models.mis_transforms[4] = yaw;
    scene->models.mis_transforms[5] = 0;

    scene->models.mis_transforms_updated_flags[0] = 1;
    return;
    

    scene->models.mis_transforms[3] -= dt;
        
    if (scene->models.mis_transforms[3] > PI * 2)
    {
        scene->models.mis_transforms[3] = 0;
    }

    scene->models.mis_transforms_updated_flags[0] = 1;*/
}

void engine_on_keyup(Engine* engine, WPARAM wParam)
{
    switch (wParam)
    {
    case VK_F1:
    {
        Scene* scene = &engine->scenes[engine->current_scene_id];

        V3 colour =
        {
            random_float(),
            random_float(),
            random_float()
        };

        point_lights_create(&scene->point_lights, engine->renderer.camera.position, colour, 1);

        if (directions)
        {
             float* temp = realloc(directions, (size_t)scene->point_lights.count * 3 * sizeof(float));
             if (temp)
             {
                 directions = temp;
             }
        }
        else
        {
            directions = malloc((size_t)scene->point_lights.count * 3 * sizeof(float));
        }

        if (!directions)
        {
            printf("!directions.\n");
            return;
        }

        int i = (scene->point_lights.count - 1) * 3;
        directions[i] = engine->renderer.camera.direction.x;
        directions[i + 1] = engine->renderer.camera.direction.y;
        directions[i + 2] = engine->renderer.camera.direction.z;

        break;
    }
    case VK_F2:
    {
        g_draw_normals = !g_draw_normals;
        break;
    }
    case VK_F3:
    {
        Scene* scene = &engine->scenes[engine->current_scene_id];
        engine->renderer.camera.position.x = scene->point_lights.world_space_positions[0];
        engine->renderer.camera.position.y = scene->point_lights.world_space_positions[1];
        engine->renderer.camera.position.z = scene->point_lights.world_space_positions[2];



    }
    }
}

int main()
{
	Engine engine;
	if (STATUS_OK == engine_init(&engine, 1600, 900))
	{
		engine_run(&engine);
	}
	
	engine_destroy(&engine);

	return 0;
}
