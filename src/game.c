#include <bulk_data.h>
#include <logger.h>
#include <game.h>
#include "entity.c"
#include "player.c"
#include "widget.c"

#include "systems/animation.c"
#include "systems/collision.c"

#include "core/random/generator.c"
#include "core/memory/memory.c"
#include "core/string/string.c"
#include "core/math/math_utils.c"
#include "core/utils/utils.c"
#include "core/asset_store/asset_store.c"
#include "core/input/input.c"
#include "core/renderer/frontend/camera.c"
#include "core/renderer/frontend/renderer.c"
#include "core/renderer/vulkan_backend/vulkan_backend.c"

#include <time.h>
#include <SDL2/SDL.h>
#include <assert.h>
#include <string.h>

#define DELTA_TIME   1.0 / 60.0


static void setup(game_t *game)
{
    //add all the resources
#if 1
    const char *asset_id = "heraklios";
    const char *file_path = "./assets/models/heraklios/Heraklios.gltf";

    asset_store_add_skinned_model(&game->asset_store, 
                                  &game->renderer, 
                                  asset_id,
                                  file_path,
                                  &game->bulk_data.renderbuffers);


    uint32_t index = asset_store_get_asset_index(&game->asset_store, asset_id, ASSET_TYPE_SKINNED_MODEL);
    game->skinned_model_index = index;
    
    skinned_model_t *model = asset_store_get_asset_ptr_null(&game->asset_store, asset_id, ASSET_TYPE_SKINNED_MODEL);
    skinned_model_update_animation(model, &game->renderer, DELTA_TIME);
    
    //renderer fetch shader resources
    shader_resource_list_t resources = {0};
    resources.globals_data     = game->renderer.uniform_buffer_render_data;
    resources.instance_count   = 1;
    resources.instance_data = memory_alloc(sizeof(void*) * resources.instance_count, MEM_TAG_TEMP);
    resources.instance_data[0] = model->rendering_data;
    renderer_initialize_shader(&game->renderer, SHADER_TYPE_SKINNED_GEOMETRY, &resources);

#endif
    //init camera
    game->renderer.camera.position = (vec3f_t){0.0f, 120.0f, 150.0f};
    game->renderer.camera.front    = (vec3f_t){0.0f, 0.0f, -1.0f};
    game->renderer.camera.up       = (vec3f_t){0.0f, 1.0f, 0.0f};
    game->renderer.camera.fov      = PI / 2.0f;
    game->renderer.camera.yaw      = 0.0f;
    game->renderer.camera.pitch    = 0.0f;
    game->renderer.camera.zfar     = 256.0f;
    game->renderer.camera.znear    = 0.1f;
    game->renderer.camera.aspect   = (float)game->window_width / (float)game->window_height;

    game->performance_freq = SDL_GetPerformanceFrequency();
    game->previous_counter = SDL_GetPerformanceCounter();
}

static void update(game_t *game)
{
    //if quit was requested, quit now
    if (game->input.quit) {
        game->is_running = false;
        return;
    }

    memory_begin(MEM_TAG_RENDER);

    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t elapsed = now - game->previous_counter;
    game->previous_counter = now;
    
    double sec = (double)elapsed / game->performance_freq;
    game->accumulator += sec;
#if 1
    while (game->accumulator > 1.0 / 61.0) {
#endif
        memory_begin(MEM_TAG_SIM);

        skinned_model_t *model = bulk_data_getp_null_skinned_model_t(&game->bulk_data.skinned_models, game->skinned_model_index);
        skinned_model_update_animation(model, &game->renderer, DELTA_TIME);
        //update all entities
        for (uint32_t i = 0; i < game->bulk_data.entities.count; i++) {

            entity_t *e = bulk_data_getp_null_entity_t(&game->bulk_data.entities, i);

            if (e) {

                switch (e->type)
                {
                    case(ENTITY_TYPE_PLAYER):
                        update_player(e, &game->input, DELTA_TIME, &game->bulk_data.entities);
                        break;
                    case(ENTITY_TYPE_WEAPON):
                        e->p = game->player_entity->p;
                        break;
                    case(ENTITY_TYPE_WIDGET):
                        update_widget(e, 1.0/60.0, sec);
                        break;
                    default: 
                        break;
                }
            }
        }

        game->accumulator -= 1.0 / 59.0;
        if (game->accumulator < 0) game->accumulator = 0.0;
#if 1
    }
#endif
}

