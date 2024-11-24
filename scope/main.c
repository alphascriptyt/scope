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
        return status;
    }

    engine->current_scene_id = 0;
    ++engine->scenes_count;
    
    // Load models into the scene
    load_model_base_from_obj(&scene->models, "C:/Users/olive/source/repos/scope/scope/res/models/suzanne.obj");
    int n0 = 100;
    create_model_instances(&scene->models, 0, n0);

    log_info("Created model instances.");
    V3 pos = { 0, 0, -3 };
    V3 pos1 = { 0, 0, -10 };

    // TODO: this is currently PITCH, YAW, ROLL. This doesn't make much sense calling this eulers.
    V3 eulers = { 0, 0,  0 }; // I think I'd rather have the option to set pitch,yaw,roll. 
    V3 eulers1 = { 0.5, 0,  0 }; // I think I'd rather have the option to set pitch,yaw,roll. 

    // TODO: Rename everything from orientation to eulers as thats what it is
    // TODO: Helper function to convert pitch,yaw,roll to direction and vice versa?
    V3 scale = { 1, 1, 1 };
    V3 plane_scale = { 10, 0.1f, 10 };
    V3 scale1 = { 4, 4, 4 };

    for (int i = 0; i < n0; ++i)
    {
        int index_transform = i * STRIDE_MI_TRANSFORM;

        scene->models.mis_transforms[index_transform] = pos[0];
        scene->models.mis_transforms[++index_transform] = pos[1];
        scene->models.mis_transforms[++index_transform] = pos[2] - i * 3;
        scene->models.mis_transforms[++index_transform] = eulers1[0];
        scene->models.mis_transforms[++index_transform] = eulers1[1];
        scene->models.mis_transforms[++index_transform] = eulers1[2];
        scene->models.mis_transforms[++index_transform] = scale[0];
        scene->models.mis_transforms[++index_transform] = scale[1];
        scene->models.mis_transforms[++index_transform] = scale[2];

        scene->models.mis_transforms_updated_flags[i] = 1;
    }
}

void engine_on_update(Engine* engine, float dt)
{
    Scene* scene = &engine->scenes[engine->current_scene_id];

    
    scene->models.mis_transforms[4] += dt;
        
    if (scene->models.mis_transforms[4] > PI * 2)
    {
        scene->models.mis_transforms[4] = 0;
    }

    scene->models.mis_transforms_updated_flags[0] = 1;
    
    
    
    

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
