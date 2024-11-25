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
    load_model_base_from_obj(&scene->models, "C:/Users/olive/source/repos/scope/scope/res/models/axis.obj");
    int n0 = 1;
    create_model_instances(&scene->models, 0, n0);

    log_info("Created model instances.");
    V3 pos = { 0, 0, -3 };

    V3 eulers1 = { 0,0, 0};

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
    return;
    Scene* scene = &engine->scenes[engine->current_scene_id];

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
