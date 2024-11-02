#include "skinned_model.h"
#include "asset_types.h"

static void model_load_materials(gltf_model_t *gltf_model, skinned_model_t *model)
{
    assert(gltf_model->material_count == 1 && "model has multiple materials");
    gltf_material_t *gltf_material = &gltf_model->materials[0];
    material_t *material = &model->material;

    material->base_color_factor = gltf_material->pbr_metallic_roughness.base_color_factor;
    material->base_color_texture_index = gltf_material->pbr_metallic_roughness.base_color_texture_index;
}

static void model_load_textures(gltf_model_t *gltf_model, skinned_model_t *model, renderer_t *renderer, bulk_data_texture_t *textures)
{
    model->texture_count = gltf_model->texture_count;
    assert(gltf_model->texture_count <= MAX_TEXTURES_PER_MODEL);

    for (uint32_t i = 0; i < gltf_model->texture_count; i++) {
        model->textures[i] = bulk_data_allocate_slot_texture_t(textures);
        texture_t *texture = bulk_data_getp_null_texture_t(textures, model->textures[i]);
        renderer_create_texture(renderer, texture, gltf_model->image_paths[i]);
    }
}

static void assign_parent_to_node(model_node_t *model_node, 
                                  skinned_model_t *model, 
                                  gltf_node_t *gltf_nodes, 
                                  uint32_t parent, 
                                  uint32_t node_index)
{
    model_node->parent = parent;

    gltf_node_t *gltf_node = &gltf_nodes[node_index];
    for (uint32_t i = 0; i < gltf_node->child_count; i++)
    {
        uint32_t child_index = gltf_node->children[i];
        assign_parent_to_node(&model->nodes[child_index], model, gltf_nodes, node_index, child_index);
    }
}

static void model_load_nodes(gltf_model_t *gltf_model, skinned_model_t *model)
{
    model->node_count = gltf_model->node_count;
    assert(model->node_count <= MAX_NODES_PER_MODEL);

    for (uint32_t i = 0; i < gltf_model->node_count; i++) {
        model_node_t *node = &model->nodes[i];
        gltf_node_t *gltf_node = &gltf_model->nodes[i];

        node->local_transform = gltf_node->local_transform;
        node->index = i;
        node->skin = gltf_node->skin;
        node->mesh = gltf_node->mesh;
        node->translation  = gltf_node->translation;
        node->scale = gltf_node->scale;
        node->rotation = gltf_node->rotation;
    }

    //! now do the parent->child relationship
    model_node_t *root = &model->nodes[0];
    assign_parent_to_node(root, model, gltf_model->nodes, UINT32_MAX, 0);
}

static void model_load_skins(gltf_model_t *gltf_model, skinned_model_t *model, renderer_t *renderer, bulk_data_renderbuffer_t *renderbuffers)
{
    assert((gltf_model->skin_count == 1) && "gltf model has multiple skins");

    gltf_skin_t *gltf_skin = &gltf_model->skins[0];
    skin_t *skin = &model->skin;
    memset(skin, 0, sizeof(*skin));

    skin->joint_count = gltf_skin->joint_count;
    assert(skin->joint_count <= MAX_BONES_PER_SKIN);
    memmove(skin->joints, gltf_skin->joints, skin->joint_count);
    skin->skeleton_root = gltf_skin->skeleton;

    //inverse bind matrices
    if (gltf_skin->inverse_bind_matrices != UINT32_MAX) {
        gltf_accessor_t *accessor = &gltf_model->accessors[gltf_skin->inverse_bind_matrices];
        gltf_buffer_view_t *bv    = &gltf_model->buffer_views[accessor->buffer_view];
        gltf_buffer_t      *buf   = &gltf_model->buffers[bv->buffer];

        memmove(skin->inverse_bind_matrices, &buf->data[accessor->byte_offset + bv->byte_offset], accessor->count * sizeof(mat4f_t));

        VkDeviceSize ssbo_size = skin->joint_count * sizeof(mat4f_t);
        skin->ssbo = bulk_data_allocate_slot_renderbuffer_t(renderbuffers);

        renderbuffer_t *ssbo = bulk_data_getp_null_renderbuffer_t(renderbuffers, skin->ssbo);
        renderer_create_renderbuffer(renderer, ssbo, RENDERBUFFER_TYPE_STORAGE_BUFFER, NULL, ssbo_size);
    }
}

