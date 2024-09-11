#include "json_loader.h"
#include "../../game.h"
#include "../logger/logger.h"
#include "../string/string.h"
#include "../math/math_utils.h"
#include "../utils/utils.h"
#include "cJSON.c"

#include <assert.h>

#define NIL UINT32_MAX
#define MODEL_NODE_CHILDREN_COUNT 4

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

static entity_type_t 
entity_type_from_string(const char *str)
{
    if (strcmp(str, "player") == 0) return ENTITY_TYPE_PLAYER;
    if (strcmp(str, "weapon") == 0) return ENTITY_TYPE_WEAPON;
    if (strcmp(str, "off-hand") == 0) return ENTITY_TYPE_WEAPON;
    if (strcmp(str, "enemy") == 0) return ENTITY_TYPE_ENEMY;
    if (strcmp(str, "tile") == 0) return ENTITY_TYPE_TILE;
    if (strcmp(str, "widget") == 0) return ENTITY_TYPE_WIDGET;
    return ENTITY_TYPE_UNKNOWN;
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
    if (strcmp(str, "OPAQUE") == 0) return OPAQUE;
    else if (strcmp(str, "MASK") == 0) return MASK;
    else if (strcmp(str, "BLEND") == 0) return BLEND;
    else return NIL;
}

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

#if 0
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


static void set_push(uint32_t *set, uint32_t *p_count, uint32_t num)
{
    uint32_t count = *p_count;

    for (uint32_t i = 0; i < count; i++)
    {
        if (set[i] == num) return;
    }
    set[count++] = num;
    *p_count = count;
}

