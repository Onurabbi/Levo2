#include <game.h>
#include <logger.h>
#include <string_utils.h>
#include <math_utils.h>
#include <utils.h>

#include <json_loader.h>
#include <asset_types.h>
#include "cJSON.c"

#include <assert.h>

entity_state_t get_animation_state_from_string(const char *state_string)
{
    if (strcmp(state_string, "idle") == 0) return ENTITY_STATE_IDLE;
    if (strcmp(state_string, "run") == 0) return ENTITY_STATE_RUN;
    if (strcmp(state_string, "melee") == 0) return ENTITY_STATE_MELEE;
    if (strcmp(state_string, "ranged") == 0) return ENTITY_STATE_RANGED;
    if (strcmp(state_string, "hit") == 0) return ENTITY_STATE_HIT;
    if (strcmp(state_string, "dead") == 0) return ENTITY_STATE_DEAD;
    return ENTITY_STATE_MAX;
}

static uint32_t path_from_string(const char *path)
{
    if (strcmp(path, "translation") == 0) return TRANSLATION;
    else if (strcmp(path, "rotation") == 0) return ROTATION;
    else if (strcmp(path, "scale") == 0) return SCALE;
    else if (strcmp(path, "weights") == 0) return WEIGHTS;
    return NIL;
}

static uint32_t interpolation_from_string(const char *interpolation)
{
    if (strcmp(interpolation, "STEP") == 0) return STEP_INTERPOLATION;
    else if (strcmp(interpolation, "LINEAR") == 0) return LINEAR_INTERPOLATION;
    else if (strcmp(interpolation, "SPHERICAL_LINEAR") == 0) return SPHERICAL_LINEAR_INTERPOLATION;
    else if (strcmp(interpolation, "CUBIC_SPLINE") == 0) return CUBIC_SPLINE_INTERPOLATION;
    return NIL;
}

static uint32_t value_type_from_string(const char *str)
{
    if (strcmp(str, "SCALAR") == 0) return SCALAR;
    else if (strcmp(str, "VEC2") == 0) return VEC2;
    else if (strcmp(str, "VEC3") == 0) return VEC3;
    else if (strcmp(str, "VEC4") == 0) return VEC4;
    else if (strcmp(str, "MAT2") == 0) return MAT2;
    else if (strcmp(str, "MAT3") == 0) return MAT3;
    else if (strcmp(str, "MAT4") == 0) return MAT4;
    return 0xFF;
}

static uint32_t alpha_mode_from_string(const char *str)
{
    if (!str)return OPAQUE;
    if (strcmp(str, "OPAQUE") == 0) return OPAQUE;
    else if (strcmp(str, "MASK") == 0) return MASK;
    else if (strcmp(str, "BLEND") == 0) return BLEND;
    else return NIL;
}

#if 0
static uint32_t comp_count_from_type(uint32_t type)
{
    uint32_t count = 0;
    switch(type)
    {
        default:
            break;
        case SCALAR:
            count = 1;
            break;
        case VEC2:
            count = 2;
            break;
        case VEC3:
            count = 3;
            break;
        case VEC4:
        case MAT2:
            count = 4;
            break;
        case MAT3:
            count = 9;
            break;
        case MAT4:
            count = 16;
            break;
    }
    return count;
}

static size_t size_from_type(uint32_t comp_type, uint32_t type)
{
    size_t size;

    switch(comp_type)
    {
        default: 
            size = 0;
            break;
        case SIGNED_BYTE:
        case UNSIGNED_BYTE:
            size = sizeof(int8_t);
            break;
        case UNSIGNED_SHORT:
        case SIGNED_SHORT:
            size = sizeof(int16_t);
            break;
        case UNSIGNED_INT:
            size = sizeof(uint32_t);
            break;
        case FLOAT:
            size = sizeof(float);
            break;
    }
    uint32_t comp_count = comp_count_from_type(type);
    size *= comp_count;
    return size;
}

