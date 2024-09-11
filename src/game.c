#include "game.h"

#include "entity.h"
#include "player.h"

#include "widget.h"

#include "systems/animation.h"
#include "systems/collision.h"

#include "core/random/generator.h"
#include "core/memory/memory.h"
#include "core/logger/logger.h"
#include "core/string/string.h"
#include "core/math/math_utils.h"
#include "core/utils/utils.h"
#include "core/asset_store/cJSON.h"
#include "core/asset_store/json_loader.h"
#include "core/vulkan_renderer/vulkan_buffer.h"
#include "core/input/input.h"

#include <time.h>
#include <SDL2/SDL.h>
#include <assert.h>
#include <string.h>

#define DELTA_TIME   1.0 / 60.0
#define TILE_COUNT_X 30


static void model_load_materials(gltf_model_t *gltf_model, model_t *model)
{
    model->material_count = gltf_model->material_count;
    model->materials = memory_alloc(model->material_count * sizeof(material_t), MEM_TAG_HEAP);

    for (uint32_t i = 0; i < model->material_count; i++)
    {
        gltf_material_t *gltf_material = &gltf_model->materials[i];
        material_t *material = &model->materials[i];

        material->base_color_factor = gltf_material->pbr_metallic_roughness.base_color_factor;
        material->base_color_texture_index = gltf_material->pbr_metallic_roughness.base_color_texture_index;
    }
}

static void model_load_textures(gltf_model_t *gltf_model, model_t *model, asset_store_t* asset_store, vulkan_renderer_t *renderer)
{
    const char *model_file_name = file_name_wo_extension(gltf_model->path, MEM_TAG_TEMP);

    model->textures      = memory_alloc(sizeof(vulkan_texture_t) * gltf_model->image_count, MEM_TAG_HEAP);
    model->texture_count = gltf_model->image_count;

    for (uint32_t i = 0; i < model->texture_count; i++)
    {
        const char *file_path = gltf_model->image_paths[i];
        const char *asset_id  = format_string(MEM_TAG_PERMANENT,"%s-%u", model_file_name, i);

        asset_store_add_texture(asset_store, renderer, asset_id, file_path);
        model->textures[i] = asset_store_get_texture(asset_store, asset_id);
    }
}

static void load_node(gltf_model_t *gltf_model,
                      model_t *model,
                      gltf_node_t *gltf_node,   //source node
                      model_node_t *model_node, //destination node
                      uint32_t parent,          //parent node index
                      uint32_t node_index)      //current node index
{
    model_node->local_transform = gltf_node->local_transform;
    model_node->parent = parent;
    model_node->index  = node_index;
    model_node->skin   = gltf_node->skin;
    model_node->mesh   = gltf_node->mesh;

    //load children
    for (uint32_t i = 0; i < gltf_node->child_count; i++)
    {
        uint32_t child_index = gltf_node->children[i];
        load_node(gltf_model, model, &gltf_model->nodes[child_index], &model->nodes[child_index], node_index, child_index);
    }
}