gltf_model_t *model_load_from_gltf(const char * path)
{
    long buf_size;
    char *buf = read_whole_file(path, &buf_size, MEM_TAG_TEMP);
    if (!buf)
    {
        LOGE("Can't read gltf file %s", path);
        return NULL;
    }

    cJSON* root = cJSON_Parse(buf);
    if (!root)
    {
        LOGE("Unable to parse json file %s", path);
        return NULL;
    }

    gltf_model_t *gltf_model = memory_alloc(sizeof(gltf_model_t), MEM_TAG_TEMP);
    if (!gltf_model)
    {
        LOGE("Failed to allocate model memory");
        return NULL;
    }
    
    gltf_model->path = string_duplicate(path, MEM_TAG_TEMP);

    //do scenes
    cJSON *scenes_node = cJSON_GetObjectItem(root, "scenes");
    gltf_model->scene_count = cJSON_GetArraySize(scenes_node);
    gltf_model->scenes = memory_alloc(sizeof(gltf_scene_t) * gltf_model->scene_count, MEM_TAG_TEMP);

    for  (uint32_t i = 0; i < gltf_model->scene_count; i++)
    {
        cJSON *scene_node = cJSON_GetArrayItem(scenes_node, i);
        cJSON *nodes_node = cJSON_GetObjectItem(scene_node, "nodes");
        int node_count = cJSON_GetArraySize(nodes_node);

        gltf_scene_t *scene = &gltf_model->scenes[i];
        scene->node_count = node_count;
        scene->nodes = memory_alloc(sizeof(uint32_t) * scene->node_count, MEM_TAG_TEMP);

        for (uint32_t j = 0; j < scene->node_count; j++)
        {
            scene->nodes[j] = cJSON_GetNumberValue(cJSON_GetArrayItem(nodes_node, j));
        }
    }

    //do nodes 
    cJSON *nodes = cJSON_GetObjectItem(root, "nodes");
    if (!nodes)
    {
        LOGE("Unable to find \"nodes\" node. Aborting");
        goto exit;
    }

    gltf_model->node_count = cJSON_GetArraySize(nodes);
    gltf_model->nodes = memory_alloc(gltf_model->node_count * sizeof(gltf_node_t), MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->node_count; i++)
    {
        cJSON* node = cJSON_GetArrayItem(nodes, i);
        gltf_node_t *model_node = &gltf_model->nodes[i];
        
        //children
        cJSON *children = cJSON_GetObjectItem(node, "children");
        model_node->child_count = cJSON_GetArraySize(children);
        assert(model_node->child_count <= MODEL_NODE_CHILDREN_COUNT);

        for (int j = 0; j < model_node->child_count; j++)
        {
            model_node->children[j] = (uint8_t)cJSON_GetNumberValue(cJSON_GetArrayItem(children, j));
        }

        //name
        const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(node, "name"));
        model_node->name = string_duplicate(name, MEM_TAG_TEMP);

        //matrix
        cJSON *matrix = cJSON_GetObjectItem(node, "matrix");
        if (matrix)
        {
            int j = 0;
            //copy the matrix directly
            for (uint32_t col = 0; col < 4; col++)
            {
                for (uint32_t row = 0; row < 4; row++)
                {
                    model_node->local_transform.m[row][col] = (float)cJSON_GetNumberValue(cJSON_GetArrayItem(matrix, j++));
                }
            }
        }
        else 
        {
            //default t,r,s
            vec3f_t t_vec = {0};
            quat_t rot_q = {0};
            vec3f_t scale_vec = {1.0f, 1.0f, 1.0f};

            //construct the matrix from TRS
            cJSON *trans = cJSON_GetObjectItem(node, "translation");
            if (trans)
            {
                t_vec.x = cJSON_GetNumberValue(cJSON_GetArrayItem(trans, 0));
                t_vec.y = cJSON_GetNumberValue(cJSON_GetArrayItem(trans, 1));
                t_vec.z = cJSON_GetNumberValue(cJSON_GetArrayItem(trans, 2));
            }

            cJSON *rot = cJSON_GetObjectItem(node, "rotation");
            if (rot)
            {
                rot_q.x = cJSON_GetNumberValue(cJSON_GetArrayItem(rot, 0));
                rot_q.y = cJSON_GetNumberValue(cJSON_GetArrayItem(rot, 1));
                rot_q.z = cJSON_GetNumberValue(cJSON_GetArrayItem(rot, 2));
                rot_q.w = cJSON_GetNumberValue(cJSON_GetArrayItem(rot, 3));
            } 

            cJSON *scale = cJSON_GetObjectItem(node, "scale");
            if (scale)
            {
                scale_vec.x = cJSON_GetNumberValue(cJSON_GetArrayItem(scale, 0));
                scale_vec.x = cJSON_GetNumberValue(cJSON_GetArrayItem(scale, 1));
                scale_vec.x = cJSON_GetNumberValue(cJSON_GetArrayItem(scale, 2));
            }

            transform_from_TRS(&model_node->local_transform, t_vec, rot_q, scale_vec);
        }

        //skin
        model_node->skin = UINT32_MAX;
        cJSON* skin_node = cJSON_GetObjectItem(node, "skin");
        if (skin_node)
        {
            model_node->skin = cJSON_GetNumberValue(skin_node);
        }

        //mesh
        model_node->mesh = UINT32_MAX;
        cJSON* mesh_node = cJSON_GetObjectItem(node, "mesh");
        if (mesh_node)
        {
            model_node->mesh = cJSON_GetNumberValue(mesh_node);
        }
    }

    //we need to load the actual data here

    //cache buffers
    cJSON *buffers_node       = cJSON_GetObjectItem(root, "buffers");
    gltf_model->buffer_count  = (uint32_t)cJSON_GetArraySize(buffers_node);
    gltf_model->buffers = memory_alloc(gltf_model->buffer_count * sizeof(gltf_buffer_t), MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->buffer_count; i++)
    {
        cJSON* buffer_node      = cJSON_GetArrayItem(buffers_node, i);
        gltf_buffer_t *buffer   = &gltf_model->buffers[i];
        const char *uri = cJSON_GetStringValue(cJSON_GetObjectItem(buffer_node, "uri"));
        const char *binary_path = string_concatenate("./assets/models/cesium_man/", uri, MEM_TAG_TEMP);
        buffer->size = cJSON_GetNumberValue(cJSON_GetObjectItem(buffer_node, "byteLength"));
        long size;
        buffer->data = read_whole_file(binary_path, &size, MEM_TAG_TEMP);
    }

    //cache accessors
    cJSON* accessors_node      = cJSON_GetObjectItem(root, "accessors");
    gltf_model->accessor_count = cJSON_GetArraySize(accessors_node);
    gltf_model->accessors      = memory_alloc(gltf_model->accessor_count * sizeof(gltf_accessor_t), MEM_TAG_TEMP);
    
    for (uint32_t i = 0; i < gltf_model->accessor_count; i++)
    {
        cJSON *accessor_node = cJSON_GetArrayItem(accessors_node, i);

        gltf_accessor_t *accessor = &gltf_model->accessors[i];
        accessor->buffer_view = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(accessor_node, "bufferView"));
        accessor->byte_offset = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(accessor_node, "byteOffset"));
        accessor->component_type = cJSON_GetNumberValue(cJSON_GetObjectItem(accessor_node, "componentType"));
        accessor->type = value_type_from_string(cJSON_GetStringValue(cJSON_GetObjectItem(accessor_node, "type")));
        accessor->count = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(accessor_node, "count"));
    }

    //cache bufferviews 
    cJSON* buffer_views_node      = cJSON_GetObjectItem(root, "bufferViews");
    gltf_model->buffer_view_count = cJSON_GetArraySize(buffer_views_node);
    gltf_model->buffer_views      = memory_alloc(gltf_model->buffer_view_count * sizeof(gltf_buffer_view_t), MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->buffer_view_count; i++)
    {
        cJSON *buffer_view_node = cJSON_GetArrayItem(buffer_views_node, i);

        gltf_buffer_view_t *buffer_view = &gltf_model->buffer_views[i];
        buffer_view->buffer = cJSON_GetNumberValue(cJSON_GetObjectItem(buffer_view_node, "buffer"));
        buffer_view->byte_offset = cJSON_GetNumberValue(cJSON_GetObjectItem(buffer_view_node, "byteOffset"));
        buffer_view->byte_length = cJSON_GetNumberValue(cJSON_GetObjectItem(buffer_view_node, "byteLength"));
        buffer_view->byte_stride = cJSON_GetNumberValue(cJSON_GetObjectItem(buffer_view_node, "byteStride"));
        buffer_view->target = cJSON_GetNumberValue(cJSON_GetObjectItem(buffer_view_node, "target"));
    }
    
    //load meshes
    cJSON *meshes_node = cJSON_GetObjectItem(root, "meshes");
    gltf_model->mesh_count = cJSON_GetArraySize(meshes_node);
    gltf_model->meshes = memory_alloc(sizeof(gltf_mesh_t) * gltf_model->mesh_count, MEM_TAG_TEMP);

    for (int i = 0; i < gltf_model->mesh_count; i++)
    {
        cJSON *mesh_node = cJSON_GetArrayItem(meshes_node, i);
        gltf_mesh_t *mesh = &gltf_model->meshes[i];

        const char *mesh_name = cJSON_GetStringValue(cJSON_GetObjectItem(mesh_node, "name"));
        mesh->name = string_duplicate(mesh_name, MEM_TAG_TEMP);

        cJSON *primitives_node = cJSON_GetArrayItem(mesh_node, 0);
        int primitive_count = cJSON_GetArraySize(primitives_node);
        if (primitive_count > 1)
        {
            assert(false && "Mesh has more than one primitive");
        }
        cJSON *primitive_node = cJSON_GetArrayItem(primitives_node, 0);

        cJSON *attributes_node    = cJSON_GetObjectItem(primitive_node, "attributes");
        mesh->primitive.joints    = cJSON_GetNumberValue(cJSON_GetObjectItem(attributes_node, "JOINTS_0"));
        mesh->primitive.normal    = cJSON_GetNumberValue(cJSON_GetObjectItem(attributes_node, "NORMAL"));
        mesh->primitive.tex_coord = cJSON_GetNumberValue(cJSON_GetObjectItem(attributes_node, "TEXCOORD_0"));
        mesh->primitive.position  = cJSON_GetNumberValue(cJSON_GetObjectItem(attributes_node, "POSITION"));
        mesh->primitive.weights   = cJSON_GetNumberValue(cJSON_GetObjectItem(attributes_node, "WEIGHTS_0"));

        mesh->primitive.indices  = cJSON_GetNumberValue(cJSON_GetObjectItem(primitive_node, "indices"));
        mesh->primitive.mode     = cJSON_GetNumberValue(cJSON_GetObjectItem(primitive_node, "mode"));
        mesh->primitive.material = cJSON_GetNumberValue(cJSON_GetObjectItem(primitive_node, "material")); 
    }

    //index count
    gltf_model->index_counts         = memory_alloc(sizeof(uint32_t) * gltf_model->mesh_count, MEM_TAG_TEMP);
    gltf_model->indices_byte_lengths = memory_alloc(sizeof(size_t) * gltf_model->mesh_count, MEM_TAG_TEMP);
    //vertex count
    gltf_model->vertex_counts = memory_alloc(sizeof(uint32_t) * gltf_model->mesh_count, MEM_TAG_TEMP);
    gltf_model->vertices_byte_lengths = memory_alloc(sizeof(size_t) * gltf_model->mesh_count, MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->mesh_count; i++)
    {
        //vertices
        uint32_t *unique_bvs = memory_alloc(sizeof(uint32_t) * 5, MEM_TAG_TEMP);
        uint32_t unique_bv_count = 0;

        uint32_t accessor_index = gltf_model->meshes[i].primitive.joints;
        gltf_accessor_t *accessor = &gltf_model->accessors[accessor_index];
        set_push(unique_bvs, &unique_bv_count, accessor->buffer_view);
        gltf_model->vertex_counts[i] = accessor->count;

        accessor_index = gltf_model->meshes[i].primitive.normal;
        accessor = &gltf_model->accessors[accessor_index];
        set_push(unique_bvs, &unique_bv_count, accessor->buffer_view);

        accessor_index = gltf_model->meshes[i].primitive.tex_coord;
        accessor = &gltf_model->accessors[accessor_index];
        set_push(unique_bvs, &unique_bv_count, accessor->buffer_view);

        accessor_index = gltf_model->meshes[i].primitive.position;
        accessor = &gltf_model->accessors[accessor_index];
        set_push(unique_bvs, &unique_bv_count, accessor->buffer_view);

        accessor_index = gltf_model->meshes[i].primitive.weights;
        accessor = &gltf_model->accessors[accessor_index];
        set_push(unique_bvs, &unique_bv_count, accessor->buffer_view);

        for (uint32_t j = 0; j < unique_bv_count; j++)
        {
            gltf_buffer_view_t *bv = &gltf_model->buffer_views[unique_bvs[j]];
            gltf_model->vertices_byte_lengths[i] += bv->byte_length; 
        }
        
        //indices
        accessor_index = gltf_model->meshes[i].primitive.indices;
        gltf_model->index_counts[i] = gltf_model->accessors[accessor_index].count;

        gltf_buffer_view_t* bv = &gltf_model->buffer_views[gltf_model->accessors[accessor_index].buffer_view];
        gltf_model->indices_byte_lengths[i] = bv->byte_length;
    }
    
    //load animations
    cJSON *animations_node = cJSON_GetObjectItem(root, "animations");
    gltf_model->animation_count = cJSON_GetArraySize(animations_node);

    gltf_model->animations = memory_alloc(sizeof(gltf_animation_t) * gltf_model->animation_count, MEM_TAG_TEMP);
    for (int32_t i = 0; i < gltf_model->animation_count; i++)
    {
        gltf_animation_t *animation = &gltf_model->animations[i];

        cJSON *animation_node = cJSON_GetArrayItem(animations_node, i);

        cJSON *channels_node = cJSON_GetObjectItem(animation_node, "channels");
        animation->channel_count = (uint32_t)cJSON_GetArraySize(channels_node);
        animation->channels = memory_alloc(sizeof(gltf_channel_t) * animation->channel_count, MEM_TAG_TEMP);

        for (uint32_t j = 0; j < animation->channel_count; j++)
        {
            cJSON *channel_node = cJSON_GetArrayItem(channels_node, j);
            cJSON *target_node  = cJSON_GetObjectItem(channel_node, "target");

            gltf_channel_t *channel = &animation->channels[j];

            channel->sampler = cJSON_GetNumberValue(cJSON_GetObjectItem(channel_node, "sampler"));
            const char *path = cJSON_GetStringValue(cJSON_GetObjectItem(target_node, "path"));
            channel->path    = path_from_string(path);
            channel->node    = cJSON_GetNumberValue(cJSON_GetObjectItem(target_node, "node"));
        }

        cJSON *samplers_node = cJSON_GetObjectItem(animation_node, "samplers");
        animation->sampler_count = (uint32_t)cJSON_GetArraySize(samplers_node);
        animation->samplers = memory_alloc(sizeof(gltf_animation_sampler_t) * animation->sampler_count, MEM_TAG_TEMP);

        for (uint32_t j = 0; j < animation->sampler_count; j++)
        {
            cJSON *sampler_node = cJSON_GetArrayItem(samplers_node, j);
            gltf_animation_sampler_t *sampler = &animation->samplers[j];

            sampler->input = cJSON_GetNumberValue(cJSON_GetObjectItem(sampler_node, "input"));
            sampler->output = cJSON_GetNumberValue(cJSON_GetObjectItem(sampler_node, "output"));
            const char *interpolation = cJSON_GetStringValue(cJSON_GetObjectItem(sampler_node, "interpolation"));
            sampler->interpolation = interpolation_from_string(interpolation);
        }
    }

    //skins
    cJSON* skins_node         = cJSON_GetObjectItem(root, "skins");
    gltf_model->skin_count    = cJSON_GetArraySize(skins_node);
    gltf_model->skins         = memory_alloc(sizeof(gltf_skin_t) * gltf_model->skin_count, MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->skin_count; i++)
    {
        cJSON *skin_node       = cJSON_GetArrayItem(skins_node, i);

        gltf_skin_t *gltf_skin = &gltf_model->skins[i];
        gltf_skin->inverse_bind_matrices = cJSON_GetNumberValue(cJSON_GetObjectItem(skin_node, "inverseBindMatrices"));
        gltf_skin->skeleton = cJSON_GetNumberValue(cJSON_GetObjectItem(skin_node, "skeleton"));

        cJSON *joints_node = cJSON_GetObjectItem(skin_node, "joints");
        gltf_skin->joint_count = cJSON_GetArraySize(joints_node);
        gltf_skin->joints = memory_alloc(gltf_skin->joint_count * sizeof(uint32_t), MEM_TAG_TEMP);

        for (uint32_t j = 0; j < gltf_skin->joint_count; j++)
        {
            gltf_skin->joints[j] = cJSON_GetNumberValue(cJSON_GetArrayItem(joints_node, j));
        }
        const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(skin_node, "name"));
        gltf_skin->name = string_duplicate(name, MEM_TAG_TEMP);
    }
    
    //materials
    cJSON *materials_node = cJSON_GetObjectItem(root, "materials");
    gltf_model->material_count = cJSON_GetArraySize(materials_node);
    gltf_model->materials = memory_alloc(sizeof(gltf_material_t) * gltf_model->material_count, MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->material_count; i++)
    {
        cJSON* material_node = cJSON_GetArrayItem(materials_node, i);
        gltf_material_t *material = &gltf_model->materials[i];

        cJSON *pbr_mr_node = cJSON_GetObjectItem(material_node, "pbrMetallicRoughness");
        cJSON *base_color_texture_node = cJSON_GetObjectItem(pbr_mr_node, "baseColorTexture");
        material->pbr_metallic_roughness.base_color_texture_index = cJSON_GetNumberValue(cJSON_GetObjectItem(base_color_texture_node, "index"));
        material->pbr_metallic_roughness.tex_coord = cJSON_GetNumberValue(cJSON_GetObjectItem(base_color_texture_node, "texCoord"));
        material->pbr_metallic_roughness.metallic_factor = cJSON_GetNumberValue(cJSON_GetObjectItem(pbr_mr_node, "metallicFactor"));

        cJSON *base_color_factor_node = cJSON_GetObjectItem(pbr_mr_node, "baseColorFactor");
        material->pbr_metallic_roughness.base_color_factor.x = cJSON_GetNumberValue(cJSON_GetArrayItem(base_color_factor_node, 0));
        material->pbr_metallic_roughness.base_color_factor.y = cJSON_GetNumberValue(cJSON_GetArrayItem(base_color_factor_node, 1));
        material->pbr_metallic_roughness.base_color_factor.z = cJSON_GetNumberValue(cJSON_GetArrayItem(base_color_factor_node, 2));
        material->pbr_metallic_roughness.base_color_factor.w = cJSON_GetNumberValue(cJSON_GetArrayItem(base_color_factor_node, 3));

        cJSON *emissive_node = cJSON_GetObjectItem(material_node, "emissiveFactor");
        material->emissive_factor.x = cJSON_GetNumberValue(cJSON_GetArrayItem(emissive_node, 0));
        material->emissive_factor.y = cJSON_GetNumberValue(cJSON_GetArrayItem(emissive_node, 1));
        material->emissive_factor.z = cJSON_GetNumberValue(cJSON_GetArrayItem(emissive_node, 2));

        const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(material_node, "name"));
        material->name = string_duplicate(name, MEM_TAG_TEMP);

        material->alpha_mode = alpha_mode_from_string(cJSON_GetStringValue(cJSON_GetObjectItem(material_node, "alphaMode")));
        material->double_sided = cJSON_GetNumberValue(cJSON_GetObjectItem(material_node, "doubleSided"));
    }

    //textures
    cJSON *textures_node = cJSON_GetObjectItem(root, "textures");
    gltf_model->texture_count = cJSON_GetArraySize(textures_node);
    gltf_model->textures = memory_alloc(sizeof(gltf_texture_t) * gltf_model->texture_count, MEM_TAG_TEMP);
    
    for (uint32_t i = 0; i < gltf_model->texture_count; i++)
    {
        cJSON *texture_node = cJSON_GetArrayItem(textures_node, i);
        gltf_texture_t *texture = &gltf_model->textures[i];
        texture->sampler = cJSON_GetNumberValue(cJSON_GetObjectItem(texture_node, "sampler"));
        texture->source  = cJSON_GetNumberValue(cJSON_GetObjectItem(texture_node, "source"));
    }

    //images
    cJSON *images_node = cJSON_GetObjectItem(root, "images");
    gltf_model->image_count = cJSON_GetArraySize(images_node);
    gltf_model->image_paths = memory_alloc(sizeof(const char *) * gltf_model->image_count, MEM_TAG_TEMP);

    for (uint32_t i = 0; i < gltf_model->image_count; i++)
    {
        cJSON *image_node = cJSON_GetArrayItem(images_node, i);
        const char *image_path = cJSON_GetStringValue(cJSON_GetObjectItem(image_node, "uri"));
        gltf_model->image_paths[i] = string_concatenate("./assets/models/cesium_man/", image_path, MEM_TAG_TEMP);
    }

    //samplers
    cJSON *samplers_node = cJSON_GetObjectItem(root, "samplers");
    gltf_model->sampler_count = cJSON_GetArraySize(samplers_node);
    gltf_model->samplers = memory_alloc(sizeof(gltf_sampler_t) * gltf_model->sampler_count, MEM_TAG_TEMP);
    
    for (uint32_t i = 0; i < gltf_model->sampler_count; i++)
    {
        cJSON *sampler_node = cJSON_GetArrayItem(samplers_node, i);
        gltf_sampler_t *sampler = &gltf_model->samplers[i];
        sampler->mag_filter = cJSON_GetNumberValue(cJSON_GetObjectItem(sampler_node, "magFilter"));
        sampler->min_filter = cJSON_GetNumberValue(cJSON_GetObjectItem(sampler_node, "minFilter"));
        sampler->wrap_s     = cJSON_GetNumberValue(cJSON_GetObjectItem(sampler_node, "wrapS"));
        sampler->wrap_t     = cJSON_GetNumberValue(cJSON_GetObjectItem(sampler_node, "wrapT"));
    }

    LOGI("Success");
