#include "game.h"
#include "core/containers/containers.c"
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
#include "core/renderer/frontend/renderer.c"
#include "core/renderer/vulkan_backend/vulkan_backend.c"

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

    for (uint32_t i = 0; i < model->material_count; i++) {
        gltf_material_t *gltf_material = &gltf_model->materials[i];
        material_t *material = &model->materials[i];

        material->base_color_factor = gltf_material->pbr_metallic_roughness.base_color_factor;
        material->base_color_texture_index = gltf_material->pbr_metallic_roughness.base_color_texture_index;
    }
}

static void model_load_textures(gltf_model_t *gltf_model, model_t *model, asset_store_t* asset_store, renderer_t *renderer)
{
    const char *model_file_name = file_name_wo_extension(gltf_model->path, MEM_TAG_TEMP);

    model->textures      = memory_alloc(sizeof(texture_t) * gltf_model->image_count, MEM_TAG_HEAP);
    model->texture_count = gltf_model->image_count;

    for (uint32_t i = 0; i < model->texture_count; i++) {
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
    for (uint32_t i = 0; i < gltf_node->child_count; i++) {
        uint32_t child_index = gltf_node->children[i];
        load_node(gltf_model, model, &gltf_model->nodes[child_index], &model->nodes[child_index], node_index, child_index);
    }
}

static void load_nodes(gltf_model_t *gltf_model, model_t *model)
{
    model->node_count = gltf_model->node_count;
    model->nodes = memory_alloc(sizeof(model_node_t) * model->node_count, MEM_TAG_HEAP);

    gltf_scene_t *scene = &gltf_model->scenes[0];
    for (uint32_t i = 0; i < scene->node_count; i++) {
        uint32_t node_index = scene->nodes[i];
        load_node(gltf_model, 
                  model,
                  &gltf_model->nodes[node_index], 
                  &model->nodes[node_index], 
                  UINT32_MAX, 
                  node_index);
    }
}

static void load_skins(gltf_model_t *gltf_model, model_t *model, renderer_t *renderer)
{
    model->skin_count = gltf_model->skin_count;
    model->skins = memory_alloc(sizeof(skin_t) * model->skin_count, MEM_TAG_HEAP);
    
    for (uint32_t i = 0; i < gltf_model->skin_count; i++) {
        gltf_skin_t *gltf_skin = &gltf_model->skins[i];
        skin_t *skin = &model->skins[i];

        skin->joint_count = gltf_skin->joint_count;
        skin->joints = memory_alloc(skin->joint_count * sizeof(uint32_t), MEM_TAG_HEAP);
        memmove(skin->joints, gltf_skin->joints, skin->joint_count * sizeof(uint32_t));

        //inverse bind matrices
        if (gltf_skin->inverse_bind_matrices != UINT32_MAX) {
            gltf_accessor_t *accessor = &gltf_model->accessors[gltf_skin->inverse_bind_matrices];
            gltf_buffer_view_t *bv    = &gltf_model->buffer_views[accessor->buffer_view];
            gltf_buffer_t      *buf   = &gltf_model->buffers[bv->buffer];

            skin->inverse_bind_matrix_count = accessor->count;
            skin->inverse_bind_matrices     = memory_alloc(sizeof(mat4f_t) * skin->inverse_bind_matrix_count, MEM_TAG_HEAP);
            memcpy(skin->inverse_bind_matrices, &buf->data[accessor->byte_offset + bv->byte_offset], accessor->count * sizeof(mat4f_t));

            VkDeviceSize ssbo_size = skin->inverse_bind_matrix_count * sizeof(mat4f_t);
            renderer_create_renderbuffer(renderer, &skin->ssbo, RENDERBUFFER_TYPE_STORAGE_BUFFER, &renderer->shaders[SHADER_TYPE_SKINNED_GEOMETRY], NULL, ssbo_size);
        }
    }
}

static void load_animations(gltf_model_t *gltf_model, model_t *model)
{
    model->animation_count = gltf_model->animation_count;
    model->animations = memory_alloc(model->animation_count * sizeof(animation_t), MEM_TAG_HEAP);

    for (uint32_t i = 0; i < model->animation_count; i++) {
        gltf_animation_t *gltf_animation = &gltf_model->animations[i];
        animation_t *animation = &model->animations[i];

        //samplers
        animation->sampler_count = gltf_animation->sampler_count;
        animation->samplers = memory_alloc(animation->sampler_count * sizeof(animation_sampler_t), MEM_TAG_HEAP);
        animation->start_time = INFINITY;
        animation->end_time = -INFINITY;

        for (uint32_t j = 0; j < animation->sampler_count; j++) {
            gltf_animation_sampler_t *gltf_sampler = &gltf_animation->samplers[j];
            animation_sampler_t *sampler = &animation->samplers[j];

            sampler->interpolation = gltf_sampler->interpolation;

            //sampler keyframe and input time values
            {
                gltf_accessor_t *accessor = &gltf_model->accessors[gltf_sampler->input];
                gltf_buffer_view_t *bv    = &gltf_model->buffer_views[accessor->buffer_view];
                gltf_buffer_t *buffer     = &gltf_model->buffers[bv->buffer];
                const void *data          = &buffer->data[accessor->byte_offset + bv->byte_offset];
                const float *dst          = (const float*)data;

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
                        for (uint32_t index = 0; index < accessor->count; index++) {
                            const vec3f_t *vec3 = &buf[index];
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

            for (uint32_t k = 0; k < animation->channel_count; k++) {
                gltf_channel_t *gltf_channel = &gltf_animation->channels[k];
                animation_channel_t *channel = &animation->channels[k];
                channel->path    = gltf_channel->path;
                channel->sampler = gltf_channel->sampler;
                channel->node    = gltf_channel->node;
            }
        }
    }
}

static void get_node_matrix(model_t *model, model_node_t *model_node, mat4f_t *out)
{
    mat4f_t node_matrix = model_node->local_transform;
    uint32_t current_parent = model_node->parent;
    while (current_parent != UINT32_MAX) {
        model_node_t *parent_node = &model->nodes[current_parent];
        mat4f_t temp = node_matrix;
        mat4_multiply(&parent_node->local_transform, &temp, &node_matrix);
        current_parent = parent_node->parent;
    }
    *out = node_matrix;
}

static void update_joints(model_t *model, renderer_t *renderer)
{
    for (uint32_t i = 0; i < model->node_count; i++) {
        model_node_t *root = &model->nodes[i];
        if (root->skin != UINT32_MAX) {
            mat4f_t local_transform = {0};
            get_node_matrix(model, root, &local_transform);

            mat4f_t inverse_transform = {0};
            mat4_inverse(&local_transform, &inverse_transform);

            skin_t *skin = &model->skins[root->skin];
            uint32_t joint_count    = skin->joint_count;
            mat4f_t *joint_matrices = memory_alloc(joint_count * sizeof(mat4f_t), MEM_TAG_TEMP);

            for (uint32_t j = 0; j < joint_count; j++) {
                model_node_t *joint = &model->nodes[skin->joints[j]];

                mat4f_t joint_mat = {0};
                get_node_matrix(model, joint, &joint_mat);

                mat4f_t temp = mat4_identity();
                mat4_multiply(&joint_mat, &skin->inverse_bind_matrices[j], &temp);
                mat4_multiply(&inverse_transform, &temp, &joint_matrices[j]);
            }
            renderer_copy_to_renderbuffer(renderer, &skin->ssbo, joint_matrices, joint_count * sizeof(mat4f_t));
        }
    }
}

static void load_meshes(gltf_model_t *gltf_model, 
                        model_t      *model,
                        renderer_t *renderer)
{
    model->mesh_count = gltf_model->mesh_count;
    model->meshes = memory_alloc(model->mesh_count * sizeof(model->mesh_count), MEM_TAG_HEAP);
    for (uint32_t i = 0; i < model->mesh_count; i++) {
        model->meshes[i].primitive_count = gltf_model->meshes[i].primitive_count;
    }
    //allocate vertex and index buffers
    //calculate total buffer size and number of indices
    uint32_t index_buffer_size  = 0;
    size_t index_count          = 0;
    uint32_t vertex_buffer_size = 0;
    size_t vertex_count         = 0;

    for (uint32_t i = 0; i < gltf_model->mesh_count; i++) {
        index_buffer_size += gltf_model->indices_byte_lengths[i];
        index_count += gltf_model->index_counts[i];

        vertex_buffer_size += gltf_model->vertices_byte_lengths[i];
        vertex_count += gltf_model->vertex_counts[i];
    }

    //vertex and index buffers for ALL meshes in the entire models
    skinned_vertex_t *vertex_buffer = (skinned_vertex_t*)memory_alloc(vertex_buffer_size, MEM_TAG_TEMP);
    uint32_t *index_buffer  = (uint32_t*)memory_alloc(index_buffer_size, MEM_TAG_TEMP);
    
    index_count  = 0;
    vertex_count = 0;

    for (uint32_t i = 0; i < model->mesh_count; i++) {
        gltf_mesh_t *gltf_mesh = &gltf_model->meshes[i];
        mesh_t *mesh = &model->meshes[i];

        for (uint32_t j = 0; j < mesh->primitive_count; j++) {
            gltf_mesh_primitive_t *gltf_primitive = &gltf_mesh->primitives[j];
            primitive_t *primitive = &mesh->primitives[j];

            uint32_t first_index      = index_count;
            uint32_t vertex_start     = vertex_count;
            uint32_t prim_index_count = 0;
            bool has_skin = false;

            {
                const float *position_buffer         = NULL;
                const float *normals_buffer          = NULL;
                const float *tex_coords_buffer       = NULL;
                const uint16_t *joint_indices_buffer = NULL;
                const float *joint_weights_buffer    = NULL;
                uint32_t prim_vertex_count           = 0; 

                if (gltf_primitive->position != UINT32_MAX) {
                    gltf_accessor_t *accessor = &gltf_model->accessors[gltf_primitive->position];
                    gltf_buffer_view_t *bv    = &gltf_model->buffer_views[accessor->buffer_view];
                    position_buffer = (const float *)(&gltf_model->buffers[bv->buffer].data[accessor->byte_offset + bv->byte_offset]);
                    prim_vertex_count = accessor->count;
                }

                if (gltf_primitive->normal != UINT32_MAX) {
                    gltf_accessor_t *accessor = &gltf_model->accessors[gltf_primitive->normal];
                    gltf_buffer_view_t *bv    = &gltf_model->buffer_views[accessor->buffer_view];
                    normals_buffer = (const float *)(&gltf_model->buffers[bv->buffer].data[accessor->byte_offset + bv->byte_offset]);
                }

                if (gltf_primitive->tex_coord != UINT32_MAX) {
                    gltf_accessor_t *accessor = &gltf_model->accessors[gltf_primitive->tex_coord];
                    gltf_buffer_view_t *bv    = &gltf_model->buffer_views[accessor->buffer_view];
                    tex_coords_buffer = (const float *)(&gltf_model->buffers[bv->buffer].data[accessor->byte_offset + bv->byte_offset]);
                }

                if (gltf_primitive->joints != UINT32_MAX) {
                    gltf_accessor_t *accessor = &gltf_model->accessors[gltf_primitive->joints];
                    gltf_buffer_view_t *bv    = &gltf_model->buffer_views[accessor->buffer_view];
                    joint_indices_buffer = (const uint16_t *)(&gltf_model->buffers[bv->buffer].data[accessor->byte_offset + bv->byte_offset]);
                }

                if (gltf_primitive->weights != UINT32_MAX) {
                    gltf_accessor_t *accessor = &gltf_model->accessors[gltf_primitive->joints];
                    gltf_buffer_view_t *bv    = &gltf_model->buffer_views[accessor->buffer_view];
                    joint_weights_buffer      = (const float *)(&gltf_model->buffers[bv->buffer].data[accessor->byte_offset + bv->byte_offset]);
                }

                has_skin = (joint_indices_buffer && joint_weights_buffer);

                for (uint32_t v = 0; v < prim_vertex_count; v++) {
                    skinned_vertex_t *vert = &vertex_buffer[vertex_count++];
                    //positions
                    vert->pos    = (vec3f_t){position_buffer[v * 3], position_buffer[v * 3 + 1], position_buffer[v * 3 + 2]};
                    //normals
                    vec3f_t normal = {0};
                    if (normals_buffer) {
                        normal = vec3_normalize((vec3f_t){normals_buffer[v * 3], normals_buffer[v * 3 + 1], normals_buffer[v * 3 + 2]});
                    }
                    vert->normal =  normal;
                    //tex coords
                    vec2f_t tex_coords = {0};
                    if (tex_coords_buffer) {
                        tex_coords = vec2_normalize((vec2f_t){tex_coords_buffer[v * 2], tex_coords_buffer[v * 2 + 1]});
                    }
                    vert->uv = tex_coords;
                    //joint indices
                    vec4i_t joint_indices = {0};
                    if (has_skin){
                        joint_indices = (vec4i_t){joint_indices_buffer[v * 4], joint_indices_buffer[v * 4 + 1], joint_indices_buffer[v * 4 + 2], joint_indices_buffer[v * 4 +3]};
                    }
                    vert->joint_indices = joint_indices;
                    //joint weights
                    vec4f_t joint_weights = {0};
                    if (has_skin){
                        joint_weights = (vec4f_t){joint_weights_buffer[v * 4], joint_weights_buffer[v * 4 + 1], joint_weights_buffer[v * 4 + 2], joint_weights_buffer[v * 4 + 3]};
                    }
                    vert->joint_weights = joint_weights;
                }
            }

            //indices
            {
                gltf_accessor_t *accessor = &gltf_model->accessors[gltf_primitive->indices];
                gltf_buffer_view_t *bv    = &gltf_model->buffer_views[accessor->buffer_view];
                gltf_buffer_t *buffer     = &gltf_model->buffers[bv->buffer];
                
                prim_index_count = accessor->count;
                
                switch (accessor->component_type)
                {
                    case UNSIGNED_BYTE:
                    {
                        const uint8_t *buf = (const uint8_t *)(&buffer->data[accessor->byte_offset + bv->byte_offset]);
                        for (uint32_t index = 0; index < accessor->count; index++) {
                            index_buffer[prim_index_count++] = buf[index] + vertex_start;
                        }
                        break;
                    }

                    case UNSIGNED_SHORT:
                    {
                        const uint16_t *buf = (const uint16_t *)(&buffer->data[accessor->byte_offset + bv->byte_offset]);
                        for (uint32_t index = 0; index < accessor->count; index++){
                            index_buffer[prim_index_count++] = buf[index] + vertex_start;
                        }
                        break;
                    }

                    case UNSIGNED_INT:
                    {
                        const uint32_t *buf = (const uint32_t *)(&buffer->data[accessor->byte_offset + bv->byte_offset]);
                        for (uint32_t index = 0; index < accessor->count; index++){
                            index_buffer[prim_index_count++] = buf[index] + vertex_start;
                        }
                        break;
                    }
                    default:
                        assert(false);
                        break;
                }
            }

            primitive->first_index = first_index;
            primitive->index_count = prim_index_count;
            primitive->material    = gltf_primitive->material;
            index_count += prim_index_count;
        }
    }

    renderer_create_renderbuffer(renderer, &model->vertex_buffer, RENDERBUFFER_TYPE_VERTEX_BUFFER, &renderer->shaders[SHADER_TYPE_SKINNED_GEOMETRY],(uint8_t *)vertex_buffer, vertex_buffer_size);
    renderer_create_renderbuffer(renderer, &model->index_buffer, RENDERBUFFER_TYPE_INDEX_BUFFER, &renderer->shaders[SHADER_TYPE_SKINNED_GEOMETRY], (uint8_t*)index_buffer, index_buffer_size);
}


static void setup(game_t *game)
{
    //create fps entity
    gltf_model_t *gltf_model = model_load_from_gltf("./assets/models/cesium_man/cesium_man.gltf");

    model_t model;
    model_load_textures(gltf_model, &model, &game->asset_store, &game->renderer);
    model_load_materials(gltf_model, &model);
    load_nodes(gltf_model, &model);
    load_skins(gltf_model, &model, &game->renderer);
    load_animations(gltf_model, &model);
    update_joints(&model, &game->renderer);
    load_meshes(gltf_model, &model, &game->renderer);

    game->performance_freq = SDL_GetPerformanceFrequency();
    game->previous_counter = SDL_GetPerformanceCounter();

    game->camera.size.x = game->window_width + game->tile_width;
    game->camera.size.y = game->window_height + game->tile_width;

    game->camera.min.x = 0;
    game->camera.min.y = 0;
    if (game->player_entity) {
        game->camera.min.x  = game->player_entity->p.x + game->player_entity->size.x / 2 - game->window_width / 2;
        game->camera.min.y  = game->player_entity->p.y + game->player_entity->size.y / 2 - game->window_height / 2;
    }
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

    while (game->accumulator > 1.0 / 61.0) {
        memory_begin(MEM_TAG_SIM);

        //update all entities
        for (uint32_t i = 0; i < bulk_data_size(&game->entities); i++) {

            entity_t *e = bulk_data_getp_null(&game->entities, i);

            if (e) {

                switch (e->type)
                {
                    case(ENTITY_TYPE_PLAYER):
                        update_player(e, &game->input, DELTA_TIME, &game->entities);
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

        //add some text to render

        game->camera.min.x = 0;
        game->camera.min.y = 0;
        if (game->player_entity) {
            game->camera.min.x  = game->player_entity->p.x + game->player_entity->size.x / 2 - game->window_width / 2;
            game->camera.min.y  = game->player_entity->p.y + game->player_entity->size.y / 2 - game->window_height / 2;
        }

        game->accumulator -= 1.0 / 59.0;
        if (game->accumulator < 0) game->accumulator = 0.0;
    }
}


static void render(game_t *game)
{
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
    game->window_width = 1920;
    game->window_height = 1080;
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
    game->tile_width = (float)game->window_width / (float)TILE_COUNT_X;

    srand(time(NULL));
    memory_init();

    asset_store_init(&game->asset_store);
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
    renderer_create_shader(&game->renderer, SHADER_TYPE_SKINNED_GEOMETRY);

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

