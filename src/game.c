#include "core/containers/bulk_data.inc"
#include "game.h"
#include "entity.c"
#include "player.c"
#include "widget.c"

#include "systems/animation.c"
#include "systems/collision.c"

#include "core/random/generator.c"
#include "core/memory/memory.c"
#include "core/logger/logger.h"
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

static void skinned_model_update_animation(skinned_model_t *model, renderer_t *renderer, bulk_data_renderbuffer_t *bd, float dt)
{
    if (model->active_animation > model->animation_count - 1) {
        LOGE("No animation with index %u", model->active_animation);
    }
    animation_t *animation = &model->animations[model->active_animation];
    animation->current_time += dt;
    //!NOTE: looping
    if (animation->current_time > animation->end_time) {
        animation->current_time -= animation->end_time;
    }

    for (uint32_t i = 0 ; i < animation->channel_count; i++) {
        animation_channel_t *channel = &animation->channels[i];
        animation_sampler_t *sampler = &animation->samplers[channel->sampler];

        for (uint32_t j = 0; j < sampler->input_count - 1; j++) {
            //! TODO: fix this at some point 
            if (sampler->interpolation != LINEAR_INTERPOLATION) {
                LOGE("We only support linear interpolation!");
                continue;
            }

            if ((animation->current_time >= sampler->inputs[j]) && (animation->current_time <= sampler->inputs[j + 1])) {
                float t = (animation->current_time - sampler->inputs[j]) / (sampler->inputs[j + 1] - sampler->inputs[j]);
                model_node_t *node = &model->nodes[channel->node];
                if (channel->path == TRANSLATION) {
                    vec4f_t trans = vec4_lerp(sampler->outputs[j], sampler->outputs[j + 1], t);
                    node->translation.x = trans.x;
                    node->translation.y = trans.y;
                    node->translation.z = trans.z;
                } else if(channel->path == ROTATION) {
                    quat_t q1 = {};
                    q1.x = sampler->outputs[j].x;
                    q1.y = sampler->outputs[j].y;
                    q1.z = sampler->outputs[j].z;
                    q1.w = sampler->outputs[j].w;

                    quat_t q2 = {};
                    q2.x = sampler->outputs[j + 1].x;
                    q2.y = sampler->outputs[j + 1].y;
                    q2.z = sampler->outputs[j + 1].z;
                    q2.w = sampler->outputs[j + 1].w;

                    node->rotation = quat_normalize(quat_lerp(q1, q2, t));
                } else if (channel->path == SCALE) {
                    vec4f_t scale = vec4_lerp(sampler->outputs[j], sampler->outputs[j + 1], t);
                    node->scale = (vec3f_t){scale.x, scale.y, scale.z};
                }
            }
        }
    }
#if 0
    for (uint32_t i = 0; i < model->node_count; i++) {
        model_node_t *node = &model->nodes[i];
        update_joints(model, node, renderer, bd);
    }
#endif
}

static void skinned_model_draw(skinned_model_t *model, 
                               renderer_t *renderer, 
                               shader_t *shader, 
                               bulk_data_renderbuffer_t *buffers, 
                               bulk_data_texture_t *textures)
{
    renderer_bind_vertex_buffers(renderer, &model->vertex_buffer);
    renderer_bind_index_buffers(renderer, &model->index_buffer);

    for (uint32_t i = 0; i < model->node_count; i++) {
        model_node_t *node = &model->nodes[i]; 
        if (node->mesh != UINT32_MAX) {
            mat4f_t node_matrix = {0};
            get_node_matrix(model, node, &node_matrix);
            update_joints(model, node, renderer, buffers);

            renderer_push_constants(renderer, shader, &node_matrix, sizeof(mat4f_t), 0, SHADER_STAGE_VERTEX);
            renderer_shader_bind_resource(renderer, SHADER_TYPE_SKINNED_GEOMETRY, RENDER_DATA_SKINNED_MODEL, model->rendering_data);
            renderer_draw_indexed(renderer, 0, model->mesh.first_index, model->mesh.index_count, 0, 1);
        }
    }
}

static void setup(game_t *game)
{
    //add all the resources
    asset_store_add_skinned_model(&game->asset_store, 
                                  &game->renderer, 
                                  "cesium-man", 
                                  "./assets/models/cesium_man/cesium_man.gltf",
                                  &game->bulk_data.renderbuffers);
    
    uint32_t index = asset_store_get_asset_index(&game->asset_store, "cesium-man", ASSET_TYPE_SKINNED_MODEL);
    game->skinned_model_index = index;
    
    skinned_model_t *model = asset_store_get_asset_ptr_null(&game->asset_store, "cesium-man", ASSET_TYPE_SKINNED_MODEL);
    skinned_model_update_animation(model, &game->renderer, &game->bulk_data.renderbuffers, DELTA_TIME);

    //renderer fetch shader resources
    shader_resource_list_t resources = {0};
    resources.globals_data     = game->renderer.uniform_buffer_render_data;
    resources.instance_count   = 1;
    resources.instance_data = memory_alloc(sizeof(void*) * resources.instance_count, MEM_TAG_TEMP);
    resources.instance_data[0] = model->rendering_data;

    renderer_initialize_shader(&game->renderer, SHADER_TYPE_SKINNED_GEOMETRY, &resources);

    //init camera
    game->renderer.camera.position = (vec3f_t){0.0f, 0.5f, 2.0f};
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
        skinned_model_update_animation(model, &game->renderer, &game->bulk_data.renderbuffers, DELTA_TIME);
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
    skinned_model_draw(model, 
                       &game->renderer, 
                       &game->renderer.shaders[SHADER_TYPE_SKINNED_GEOMETRY], 
                       &game->bulk_data.renderbuffers, 
                       &game->bulk_data.textures);

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
    bulk_data_init_tile_t(&game->bulk_data.tiles);
    bulk_data_init_weapon_t(&game->bulk_data.weapons);
    bulk_data_init_widget_t(&game->bulk_data.widgets);
    bulk_data_init_renderbuffer_t(&game->bulk_data.renderbuffers);
    bulk_data_init_texture_t(&game->bulk_data.textures);
    bulk_data_init_skinned_model_t(&game->bulk_data.skinned_models);

    asset_store_init(&game->asset_store, &game->bulk_data.textures, &game->bulk_data.skinned_models);

    memset(&game->renderer, 0, sizeof(game->renderer));

    renderer_config_t renderer_config = {0};
#if defined (DEBUG)
    renderer_config.flags |= RENDERER_CONFIG_FLAG_ENABLE_VALIDATION;
#endif

    window_t window = {0};
    window.window = game->window;
    window.width = 0;
    window.height = 0;

    renderer_initialize(&game->renderer, &window, renderer_config);
    renderer_create_shader(&game->renderer, &game->bulk_data.renderbuffers, SHADER_TYPE_SKINNED_GEOMETRY);
    game->entity_id = 0;

    game->is_running = true;
}