exit:
    cJSON_Delete(root);
    return gltf_model;
}


static void load_tiles_from_json(game_t *game, cJSON *root)
{
    cJSON *tilemap_node = cJSON_GetObjectItem(root, "tilemap");
    if (!tilemap_node)
    {
        LOGE("NOOOOOOOOOOOOOO");
        return;
    }

    const char *map_file_path = cJSON_GetStringValue(cJSON_GetObjectItem(tilemap_node, "map_file"));
    if (!map_file_path)
    {
        LOGE("NOOOOO");
        return;
    }

    FILE *map_file = fopen(map_file_path, "r");
    if (!map_file)
    {
        LOGE("NOOOOOOOOOO");
        return;
    }

    uint32_t rows  = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(tilemap_node, "row_count"));
    uint32_t cols  = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(tilemap_node, "col_count"));
    uint32_t tile_size = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(tilemap_node, "tile_size"));
    
    const char *texture_path = cJSON_GetStringValue(cJSON_GetObjectItem(tilemap_node, "texture"));

    for (uint32_t row = 0; row < rows; row++)
    {
        for (uint32_t col = 0; col < cols; col++)
        {
            int tile_row = fgetc(map_file) - '0';
            int tile_col = fgetc(map_file) - '0';
            if (tile_row == EOF || tile_col == EOF)
            {
                LOGE("NOOOOOO");
                fclose(map_file);
                return;
            }
            
            //create tile entity
            entity_t *tile_entity = bulk_data_push_struct(&game->entities);
            tile_entity->id = game->entity_id++;
            tile_entity->type = ENTITY_TYPE_TILE;
            tile_entity->p = (vec2f_t){(float)col * game->tile_width, (float)row * game->tile_width};
            tile_entity->size = (vec2f_t){(float)game->tile_width, (float)game->tile_width};
            tile_entity->z_index = 0;

            tile_t *tile_data = bulk_data_push_struct(&game->tiles);
            tile_data->entity = tile_entity;
            tile_data->sprite.texture = asset_store_get_texture(&game->asset_store, texture_path);
            assert(tile_data->sprite.texture);
            tile_data->sprite.src_rect = (rect_t){{(float)tile_col * tile_size, (float)tile_row * tile_size},{tile_size,tile_size}};
            tile_entity->data = tile_data;

            int collision = fgetc(map_file) - '0';
            if (collision == 1)
            {
                tile_entity->flags = 0x1;
            }
            else
            {
                tile_entity->flags = 0x0;
            }

            fgetc(map_file);
        }
        fgetc(map_file);
    }
}