static void model_load_animations(gltf_model_t *gltf_model, skinned_model_t *model)
{
    model->animation_count = gltf_model->animation_count; 
    assert(gltf_model->animation_count <= MAX_ANIMATIONS_PER_MODEL && "model has too many animations");

    for (uint32_t i = 0; i < model->animation_count; i++) {
        gltf_animation_t *gltf_animation = &gltf_model->animations[i];

        animation_t *animation = &model->animations[i];

        animation->sampler_count = gltf_animation->sampler_count;
        animation->channel_count = gltf_animation->channel_count;
        animation->samplers = memory_alloc(animation->sampler_count * sizeof(animation_sampler_t), MEM_TAG_HEAP);
        animation->channels = memory_alloc(animation->channel_count * sizeof(animation_channel_t), MEM_TAG_HEAP);

        animation->start_time = INFINITY;
        animation->end_time = -INFINITY;

        //samplers
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
                memcpy(sampler->inputs, dst, sampler->input_count * sizeof(float));

                for (uint32_t k = 0; k < sampler->input_count; k++)
                {
                    float input = sampler->inputs[k];
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
                gltf_accessor_t *accessor = &gltf_model->accessors[gltf_sampler->output];
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
                        memcpy(sampler->outputs, buf, accessor->count * sizeof(vec4f_t));
                        break;
                    }
                }
            }
        }

        //channels
        for (uint32_t k = 0; k < animation->channel_count; k++) {
            gltf_channel_t *gltf_channel = &gltf_animation->channels[k];
            animation_channel_t *channel = &animation->channels[k];

            channel->path    = gltf_channel->path;
            channel->sampler = gltf_channel->sampler;
            channel->node    = gltf_channel->node;
        }
    }
}

static void get_node_matrix(skinned_model_t *model, model_node_t *model_node, mat4f_t *out)
{
    mat4f_t node_matrix = model_node->local_transform;
    transform_from_TRS(&node_matrix, model_node->translation, model_node->rotation, model_node->scale);

    uint32_t current_parent = model_node->parent;
    while (current_parent != UINT32_MAX) {
        model_node_t *parent_node = &model->nodes[current_parent];
        mat4f_t parent_mat = parent_node->local_transform;
        transform_from_TRS(&parent_mat, parent_node->translation, parent_node->rotation, parent_node->scale);

        mat4f_t temp = node_matrix;
        mat4_multiply(&temp, &parent_mat, &node_matrix);
        current_parent = parent_node->parent;
    }
    *out = node_matrix;
}

static void update_joints(skinned_model_t *model, model_node_t *node, renderer_t *renderer, bulk_data_renderbuffer_t *renderbuffers)
{
    if (node->skin != UINT32_MAX) {
        mat4f_t local_transform = {0};
        get_node_matrix(model, node, &local_transform);

        mat4f_t inverse_transform = {0};
        mat4_inverse(&local_transform, &inverse_transform);

        skin_t *skin = &model->skin;
        uint32_t joint_count    = skin->joint_count;

        mat4f_t *joint_matrices = memory_alloc(joint_count * sizeof(mat4f_t), MEM_TAG_TEMP);

        for (uint32_t i = 0; i < joint_count; i++) {
            model_node_t *joint = &model->nodes[skin->joints[i]];

            mat4f_t joint_mat = {0};
            get_node_matrix(model, joint, &joint_mat);

            mat4f_t temp = {0};
            mat4_multiply(&skin->inverse_bind_matrices[i], &joint_mat, &temp);
            mat4_multiply(&temp, &inverse_transform, &joint_matrices[i]);
        }

        renderbuffer_t *ssbo = bulk_data_getp_null_renderbuffer_t(renderbuffers, skin->ssbo);
        renderer_copy_to_renderbuffer(renderer, ssbo, joint_matrices, joint_count * sizeof(mat4f_t));
    }
}

