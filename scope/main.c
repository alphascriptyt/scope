#include "engine/engine.h"

// TODO: Look into switching to an actual c compiler, not sure how to check that its compiled with c or if it even matters.

void engine_on_init(Engine* engine)
{
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
    
    // Load models into the scene
    //load_model_base_from_obj(&scene->models, "C:/Users/olive/source/repos/scope/scope/res/models/cube.obj");
    load_model_base_from_obj(&scene->models, "C:/Users/olive/source/repos/scope/scope/res/models/monkey.obj");
    load_model_base_from_obj(&scene->models, "C:/Users/olive/source/repos/scope/scope/res/models/axis.obj");
    int n0 = 1000;
    
    create_model_instances(&scene->models, 0, n0);

    //create_model_instances(&scene->models, 1, 3);

    log_info("Created model instances.");
    V3 pos = { 0, 0, 0 };
    V3 eulers = { 0,0, 0};
    V3 scale = { 1, 1, 1 };


    
    for (int i = 0; i < n0; ++i)
    {
        int index_transform = i * STRIDE_MI_TRANSFORM;

        scene->models.mis_transforms[index_transform] = pos[0];
        scene->models.mis_transforms[++index_transform] = pos[1];
        scene->models.mis_transforms[++index_transform] = pos[2] - i * 3;
        scene->models.mis_transforms[++index_transform] = eulers[0];
        scene->models.mis_transforms[++index_transform] = eulers[1];
        scene->models.mis_transforms[++index_transform] = eulers[2];
        scene->models.mis_transforms[++index_transform] = scale[0];
        scene->models.mis_transforms[++index_transform] = scale[1];
        scene->models.mis_transforms[++index_transform] = scale[2];
    }
    
    /*
    create_model_instances(&scene->models, 1, 2);




    scene->models.mis_transforms[n0 * STRIDE_MI_TRANSFORM] = 3;
    scene->models.mis_transforms[n0 * STRIDE_MI_TRANSFORM + 1] = 0;
    scene->models.mis_transforms[n0 * STRIDE_MI_TRANSFORM + 2] = -3;
    scene->models.mis_transforms[n0 * STRIDE_MI_TRANSFORM + 3] = 0;
    scene->models.mis_transforms[n0 * STRIDE_MI_TRANSFORM + 4] = 0;
    scene->models.mis_transforms[n0 * STRIDE_MI_TRANSFORM + 5] = 0;
    scene->models.mis_transforms[n0 * STRIDE_MI_TRANSFORM + 6] = 1;
    scene->models.mis_transforms[n0 * STRIDE_MI_TRANSFORM + 7] = 1;
    scene->models.mis_transforms[n0 * STRIDE_MI_TRANSFORM + 8] = 1;

    scene->models.mis_transforms[(n0+1) * STRIDE_MI_TRANSFORM] = 6;
    scene->models.mis_transforms[(n0 + 1) * STRIDE_MI_TRANSFORM + 1] = 0;
    scene->models.mis_transforms[(n0 + 1) * STRIDE_MI_TRANSFORM + 2] = -3;
    scene->models.mis_transforms[(n0 + 1) * STRIDE_MI_TRANSFORM + 3] = 0;
    scene->models.mis_transforms[(n0 + 1) * STRIDE_MI_TRANSFORM + 4] = 0;
    scene->models.mis_transforms[(n0 + 1) * STRIDE_MI_TRANSFORM + 5] = 0;
    scene->models.mis_transforms[(n0 + 1) * STRIDE_MI_TRANSFORM + 6] = 1;
    scene->models.mis_transforms[(n0 + 1) * STRIDE_MI_TRANSFORM + 7] = 2;
    scene->models.mis_transforms[(n0 + 1) * STRIDE_MI_TRANSFORM + 8] = 1;

    scene->models.mis_dirty_transforms_flags[n0] = 1;
    scene->models.mis_dirty_transforms_flags[n0 + 1] = 1;
    
    create_model_instances(&scene->models, 0, 1);
    scene->models.mis_transforms[(n0 + 2) * STRIDE_MI_TRANSFORM] = -3;
    scene->models.mis_transforms[(n0 + 2) * STRIDE_MI_TRANSFORM + 1] = 0;
    scene->models.mis_transforms[(n0 + 2) * STRIDE_MI_TRANSFORM + 2] = -3;
    scene->models.mis_transforms[(n0 + 2) * STRIDE_MI_TRANSFORM + 3] = 0;
    scene->models.mis_transforms[(n0 + 2) * STRIDE_MI_TRANSFORM + 4] = 0;
    scene->models.mis_transforms[(n0 + 2) * STRIDE_MI_TRANSFORM + 5] = 0;
    scene->models.mis_transforms[(n0 + 2) * STRIDE_MI_TRANSFORM + 6] = 3;
    scene->models.mis_transforms[(n0 + 2) * STRIDE_MI_TRANSFORM + 7] = 0.2;
    scene->models.mis_transforms[(n0 + 2) * STRIDE_MI_TRANSFORM + 8] = 0.2;

    // TODO: Function for this?
    
    scene->models.mis_dirty_transforms_flags[n0 + 2] = 1;
    */

}

void engine_on_update(Engine* engine, float dt)
{
    
    Scene* scene = &engine->scenes[engine->current_scene_id];
    
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
    v3_sub_v3_out(pos, engine->renderer.camera.position, dir);
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