void load_assets_from_json(game_t *game, cJSON *root)
{
    cJSON *assets = cJSON_GetObjectItem(root, "assets");
    if (assets)
    {
        cJSON *file_paths = cJSON_GetObjectItem(assets, "textures");
        if (file_paths)
        {
            cJSON *pair;
            cJSON_ArrayForEach(pair, file_paths)
            {
                const char *key = string_duplicate(cJSON_GetStringValue(cJSON_GetObjectItem(pair, "key")), MEM_TAG_PERMANENT);
                const char *value = string_duplicate(cJSON_GetStringValue(cJSON_GetObjectItem(pair, "value")), MEM_TAG_PERMANENT);

                if (key && value)
                {
                    asset_store_add_texture(&game->asset_store, &game->renderer, key, value);
                }
                else
                {
                    LOGE("Unable to parse key value pair");
                }
            }
        }
        else
        {
            LOGE("Unable to parse texture file paths");
            return;
        }

        uint32_t glyph_count = 0;
        
        cJSON *fonts = cJSON_GetObjectItem(assets, "fonts");
        if (fonts)
        {
            cJSON *font = NULL;
            cJSON_ArrayForEach(font, fonts)
            {
                const char *texture_id = cJSON_GetStringValue(cJSON_GetObjectItem(font, "texture"));
                game->font.texture = asset_store_get_texture(&game->asset_store, texture_id);
                assert(game->font.texture);
                
                const char *file_name = cJSON_GetStringValue(cJSON_GetObjectItem(font, "vertices"));
                char *buf = read_whole_file(file_name, NULL, MEM_TAG_TEMP);
                char *tok = strtok(buf, ",");
                while(tok)
                {
                    float x1 = strtof(tok, NULL);
                    tok = strtok(NULL, ",");
                    float x2 = strtof(tok, NULL);
                    tok = strtok(NULL, ",");
                    float y1 = strtof(tok, NULL);
                    tok = strtok(NULL, ",");
                    float y2 = strtof(tok, NULL);
                    tok = strtok(NULL, ",");

                    rect_t *glyph = &game->font.glyphs[glyph_count++];
                    glyph->min.x  = x1;
                    glyph->min.y  = y1;
                    glyph->size.x = x2 - x1;
                    glyph->size.y = y2 - y1; 
                }
            }
        }
    }
    else
    {
        LOGE("Unable to parse assets node");
        return;
    }
}