static void load_skins(gltf_model_t *gltf_model, model_t *model, vulkan_renderer_t *renderer)
{
    model->skin_count = gltf_model->skin_count;
    model->skins = memory_alloc(sizeof(skin_t) * model->skin_count, MEM_TAG_HEAP);
    
    for (uint32_t i = 0; i < gltf_model->skin_count; i++)
    {
        gltf_skin_t *gltf_skin = &gltf_model->skins[i];
        skin_t *skin = &model->skins[i];

        skin->joint_count = gltf_skin->joint_count;
        skin->joints = memory_alloc(skin->joint_count * sizeof(uint32_t), MEM_TAG_HEAP);
        memmove(skin->joints, gltf_skin->joints, skin->joint_count * sizeof(uint32_t));

        //inverse bind matrices
        if (gltf_skin->inverse_bind_matrices != UINT32_MAX)
        {
            gltf_accessor_t *accessor = &gltf_model->accessors[gltf_skin->inverse_bind_matrices];
            gltf_buffer_view_t *bv    = &gltf_model->buffer_views[accessor->buffer_view];
            gltf_buffer_t      *buf   = &gltf_model->buffers[bv->buffer];

            skin->inverse_bind_matrix_count = accessor->count;
            skin->inverse_bind_matrices     = memory_alloc(sizeof(mat4f_t) * skin->inverse_bind_matrix_count, MEM_TAG_HEAP);
            memcpy(skin->inverse_bind_matrices, &buf->data[accessor->byte_offset + bv->byte_offset], accessor->count * sizeof(mat4f_t));

            // create ssbo
            VkDeviceSize ssbo_size = skin->inverse_bind_matrix_count * sizeof(mat4f_t);
            create_vulkan_buffer(&skin->ssbo, 
                                 renderer, 
                                 ssbo_size, 
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            vkMapMemory(renderer->logical_device, skin->ssbo.memory, 0, ssbo_size, 0, &skin->ssbo.mapped);
        }
    }

}

static void load_animations(gltf_model_t *gltf_model, model_t *model)
{
    model->animation_count = gltf_model->animation_count;
    model->animations = memory_alloc(model->animation_count * sizeof(animation_t), MEM_TAG_HEAP);

    for (uint32_t i = 0; i < model->animation_count; i++)
    {
        gltf_animation_t *gltf_animation = &gltf_model->animations[i];
        animation_t *animation = &model->animations[i];

        //samplers
        animation->sampler_count = gltf_animation->sampler_count;
        animation->samplers = memory_alloc(animation->sampler_count * sizeof(animation_sampler_t), MEM_TAG_HEAP);
        animation->start_time = INFINITY;
        animation->end_time = -INFINITY;

        for (uint32_t j = 0; j < animation->sampler_count; j++)
        {
            gltf_animation_sampler_t *gltf_sampler = &gltf_animation->samplers[j];
            animation_sampler_t *sampler = &animation->samplers[j];

            sampler->interpolation = gltf_sampler->interpolation;

            //sampler keyframe and input time values
            {
                gltf_accessor_t *accessor = &gltf_model->accessors[gltf_sampler->input];
                gltf_buffer_view_t *bv    = &gltf_model->buffer_views[accessor->buffer_view];
                gltf_buffer_t *buffer     = &gltf_model->buffers[bv->buffer];
                const void *data          = &buffer->data[accessor->byte_offset + bv->byte_offset];
                const float *dst          = (float*)data;

                sampler->input_count = accessor->count;
                sampler->inputs      = memory_alloc(sampler->input_count * sizeof(float), MEM_TAG_HEAP);
                memcpy(sampler->inputs, dst, bv->byte_length);

                for (uint32_t k = 0; k < animation->samplers[j].input_count; k++)
                {
                    float input = animation->samplers[j].inputs[k];
                    if (input < animation->start_time)
                    {
                        animation->start_time = input;
                    }
                    if (input > animation->end_time)
                    {
                        animation->end_time = input;
                    }
                }
            }

            //read sampler keyframe output translate/rotate/scale values
            {
                gltf_accessor_t *accessor = &gltf_model->accessors[gltf_sampler->input];
                gltf_buffer_view_t *bv    = &gltf_model->buffer_views[accessor->buffer_view];
                gltf_buffer_t *buffer     = &gltf_model->buffers[bv->buffer];
                const void *data          = &buffer->data[accessor->byte_offset + bv->byte_offset];

                switch(accessor->type)
                {
                    case VEC3:
                    {
                        const vec3f_t *buf = (const vec3f_t *)data;
                        for (uint32_t index = 0; index < accessor->count; index++)
                        {
                            vec3f_t *vec3 = &buf[index];
                            vec4f_t *vec4 = &sampler->outputs[index];
                            vec4->x = vec3->x;
                            vec4->y = vec3->y;
                            vec4->z = vec3->z;
                            vec4->w = 0;
                        }
                        break;
                    }
                    case VEC4:
                    {
                        const vec4f_t *buf = (const vec4f_t*)data;
                        memcpy(sampler->outputs, buf, bv->byte_length);
                        break;
                    }
                }
            }

            //channels
            animation->channel_count = gltf_animation->channel_count;
            animation->channels      = memory_alloc(animation->channel_count * sizeof(animation_t), MEM_TAG_HEAP);

            for (uint32_t k = 0; k < animation->channel_count; k++)
            {
                gltf_channel_t *gltf_channel = &gltf_animation->channels[k];
                animation_channel_t *channel = &animation->channels[k];
                channel->path    = gltf_channel->path;
                channel->sampler = gltf_channel->sampler;
                channel->node    = gltf_channel->node;
            }
        }
    }
}

static void update_joints(model_t *model, model_node_t *node, vulkan_renderer_t *renderer)
{
}

static void setup(game_t *game)
{
    //create fps entity
    gltf_model_t *gltf_model = model_load_from_gltf("./assets/models/cesium_man/cesium_man.gltf");

    model_t model;
    //we have the input, now load images
    model_load_textures(gltf_model, &model, &game->asset_store, &game->renderer);
    model_load_materials(gltf_model, &model);

    //allocate vertex and index buffers
    //calculate total buffer size and number of indices
    uint32_t index_buffer_size = 0;
    size_t index_count = 0;
    uint32_t vertex_buffer_size = 0;
    size_t vertex_count = 0;
    for (uint32_t i = 0; i < gltf_model->mesh_count; i++)
    {
        index_buffer_size += gltf_model->indices_byte_lengths[i];
        index_count += gltf_model->index_counts[i];

        vertex_buffer_size += gltf_model->vertices_byte_lengths[i];
        vertex_count += gltf_model->vertex_counts[i];
    }

    //vertex and index buffers for ALL meshes in the entire models
    uint8_t *vertex_buffer = memory_alloc(vertex_buffer_size, MEM_TAG_TEMP);
    uint8_t *index_buffer  = memory_alloc(index_buffer_size, MEM_TAG_TEMP);
    
    model.node_count = gltf_model->node_count;
    model.nodes = memory_alloc(sizeof(model_node_t) * model.node_count, MEM_TAG_HEAP);

    gltf_scene_t *scene = &gltf_model->scenes[0];
    for (uint32_t i = 0; i < scene->node_count; i++)
    {
        uint32_t node_index = scene->nodes[i];
        load_node(gltf_model, 
                  &model,
                  &gltf_model->nodes[node_index], 
                  &model.nodes[node_index], 
                  UINT32_MAX, 
                  node_index);
    }

    load_skins(gltf_model, &model, &game->renderer);
    load_animations(gltf_model, &model);
    mat4f_t mat = {0};
    uint32_t i = 0;
    for (uint32_t row = 0; row < 4; row++)
    {
        for (uint32_t col = 0; col < 4; col++)
        {
            mat.m[row][col] = i++;
        }
    }

    
    //update_joints
    //do index and vertex buffers
    
#if 0
    entity_t *fps_entity = bulk_data_push_struct(&game->entities);
    fps_entity->id   = game->entity_id++;
    fps_entity->type = ENTITY_TYPE_WIDGET;
    fps_entity->p    = (vec2f_t){25, game->window_height - 50};
    fps_entity->size = (vec2f_t){0,0};//does not matter
    fps_entity->z_index = 5;

    widget_t *widget   = bulk_data_push_struct(&game->widgets);
    widget->entity     = fps_entity; 
    fps_entity->data   = widget;
    widget->text_label = bulk_data_push_struct(&game->text_labels);
    widget->text_label->font = &game->font;
    widget->text_label->offset = (vec2f_t){250,-250};
    widget->text_label->text = NULL;
    widget->text_label->duration = 1.0f;
    widget->text_label->timer    = 0.0f;
    widget->type = WIDGET_TYPE_FPS;
#endif

    game->performance_freq = SDL_GetPerformanceFrequency();
    game->previous_counter = SDL_GetPerformanceCounter();

    game->camera.size.x = game->window_width + game->tile_width;
    game->camera.size.y = game->window_height + game->tile_width;

    game->camera.min.x = 0;
    game->camera.min.y = 0;
    if (game->player_entity)
    {
        game->camera.min.x  = game->player_entity->p.x + game->player_entity->size.x / 2 - game->window_width / 2;
        game->camera.min.y  = game->player_entity->p.y + game->player_entity->size.y / 2 - game->window_height / 2;
    }
}

static void update(game_t *game)
{
    //if quit was requested, quit now
    if (game->input.quit)
    {
        game->is_running = false;
        return;
    }

    memory_begin(MEM_TAG_RENDER);

    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t elapsed = now - game->previous_counter;
    game->previous_counter = now;
    
    double sec = (double)elapsed / game->performance_freq;
    game->accumulator += sec;

    while (game->accumulator > 1.0 / 61.0)
    {
        memory_begin(MEM_TAG_SIM);

        //update all entities
        for (uint32_t i = 0; i < bulk_data_size(&game->entities); i++)
        {
            entity_t *e = bulk_data_getp_null(&game->entities, i);
            if (e)
            {
                switch (e->type)
                {
                    case(ENTITY_TYPE_PLAYER):
                    {
                        update_player(e, &game->input, DELTA_TIME, &game->entities);
                        break;
                    }
                    case(ENTITY_TYPE_WEAPON):
                    {
                        e->p = game->player_entity->p;
                        break;
                    }
                    case(ENTITY_TYPE_WIDGET):
                    {
                        update_widget(e, 1.0/60.0, sec);
                        break;
                    }
                    default: 
                        break;
                }
            }
        }

        //add some text to render

        game->camera.min.x = 0;
        game->camera.min.y = 0;
        if (game->player_entity)
        {
            game->camera.min.x  = game->player_entity->p.x + game->player_entity->size.x / 2 - game->window_width / 2;
            game->camera.min.y  = game->player_entity->p.y + game->player_entity->size.y / 2 - game->window_height / 2;
        }

        game->accumulator -= 1.0 / 59.0;
        if (game->accumulator < 0) game->accumulator = 0.0;
    }
}


static void render(game_t *game)
{
    vulkan_renderer_render(&game->renderer, game);
}

void game_run(game_t *game)
{
    setup(game);
    while(game->is_running)
    {
        process_input(&game->input);
        update(game);
        render(game);
    }
    memory_uninit();
}

void game_init(game_t *game)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        LOGE("initializing SDL %s", SDL_GetError());
        return;
    }

#if defined (DEBUG)
    game->window_width = 1920;
    game->window_height = 1080;
    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_BORDERLESS;
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
    game->tile_width = (float)game->window_width / (float)TILE_COUNT_X;

    srand(time(NULL));
    memory_init();

    asset_store_init(&game->asset_store);
    vulkan_renderer_init(&game->renderer, game->window, game->tile_width, game->tile_width);

    game->entity_id = 0;

    bulk_data_init(&game->entities, entity_t);
    bulk_data_init(&game->tiles, tile_t);
    bulk_data_init(&game->weapons, weapon_t);
    bulk_data_init(&game->animation_chunks, animation_chunk_t);
    bulk_data_init(&game->sprites, sprite_t);
    bulk_data_init(&game->widgets, widget_t);
    bulk_data_init(&game->text_labels, text_label_t);

    game->is_running = true;
}