static void model_load_meshes(gltf_model_t         *gltf_model, 
                              skinned_model_t      *model,
                              renderer_t           *renderer)
{
    //allocate vertex and index buffers
    //calculate total buffer size and number of indices
    size_t index_count          = 0;
    uint32_t vertex_buffer_size = 0;
    size_t vertex_count         = 0;

    for (uint32_t i = 0; i < gltf_model->mesh_count; i++) {
        
        index_count += gltf_model->index_counts[i];
        vertex_count += gltf_model->vertex_counts[i];
    }

    //vertex and index buffers for ALL meshes in the entire models
    skinned_vertex_t *vertex_buffer = (skinned_vertex_t*)memory_alloc(vertex_count * sizeof(skinned_vertex_t), MEM_TAG_TEMP);
    uint32_t *index_buffer  = (uint32_t*)memory_alloc(index_count * sizeof(uint32_t), MEM_TAG_TEMP);//we will just use 32 bit indices
    
    index_count  = 0;
    vertex_count = 0;

    for (uint32_t i = 0; i < gltf_model->mesh_count; i++) {
        gltf_mesh_t *gltf_mesh = &gltf_model->meshes[i];
        mesh_t *mesh = &model->mesh;

        for (uint32_t j = 0; j < gltf_mesh->primitive_count; j++) {
            gltf_mesh_primitive_t *gltf_primitive = &gltf_mesh->primitives[j];

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
                    gltf_accessor_t *accessor = &gltf_model->accessors[gltf_primitive->weights];
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
                        tex_coords = (vec2f_t){tex_coords_buffer[v * 2], tex_coords_buffer[v * 2 + 1]};
                    }
                    vert->uv = tex_coords;
                    //joint indices
                    vec4f_t joint_indices = {0};
                    if (has_skin){
                        joint_indices = (vec4f_t){joint_indices_buffer[v * 4], joint_indices_buffer[v * 4 + 1], joint_indices_buffer[v * 4 + 2], joint_indices_buffer[v * 4 +3]};
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
                            index_buffer[index_count++] = buf[index] + vertex_start;
                        }
                        break;
                    }

                    case UNSIGNED_SHORT:
                    {
                        const uint16_t *buf = (const uint16_t *)(&buffer->data[accessor->byte_offset + bv->byte_offset]);
                        for (uint32_t index = 0; index < accessor->count; index++){
                            index_buffer[index_count++] = buf[index] + vertex_start;
                        }
                        break;
                    }

                    case UNSIGNED_INT:
                    {
                        const uint32_t *buf = (const uint32_t *)(&buffer->data[accessor->byte_offset + bv->byte_offset]);
                        for (uint32_t index = 0; index < accessor->count; index++){
                            index_buffer[index_count++] = buf[index] + vertex_start;
                        }
                        break;
                    }
                    default:
                        assert(false);
                        break;
                }
            }

            mesh->first_index = first_index;
            mesh->index_count = prim_index_count;
            mesh->material    = gltf_primitive->material;
        }
    }

    renderer_create_renderbuffer(renderer, &model->vertex_buffer, RENDERBUFFER_TYPE_VERTEX_BUFFER,(uint8_t *)vertex_buffer, vertex_count * sizeof(skinned_vertex_t));
    renderer_create_renderbuffer(renderer, &model->index_buffer, RENDERBUFFER_TYPE_INDEX_BUFFER, (uint8_t*)index_buffer, index_count * sizeof(uint32_t));
}

bool create_skinned_model(skinned_model_t *skinned_model, 
                          const char *file_path, 
                          const char *asset_id, 
                          renderer_t *renderer, 
                          asset_store_t *asset_store,
                          bulk_data_renderbuffer_t *renderbuffers)
{
    gltf_model_t *gltf_model = model_load_from_gltf(file_path, asset_id);
    if (!gltf_model){
        LOGE("Unable to load gltf file: %s", file_path);
        return false;
    }

    model_load_textures(gltf_model, skinned_model, renderer, asset_store->textures);
    model_load_materials(gltf_model, skinned_model);
    model_load_nodes(gltf_model, skinned_model);
    model_load_skins(gltf_model, skinned_model, renderer, renderbuffers);
    model_load_animations(gltf_model, skinned_model);

    for (uint32_t i = 0; i < skinned_model->node_count; i++) {
        update_joints(skinned_model, &skinned_model->nodes[i], renderer, renderbuffers);
    }

    model_load_meshes(gltf_model, skinned_model, renderer);
    
    //! @TODO: Need to think about models with multiple meshes and materials.
    renderbuffer_t *ssbo = bulk_data_getp_null_renderbuffer_t(renderbuffers, skinned_model->skin.ssbo);
    render_data_config_t config = {0};
    config.type = RENDER_DATA_SKINNED_MODEL;
    config.buffer_count       = ssbo->buffer_count;
    for (uint32_t i = 0; i < config.buffer_count; i++) {
        config.buffer_indices[i] = ssbo->buffers[i];
    }
    config.texture_count      = 1;
    config.texture_indices[0] = skinned_model->textures[0];

    skinned_model->rendering_data = renderer_create_render_data(renderer, &config);
    skinned_model->active_animation = 0;
    return true;
}