uint32_t load_animations_from_json(game_t             *game, 
                                   struct cJSON       *node, 
                                   animation_chunk_t **animation_chunks, 
                                   uint32_t            animation_chunk_capacity, 
                                   vec2f_t             entity_size)
{
    uint32_t chunk_count = 0;
    cJSON *child = NULL;

    cJSON_ArrayForEach(child, node)
    {
        animation_chunk_t *current_chunk = animation_chunks[chunk_count];
        if (!current_chunk)
        {
            current_chunk = bulk_data_push_struct(&game->animation_chunks);
            current_chunk->count = 0;
            animation_chunks[chunk_count++] = current_chunk;
        }
        else if (current_chunk->count == MAX_ANIMATIONS_PER_CHUNK)
        {
            current_chunk = bulk_data_push_struct(&game->animation_chunks);
            current_chunk->count = 0;
            animation_chunks[chunk_count++] = current_chunk;
        }

        sprite_animation_t *animation = &current_chunk->animations[current_chunk->count++];

        assert(chunk_count <= animation_chunk_capacity && "Unable to add more animations");
        const char *state_str = cJSON_GetStringValue(cJSON_GetObjectItem(child, "state"));
        animation->state = get_animation_state_from_string(state_str);

        const char *texture_id = cJSON_GetStringValue(cJSON_GetObjectItem(child, "texture"));
        animation->texture = asset_store_get_texture(&game->asset_store, texture_id);
        animation->duration = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(child, "duration"));
        animation->sprite_count = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(child, "sprite_count"));

        cJSON *grand_child = cJSON_GetObjectItem(child, "offset");
        uint32_t offset_x = cJSON_GetNumberValue(cJSON_GetObjectItem(grand_child, "x"));
        uint32_t offset_y = cJSON_GetNumberValue(cJSON_GetObjectItem(grand_child, "y"));

        grand_child = cJSON_GetObjectItem(child, "size");
        float w = cJSON_GetNumberValue(cJSON_GetObjectItem(grand_child, "x"));
        float h = cJSON_GetNumberValue(cJSON_GetObjectItem(grand_child, "y"));
        uint32_t stride = cJSON_GetNumberValue(cJSON_GetObjectItem(child, "stride"));

        for (uint32_t i = 0; i < animation->sprite_count; i++)
        {
            animation->sprites[i] = (rect_t){{offset_x + i * stride, offset_y}, {w,h}};
        }
    }
    return chunk_count;
}