static void render(game_t *game)
{
    //we'll come back to this
    renderer_frame_prepare(&game->renderer, NULL); 

    //prepare all resources to render i.e. update uniform buffers
    scene_uniform_data_t scene_uniforms = {0};
    scene_uniforms.light_pos = (vec4f_t){3.0f, 3.0f, 3.0f, 1.0f};
    camera_get_projection(&game->renderer.camera, &scene_uniforms.projection);
    scene_uniforms.projection.m[1][1] *= -1;
    camera_get_view_matrix(&game->renderer.camera, &scene_uniforms.view);
    renderbuffer_t *scene_uniform_buffer = bulk_data_getp_null_renderbuffer_t(&game->bulk_data.renderbuffers, game->renderer.uniform_buffer_index);
    renderer_copy_to_renderbuffer(&game->renderer, 
                                  scene_uniform_buffer, 
                                  &scene_uniforms, 
                                  sizeof(scene_uniforms));

    //begin rendering
    renderer_begin_rendering(&game->renderer);
    //use shader
    renderer_use_shader(&game->renderer, SHADER_TYPE_SKINNED_GEOMETRY);    

    //set viewport,scissor
    renderer_set_viewport(&game->renderer, game->window_width, game->window_height, 0.0f, 1.0f);
    renderer_set_scissor(&game->renderer, 0, 0, game->window_width, game->window_height);

    //bind resources to render
    //bind scene uniforms
    renderer_shader_bind_resource(&game->renderer, SHADER_TYPE_SKINNED_GEOMETRY, RENDER_DATA_SCENE_UNIFORMS, game->renderer.uniform_buffer_render_data);

    //draw calls
    //...
    skinned_model_t *model = bulk_data_getp_null_skinned_model_t(&game->bulk_data.skinned_models, game->skinned_model_index);
    skinned_model_draw(model, &game->renderer, &game->renderer.shaders[SHADER_TYPE_SKINNED_GEOMETRY]);

    renderer_end_rendering(&game->renderer);
    renderer_frame_submit(&game->renderer, NULL); 
}

void game_run(game_t *game)
{
    setup(game);
    while(game->is_running) {
        process_input(&game->input);
        update(game);
        render(game);
    }

    memory_uninit();
}

void game_init(game_t *game)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOGE("initializing SDL %s", SDL_GetError());
        return;
    }

#if defined (DEBUG) 
    game->window_width = 1280;
    game->window_height = 720;
    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN;
#else

    SDL_DisplayMode dm = {};
    SDL_GetCurrentDisplayMode(0, &dm);
    game->window_width = dm.w;
    game->window_height = dm.h;
    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN;
#endif

    game->window = SDL_CreateWindow(NULL,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        game->window_width,
        game->window_height,
        flags
    );

    assert(game->window);

    game->is_running = false;

    srand(time(NULL));
    memory_init();

    bulk_data_init_entity_t(&game->bulk_data.entities);
    bulk_data_init_weapon_t(&game->bulk_data.weapons);
    bulk_data_init_widget_t(&game->bulk_data.widgets);
    bulk_data_init_renderbuffer_t(&game->bulk_data.renderbuffers);
    bulk_data_init_texture_t(&game->bulk_data.textures);
    bulk_data_init_skinned_model_t(&game->bulk_data.skinned_models);

    asset_store_init(&game->asset_store, &game->bulk_data.textures, &game->bulk_data.skinned_models);

    memset(&game->renderer, 0, sizeof(game->renderer));

    window_t window = {0};
    window.window = game->window;
    window.width = 0;
    window.height = 0;

    renderer_config_t renderer_config = {0};
#if defined (DEBUG)
    renderer_config.flags |= RENDERER_CONFIG_FLAG_ENABLE_VALIDATION;
#endif
    renderer_config.vulkan_buffers = &game->bulk_data.vulkan_buffers;
    renderer_config.vulkan_textures = &game->bulk_data.vulkan_textures;

    renderer_initialize(&game->renderer, &window, renderer_config, &game->bulk_data.renderbuffers, &game->bulk_data.textures);
    renderer_create_shader(&game->renderer, SHADER_TYPE_SKINNED_GEOMETRY);
    game->entity_id = 0;

    game->is_running = true;
}