typedef struct 
{
    primitive_t *primitives;
    uint32_t     primitive_count;
}mesh_t;

typedef struct
{
    model_node_t *nodes;
    uint32_t      node_count;


}model_t;
#endif

static inline float json_get_float_default(cJSON *node, const char *str, float def)
{
    float result = def;

    cJSON *child = cJSON_GetObjectItem(node, str);
    if (child) {
        double num = cJSON_GetNumberValue(child);
        if (num == NAN) {
            result = def;
        } else {
            result = (float)num;
        }
    }
    return result;
}

static inline uint32_t json_get_uint32_default(cJSON *node, const char *str, uint32_t def)
{
    uint32_t result = def;

    cJSON *child = cJSON_GetObjectItem(node, str);
    if (child) {
        double num = cJSON_GetNumberValue(child);
        if (num == NAN) {
            result = def;
        } else {
            result = (uint32_t)num;
        }
    }
    return result;
}

gltf_model_t *model_load_from_gltf(const char * path, const char *asset_id)
{
    long buf_size;
    uint8_t *buf = read_whole_file(path, &buf_size, MEM_TAG_TEMP);
    if (!buf) {
        LOGE("Can't read gltf file %s", path);
        return NULL;
    }

    cJSON* root = cJSON_Parse((const char*)buf);
    if (!root) {
        LOGE("Unable to parse json file %s", path);
        return NULL;
    }

    gltf_model_t *gltf_model = memory_alloc(sizeof(gltf_model_t), MEM_TAG_TEMP);
    if (!gltf_model) {
        LOGE("Failed to allocate model memory");
        return NULL;
    }
    
    gltf_model->path = string_duplicate(path, MEM_TAG_TEMP);
    gltf_model->asset_id = string_duplicate(asset_id, MEM_TAG_TEMP);
    const char *asset_directory = string_get_file_directory(gltf_model->path, MEM_TAG_TEMP);

    //do scenes
    cJSON *scenes_node = cJSON_GetObjectItem(root, "scenes");
    gltf_model->scene_count = cJSON_GetArraySize(scenes_node);
    gltf_model->scenes = memory_alloc(sizeof(gltf_scene_t) * gltf_model->scene_count, MEM_TAG_TEMP);

    for  (uint32_t i = 0; i < gltf_model->scene_count; i++) {
        cJSON *scene_node = cJSON_GetArrayItem(scenes_node, i);
        cJSON *nodes_node = cJSON_GetObjectItem(scene_node, "nodes");
        int node_count = cJSON_GetArraySize(nodes_node);

        gltf_scene_t *scene = &gltf_model->scenes[i];
        scene->node_count = node_count;
        scene->nodes = memory_alloc(sizeof(uint32_t) * scene->node_count, MEM_TAG_TEMP);

        for (uint32_t j = 0; j < scene->node_count; j++) {
            scene->nodes[j] = cJSON_GetNumberValue(cJSON_GetArrayItem(nodes_node, j));
        }
    }

    //do nodes 
    cJSON *nodes = cJSON_GetObjectItem(root, "nodes");
    if (!nodes) {
        LOGE("Unable to find \"nodes\" node. Aborting");
        goto exit;
    }

    gltf_model->node_count = cJSON_GetArraySize(nodes);
    gltf_model->nodes = memory_alloc(gltf_model->node_count * sizeof(gltf_node_t), MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->node_count; i++) {
        cJSON* node = cJSON_GetArrayItem(nodes, i);
        gltf_node_t *model_node = &gltf_model->nodes[i];
        
        //children
        cJSON *children = cJSON_GetObjectItem(node, "children");
        model_node->child_count = cJSON_GetArraySize(children);
        assert(model_node->child_count <= MODEL_NODE_CHILDREN_COUNT);

        for (int j = 0; j < model_node->child_count; j++) {
            model_node->children[j] = (uint8_t)cJSON_GetNumberValue(cJSON_GetArrayItem(children, j));
        }

        //name
        const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(node, "name"));
        model_node->name = string_duplicate(name, MEM_TAG_TEMP);

        //default t,r,s and local transform
        model_node->translation = (vec3f_t){0.0f, 0.0f, 0.0f};
        model_node->scale       = (vec3f_t){1.0f, 1.0f, 1.0f};
        model_node->rotation    = (quat_t){0.0f, 0.0f, 0.0f, 0.0f};
        model_node->local_transform = mat4_identity();

        //matrix
        cJSON *matrix = cJSON_GetObjectItem(node, "matrix");
        if (matrix) {
            int j = 0;
            //copy the matrix directly (column major order)
            for (uint32_t row = 0; row < 4; row++) {
                for (uint32_t col = 0; col < 4; col++) {
                    model_node->local_transform.m[row][col] = (float)cJSON_GetNumberValue(cJSON_GetArrayItem(matrix, j++));
                }
            }
        } else {
            //construct the matrix from TRS
            cJSON *trans = cJSON_GetObjectItem(node, "translation");
            if (trans) {
                model_node->translation.x = cJSON_GetNumberValue(cJSON_GetArrayItem(trans, 0));
                model_node->translation.y = cJSON_GetNumberValue(cJSON_GetArrayItem(trans, 1));
                model_node->translation.z = cJSON_GetNumberValue(cJSON_GetArrayItem(trans, 2));
            }

            cJSON *rot = cJSON_GetObjectItem(node, "rotation");
            if (rot) {
                model_node->rotation.x = cJSON_GetNumberValue(cJSON_GetArrayItem(rot, 0));
                model_node->rotation.y = cJSON_GetNumberValue(cJSON_GetArrayItem(rot, 1));
                model_node->rotation.z = cJSON_GetNumberValue(cJSON_GetArrayItem(rot, 2));
                model_node->rotation.w = cJSON_GetNumberValue(cJSON_GetArrayItem(rot, 3));
            } 

            cJSON *scale = cJSON_GetObjectItem(node, "scale");
            if (scale) {
                model_node->scale.x = cJSON_GetNumberValue(cJSON_GetArrayItem(scale, 0));
                model_node->scale.y = cJSON_GetNumberValue(cJSON_GetArrayItem(scale, 1));
                model_node->scale.z = cJSON_GetNumberValue(cJSON_GetArrayItem(scale, 2));
            }
        }

        //skin
        model_node->skin = json_get_uint32_default(node, "skin", UINT32_MAX);
        if (model_node->skin != UINT32_MAX) {
            LOGI("Mode node %s has skin", model_node->name);
        }
        //mesh
        model_node->mesh = json_get_uint32_default(node, "mesh", UINT32_MAX);
    }
    //we need to load the actual data here

    //cache buffers
    cJSON *buffers_node       = cJSON_GetObjectItem(root, "buffers");
    gltf_model->buffer_count  = (uint32_t)cJSON_GetArraySize(buffers_node);
    gltf_model->buffers       = memory_alloc(gltf_model->buffer_count * sizeof(gltf_buffer_t), MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->buffer_count; i++) {
        cJSON* buffer_node      = cJSON_GetArrayItem(buffers_node, i);
        gltf_buffer_t *buffer   = &gltf_model->buffers[i];
        const char *uri = cJSON_GetStringValue(cJSON_GetObjectItem(buffer_node, "uri"));
        const char *binary_path = string_concatenate(asset_directory, uri, MEM_TAG_TEMP);
        buffer->size = json_get_uint32_default(buffer_node, "byteLength", 0);
        long size;
        buffer->data = read_whole_file(binary_path, &size, MEM_TAG_TEMP);
    }

    //cache accessors
    cJSON* accessors_node      = cJSON_GetObjectItem(root, "accessors");
    gltf_model->accessor_count = cJSON_GetArraySize(accessors_node);
    gltf_model->accessors      = memory_alloc(gltf_model->accessor_count * sizeof(gltf_accessor_t), MEM_TAG_TEMP);
    
    for (uint32_t i = 0; i < gltf_model->accessor_count; i++) {
        cJSON *accessor_node = cJSON_GetArrayItem(accessors_node, i);

        gltf_accessor_t *accessor = &gltf_model->accessors[i];
        accessor->buffer_view = json_get_uint32_default(accessor_node, "bufferView", UINT32_MAX);
        accessor->byte_offset = json_get_uint32_default(accessor_node, "byteOffset", 0);
        accessor->component_type = json_get_uint32_default(accessor_node, "componentType", UINT32_MAX);
        accessor->type = value_type_from_string(cJSON_GetStringValue(cJSON_GetObjectItem(accessor_node, "type")));
        accessor->count = json_get_uint32_default(accessor_node, "count", 0);
    }

    //cache bufferviews 
    cJSON* buffer_views_node      = cJSON_GetObjectItem(root, "bufferViews");
    gltf_model->buffer_view_count = cJSON_GetArraySize(buffer_views_node);
    gltf_model->buffer_views      = memory_alloc(gltf_model->buffer_view_count * sizeof(gltf_buffer_view_t), MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->buffer_view_count; i++) {
        cJSON *buffer_view_node = cJSON_GetArrayItem(buffer_views_node, i);

        gltf_buffer_view_t *buffer_view = &gltf_model->buffer_views[i];
        buffer_view->buffer      = json_get_uint32_default(buffer_view_node, "buffer", UINT32_MAX);
        buffer_view->byte_offset = json_get_uint32_default(buffer_view_node, "byteOffset", 0);
        buffer_view->byte_length = json_get_uint32_default(buffer_view_node, "byteLength", UINT32_MAX);
        buffer_view->byte_stride = json_get_uint32_default(buffer_view_node, "byteStride", UINT32_MAX);
        buffer_view->target      = json_get_uint32_default(buffer_view_node, "target", UINT32_MAX);
    }
    
    //load meshes
    cJSON *meshes_node = cJSON_GetObjectItem(root, "meshes");
    gltf_model->mesh_count = cJSON_GetArraySize(meshes_node);
    gltf_model->meshes = memory_alloc(sizeof(gltf_mesh_t) * gltf_model->mesh_count, MEM_TAG_TEMP);

    for (int i = 0; i < gltf_model->mesh_count; i++) {
        cJSON *mesh_node = cJSON_GetArrayItem(meshes_node, i);
        gltf_mesh_t *mesh = &gltf_model->meshes[i];

        const char *mesh_name = cJSON_GetStringValue(cJSON_GetObjectItem(mesh_node, "name"));
        mesh->name = string_duplicate(mesh_name, MEM_TAG_TEMP);

        cJSON *primitives_node = cJSON_GetObjectItem(mesh_node, "primitives");
        mesh->primitive_count = cJSON_GetArraySize(primitives_node);
        assert(mesh->primitive_count <= 4);

        for (int j = 0; j < mesh->primitive_count; j++) {
            cJSON *primitive_node = cJSON_GetArrayItem(primitives_node, j);

            cJSON *attributes_node        = cJSON_GetObjectItem(primitive_node, "attributes");
            mesh->primitives[j].joints    = json_get_uint32_default(attributes_node, "JOINTS_0",UINT32_MAX);
            mesh->primitives[j].normal    = json_get_uint32_default(attributes_node, "NORMAL", UINT32_MAX);
            mesh->primitives[j].tex_coord = json_get_uint32_default(attributes_node, "TEXCOORD_0", UINT32_MAX);
            mesh->primitives[j].position  = json_get_uint32_default(attributes_node, "POSITION", UINT32_MAX);
            mesh->primitives[j].weights   = json_get_uint32_default(attributes_node, "WEIGHTS_0", UINT32_MAX);

            //if this is undefined it means this is an unindexed geometry
            mesh->primitives[j].indices  = json_get_uint32_default(primitive_node, "indices",UINT32_MAX);
            //triangles default
            mesh->primitives[j].mode     = json_get_uint32_default(primitive_node, "mode", 4);

            mesh->primitives[j].material = json_get_uint32_default(primitive_node, "material", UINT32_MAX); 
        }
    }

    gltf_model->index_counts  = memory_alloc(sizeof(uint32_t) * gltf_model->mesh_count, MEM_TAG_TEMP);
    gltf_model->vertex_counts = memory_alloc(sizeof(uint32_t) * gltf_model->mesh_count, MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->mesh_count; i++) {
        for (uint32_t j = 0; j < gltf_model->meshes[i].primitive_count; j++) {
            uint32_t accessor_index = gltf_model->meshes[i].primitives[j].position;
            gltf_model->vertex_counts[i] += gltf_model->accessors[accessor_index].count;
            
            //indices
            accessor_index = gltf_model->meshes[i].primitives[j].indices;
            gltf_model->index_counts[i] += gltf_model->accessors[accessor_index].count;
        }
    }
    
    //load animations
    cJSON *animations_node = cJSON_GetObjectItem(root, "animations");
    if (animations_node) {
        gltf_model->animation_count = cJSON_GetArraySize(animations_node);

        gltf_model->animations = memory_alloc(sizeof(gltf_animation_t) * gltf_model->animation_count, MEM_TAG_TEMP);
        for (int32_t i = 0; i < gltf_model->animation_count; i++) {
            gltf_animation_t *animation = &gltf_model->animations[i];

            cJSON *animation_node = cJSON_GetArrayItem(animations_node, i);
            cJSON *channels_node = cJSON_GetObjectItem(animation_node, "channels");
            animation->channel_count = (uint32_t)cJSON_GetArraySize(channels_node);
            animation->channels = memory_alloc(sizeof(gltf_channel_t) * animation->channel_count, MEM_TAG_TEMP);

            for (uint32_t j = 0; j < animation->channel_count; j++) {
                cJSON *channel_node = cJSON_GetArrayItem(channels_node, j);
                //this must exist
                cJSON *target_node  = cJSON_GetObjectItem(channel_node, "target");

                gltf_channel_t *channel = &animation->channels[j];

                //this must exist
                channel->sampler = json_get_uint32_default(channel_node, "sampler", UINT32_MAX);

                //this is required
                const char *path = cJSON_GetStringValue(cJSON_GetObjectItem(target_node, "path"));
                channel->path    = path_from_string(path);

                channel->node    = json_get_uint32_default(target_node, "node", UINT32_MAX);
            }

            cJSON *samplers_node = cJSON_GetObjectItem(animation_node, "samplers");
            if (samplers_node) {
                animation->sampler_count = (uint32_t)cJSON_GetArraySize(samplers_node);
                animation->samplers = memory_alloc(sizeof(gltf_animation_sampler_t) * animation->sampler_count, MEM_TAG_TEMP);

                for (uint32_t j = 0; j < animation->sampler_count; j++) {
                    cJSON *sampler_node = cJSON_GetArrayItem(samplers_node, j);
                    gltf_animation_sampler_t *sampler = &animation->samplers[j];

                    sampler->input = json_get_uint32_default(sampler_node, "input", UINT32_MAX);
                    sampler->output = json_get_uint32_default(sampler_node, "output", UINT32_MAX);
                    cJSON *interpolation_node = cJSON_GetObjectItem(sampler_node, "interpolation");
                    const char *interpolation = interpolation_node ? (cJSON_GetStringValue(interpolation_node)) : "LINEAR";
                    sampler->interpolation = interpolation_from_string(interpolation);
                }
            }
        }
    }

    //skins
    cJSON* skins_node         = cJSON_GetObjectItem(root, "skins");
    gltf_model->skin_count    = cJSON_GetArraySize(skins_node);
    gltf_model->skins         = memory_alloc(sizeof(gltf_skin_t) * gltf_model->skin_count, MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->skin_count; i++) {
        cJSON *skin_node       = cJSON_GetArrayItem(skins_node, i);

        gltf_skin_t *gltf_skin = &gltf_model->skins[i];
        gltf_skin->inverse_bind_matrices = json_get_uint32_default(skin_node, "inverseBindMatrices", UINT32_MAX);
        gltf_skin->skeleton = json_get_uint32_default(skin_node, "skeleton", UINT32_MAX);

        cJSON *joints_node = cJSON_GetObjectItem(skin_node, "joints");
        gltf_skin->joint_count = cJSON_GetArraySize(joints_node);
        gltf_skin->joints = memory_alloc(gltf_skin->joint_count * sizeof(uint8_t), MEM_TAG_TEMP);

        for (uint32_t j = 0; j < gltf_skin->joint_count; j++) {
            gltf_skin->joints[j] = (uint32_t)cJSON_GetNumberValue(cJSON_GetArrayItem(joints_node, j));
        }
        const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(skin_node, "name"));
        gltf_skin->name = string_duplicate(name, MEM_TAG_TEMP);
    }
    
    //materials
    cJSON *materials_node = cJSON_GetObjectItem(root, "materials");
    gltf_model->material_count = cJSON_GetArraySize(materials_node);
    gltf_model->materials = memory_alloc(sizeof(gltf_material_t) * gltf_model->material_count, MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->material_count; i++) {
        cJSON* material_node = cJSON_GetArrayItem(materials_node, i);
        gltf_material_t *material = &gltf_model->materials[i];

        //name
        const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(material_node, "name"));
        material->name = string_duplicate(name, MEM_TAG_TEMP);

        //extension
        //do extensions
        cJSON *extensions_node = cJSON_GetObjectItem(material_node, "extensions");
        if (extensions_node) {
            //! NOTE: https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_specular/README.md
            cJSON *specular_node = cJSON_GetObjectItem(extensions_node, "KHR_materials_specular");
            if (specular_node) {
                material->specular = memory_alloc(sizeof(gltf_specular_t), MEM_TAG_TEMP);
                //specular factor
                material->specular->specular_factor = json_get_uint32_default(specular_node, "specularFactor", 1.0f);
                //specular texture
                cJSON *specular_texture_node = cJSON_GetObjectItem(specular_node, "specularTexture");
                if (specular_texture_node) {
                    material->specular->specular_texture.index = json_get_uint32_default(specular_texture_node, "index", UINT32_MAX);
                    material->specular->specular_texture.tex_coord = json_get_uint32_default(specular_texture_node, "texCoord", 0);
                }
                //specular color factor
                cJSON *specular_color_factor_node = cJSON_GetObjectItem(specular_node, "specularColorFactor");
                if (specular_color_factor_node) {
                    material->specular->specular_color_factor.x = cJSON_GetNumberValue(cJSON_GetArrayItem(specular_color_factor_node, 0));
                    material->specular->specular_color_factor.y = cJSON_GetNumberValue(cJSON_GetArrayItem(specular_color_factor_node, 1));
                    material->specular->specular_color_factor.z = cJSON_GetNumberValue(cJSON_GetArrayItem(specular_color_factor_node, 2));
                } else {
                    material->specular->specular_color_factor = (vec3f_t){1.0f,1.0f,1.0f};
                }
                //specular color texture
                cJSON *specular_color_texture_node = cJSON_GetObjectItem(specular_node, "specularColorTexture");
                if (specular_color_texture_node) {
                    material->specular->specular_color_texture.index = json_get_uint32_default(specular_color_texture_node, "index", UINT32_MAX);
                    material->specular->specular_color_texture.tex_coord = json_get_uint32_default(specular_color_texture_node, "texCoord", 0);
                } 
            }
        }

        //extras
        //do extras
        cJSON *extras_node = cJSON_GetObjectItem(material_node, "extras");
        if (extras_node) {
        }

        cJSON *pbr_mr_node = cJSON_GetObjectItem(material_node, "pbrMetallicRoughness");
        if (pbr_mr_node) {
            material->pbr_metallic_roughness = memory_alloc(sizeof(gltf_pbr_metallic_roughness_t), MEM_TAG_TEMP);

            //base color texture
            cJSON *base_color_texture_node = cJSON_GetObjectItem(pbr_mr_node, "baseColorTexture");
            if (base_color_texture_node) {
                material->pbr_metallic_roughness->base_color_texture.index = json_get_uint32_default(base_color_texture_node, "index", UINT32_MAX);
                material->pbr_metallic_roughness->base_color_texture.tex_coord = json_get_uint32_default(base_color_texture_node, "texCoord", 0);
            } else {
                material->pbr_metallic_roughness->base_color_texture.index = UINT32_MAX;
                material->pbr_metallic_roughness->base_color_texture.tex_coord = UINT32_MAX;
            }

            //base color factor
            cJSON *base_color_factor_node = cJSON_GetObjectItem(pbr_mr_node, "baseColorFactor");
            if (base_color_factor_node) {
                material->pbr_metallic_roughness->base_color_factor.x = cJSON_GetNumberValue(cJSON_GetArrayItem(base_color_factor_node, 0));
                material->pbr_metallic_roughness->base_color_factor.y = cJSON_GetNumberValue(cJSON_GetArrayItem(base_color_factor_node, 1));
                material->pbr_metallic_roughness->base_color_factor.z = cJSON_GetNumberValue(cJSON_GetArrayItem(base_color_factor_node, 2));
                material->pbr_metallic_roughness->base_color_factor.w = cJSON_GetNumberValue(cJSON_GetArrayItem(base_color_factor_node, 3));
            } else {
                material->pbr_metallic_roughness->base_color_factor.x = 1;
                material->pbr_metallic_roughness->base_color_factor.y = 1;
                material->pbr_metallic_roughness->base_color_factor.z = 1;
                material->pbr_metallic_roughness->base_color_factor.w = 1;
            }

            //metallic roughness texture
            cJSON *metallic_roughness_texture_node = cJSON_GetObjectItem(pbr_mr_node, "metallicRoughnesstexture");
            if (metallic_roughness_texture_node) {
                material->pbr_metallic_roughness->metallic_roughness_texture.index = json_get_uint32_default(metallic_roughness_texture_node, "index", UINT32_MAX);
                material->pbr_metallic_roughness->metallic_roughness_texture.tex_coord = json_get_uint32_default(metallic_roughness_texture_node, "texCoord", 0);
            } else {
                material->pbr_metallic_roughness->metallic_roughness_texture.index = UINT32_MAX;
                material->pbr_metallic_roughness->metallic_roughness_texture.tex_coord = UINT32_MAX;
            }

            material->pbr_metallic_roughness->metallic_factor = json_get_float_default(pbr_mr_node, "metallicFactor", 1.0f);
            material->pbr_metallic_roughness->roughness_factor = json_get_float_default(pbr_mr_node, "roughnessFactor", 1.0f);
        } else {
            LOGE("No pbr metallic roughness workflow!!");
        }

        cJSON *normal_texture_node = cJSON_GetObjectItem(material_node, "normalTexture");
        if (normal_texture_node) {
            material->normal_texture = memory_alloc(sizeof(gltf_normal_texture_info_t), MEM_TAG_TEMP);

            material->normal_texture->index = json_get_uint32_default(normal_texture_node, "index", UINT32_MAX);
            material->normal_texture->tex_coord = json_get_uint32_default(normal_texture_node, "texCoord", 0);
            material->normal_texture->scale = json_get_uint32_default(normal_texture_node, "scale", 1.0f);
        }

        //emissive factor
        cJSON *emissive_node = cJSON_GetObjectItem(material_node, "emissiveFactor");
        if (emissive_node) {
            material->emissive_factor.x = cJSON_GetNumberValue(cJSON_GetArrayItem(emissive_node, 0));
            material->emissive_factor.y = cJSON_GetNumberValue(cJSON_GetArrayItem(emissive_node, 1));
            material->emissive_factor.z = cJSON_GetNumberValue(cJSON_GetArrayItem(emissive_node, 2));
        }

        //emissive texture
        cJSON *emissive_texture_node = cJSON_GetObjectItem(material_node, "emissiveTexture");
        if (emissive_texture_node) {
            material->emissive_texture = memory_alloc(sizeof(gltf_texture_info_t), MEM_TAG_TEMP);
            material->emissive_texture->index = json_get_uint32_default(emissive_texture_node, "index", UINT32_MAX);
            material->emissive_texture->tex_coord = json_get_uint32_default(emissive_texture_node, "texCoord", 0);
        }

        //occlusion texture
        cJSON *occlusion_texture_node = cJSON_GetObjectItem(material_node, "occlusionTexture");
        if (occlusion_texture_node) {
            material->occlusion_texture = memory_alloc(sizeof(gltf_occlusion_texture_info_t), MEM_TAG_TEMP);
            material->occlusion_texture->index = json_get_uint32_default(occlusion_texture_node, "index", UINT32_MAX);
            material->occlusion_texture->tex_coord = json_get_uint32_default(occlusion_texture_node, "texCoord", 0);
            material->occlusion_texture->strength = json_get_uint32_default(occlusion_texture_node, "strength", 1.0f);
        }

        material->alpha_mode = alpha_mode_from_string(cJSON_GetStringValue(cJSON_GetObjectItem(material_node, "alphaMode")));

        material->double_sided = json_get_uint32_default(material_node, "doubleSided", 0);
    }

    //textures
    cJSON *textures_node = cJSON_GetObjectItem(root, "textures");
    gltf_model->texture_count = cJSON_GetArraySize(textures_node);
    gltf_model->textures = memory_alloc(sizeof(gltf_texture_t) * gltf_model->texture_count, MEM_TAG_TEMP);
    
    for (uint32_t i = 0; i < gltf_model->texture_count; i++) {
        cJSON *texture_node = cJSON_GetArrayItem(textures_node, i);
        gltf_texture_t *texture = &gltf_model->textures[i];
        texture->sampler = json_get_uint32_default(texture_node, "sampler", UINT32_MAX);
        texture->source  = json_get_uint32_default(texture_node, "source", UINT32_MAX);
    }

    //images
    cJSON *images_node = cJSON_GetObjectItem(root, "images");
    gltf_model->image_count = cJSON_GetArraySize(images_node);
    gltf_model->image_paths = memory_alloc(sizeof(const char *) * gltf_model->image_count, MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->image_count; i++) {
        cJSON *image_node = cJSON_GetArrayItem(images_node, i);
        const char *image_path = cJSON_GetStringValue(cJSON_GetObjectItem(image_node, "uri"));
        gltf_model->image_paths[i] = string_concatenate(asset_directory, image_path, MEM_TAG_TEMP);
    }

    //samplers
    cJSON *samplers_node = cJSON_GetObjectItem(root, "samplers");
    gltf_model->sampler_count = cJSON_GetArraySize(samplers_node);
    gltf_model->samplers = memory_alloc(sizeof(gltf_sampler_t) * gltf_model->sampler_count, MEM_TAG_TEMP);
    
    for (uint32_t i = 0; i < gltf_model->sampler_count; i++) {
        cJSON *sampler_node = cJSON_GetArrayItem(samplers_node, i);
        gltf_sampler_t *sampler = &gltf_model->samplers[i];
        sampler->mag_filter = json_get_uint32_default(sampler_node, "magFilter", UINT32_MAX);
        sampler->min_filter = json_get_uint32_default(sampler_node, "minFilter", UINT32_MAX);
        sampler->wrap_s     = json_get_uint32_default(sampler_node, "wrapS", UINT32_MAX);
        sampler->wrap_t     = json_get_uint32_default(sampler_node, "wrapT", UINT32_MAX);
    }

    LOGI("Success");
exit:
    cJSON_Delete(root);
    return gltf_model;
}