void load_player_from_json(entity_t *entity, player_t *player, cJSON *entity_node, game_t *game)
{
    player->entity = entity;
    entity->data   = player;
    player->velocity = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(entity_node, "velocity"));
    player->dp.x = 0;
    player->dp.y = 0;
    entity->flags = ENTITY_CAN_COLLIDE;

    cJSON *anim_node = cJSON_GetObjectItem(entity_node, "animations");
    player->animation_chunk_count = load_animations_from_json(game, 
                                                                anim_node, 
                                                                player->animation_chunks,  
                                                                MAX_ANIMATION_CHUNKS_PER_ENTITY, 
                                                                entity->size);
    player->current_animation = &player->animation_chunks[0]->animations[0];
    const char *player_name = cJSON_GetStringValue(cJSON_GetObjectItem(entity_node, "name"));
    if (player_name)
    {
        //player has a name
        player->name = bulk_data_push_struct(&game->text_labels);
        player->name->text = string_duplicate(player_name, MEM_TAG_PERMANENT);
        player->name->font = &game->font;
        player->name->timer = 0.0f;
        player->name->duration = 0.0f;
        player->name->offset = (vec2f_t){game->tile_width / 4,-game->tile_width / 4};
    }
    else
    {
        LOGE("Player has no name");
        assert(false);
    }
}

static void load_entity_from_json(entity_t *entity, cJSON *entity_node, game_t *game)
{
    entity->id = game->entity_id++;
    const char *entity_type = cJSON_GetStringValue(cJSON_GetObjectItem(entity_node, "type"));
    entity->type = entity_type_from_string(entity_type);
    entity->data = NULL;

    cJSON *node = NULL;
    node = cJSON_GetObjectItem(entity_node, "position");
    entity->p.x = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "x"));
    entity->p.y = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "y"));

    node = cJSON_GetObjectItem(entity_node, "size");
    entity->size.x = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "x")) * game->tile_width;
    entity->size.y = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "y")) * game->tile_width;

    entity->z_index = (int32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(entity_node, "z_index"));

    switch (entity->type)
    {
        case ENTITY_TYPE_PLAYER:
        {
            load_player_from_json(entity, &game->player_data, entity_node, game);
            game->player_entity = entity;
            break;
        }
        case ENTITY_TYPE_WEAPON:
        {
            weapon_t *weapon = bulk_data_push_struct(&game->weapons);
            entity->data = weapon;
            entity->flags = 0x0;

            weapon->entity = entity;  
            cJSON *sprite_node = cJSON_GetObjectItem(entity_node, "sprite");
            const char *texture_id = cJSON_GetStringValue(cJSON_GetObjectItem(sprite_node, "texture"));
            
            cJSON *offset_node = cJSON_GetObjectItem(sprite_node, "offset");
            float px = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(offset_node, "x"));
            float py = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(offset_node, "y"));

            cJSON *size_node = cJSON_GetObjectItem(sprite_node, "size");
            float sx = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(size_node, "x"));
            float sy = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(size_node, "y"));

            weapon->sprite.texture  = asset_store_get_texture(&game->asset_store, texture_id);
            weapon->sprite.src_rect.min.x = px;
            weapon->sprite.src_rect.min.y = py;
            weapon->sprite.src_rect.size.x = sx;
            weapon->sprite.src_rect.size.y = sy;

            if (strcmp(entity_type, "weapon") == 0)
            {
                weapon->slot = WEAPON_SLOT_MAIN_HAND;
            }
            else if (strcmp(entity_type, "off-hand") == 0)
            {
                weapon->slot =  WEAPON_SLOT_OFF_HAND;
            }
            break;
        }
        default:
        {
            LOGE("Unknown entity type!");
            assert(false);
            break;
        }
    }
}

void load_entities_from_json(game_t *game, cJSON *root)
{
    //! NOTE: Load Entities
    cJSON *entities_node = cJSON_GetObjectItem(root, "entities");
    if (!entities_node) 
    {
        LOGE("NOOOOOOOO");
        return;
    }

    cJSON *entity_node = NULL;
    cJSON_ArrayForEach(entity_node, entities_node)
    {
        entity_t *entity = bulk_data_push_struct(&game->entities);
        load_entity_from_json(entity, entity_node, game);
    }
}

void load_level_from_json(game_t *game, const char *data_file_path)
{
    char *file_buf = read_whole_file(data_file_path, NULL, MEM_TAG_TEMP);
    cJSON *root = cJSON_Parse(file_buf);
    if (!root)
    {
        LOGE("Unable to parse json file");
        return;
    }

    load_assets_from_json(game, root);
    load_entities_from_json(game, root);
    load_tiles_from_json(game, root);

    cJSON_Delete(root);
}

