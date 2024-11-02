#include "asset_store.h"

#include "../renderer/frontend/renderer.h"
#include "../containers/containers.h"

#include "../logger/logger.h"
#include "../string/string.h"

#define STB_DS_IMPLEMENTATION
#include <stb/stb_ds.h>

#include <assert.h>
#include "json_loader.c"
#include "skinned_model.c"

void asset_store_init(asset_store_t *asset_store, bulk_data_texture_t *textures, bulk_data_skinned_model_t *skinned_models)
{
    asset_store->texture_map = NULL;
    asset_store->skinned_model_map = NULL;

    asset_store->textures = textures;
    asset_store->skinned_models = skinned_models;
}

void asset_store_add_texture(asset_store_t *store,
                             renderer_t    *renderer,
                             const char    *asset_id, 
                             const char    *file_path)
{
    if ((shgetp_null(store->texture_map, asset_id)) == NULL) {
        uint32_t slot = bulk_data_allocate_slot_texture_t(store->textures);
        texture_t *texture = bulk_data_getp_null_texture_t(store->textures, slot);
        if (texture) {
            if (!renderer_create_texture(renderer, texture, file_path)) {
                LOGE("Unable to load texture from file: %s", file_path);
                //remove from bulk data
                bulk_data_delete_item_texture_t(store->textures, slot);
                return;
            }
            shput(store->texture_map, asset_id, slot);
        }
    }
}

void asset_store_add_skinned_model(asset_store_t *asset_store, renderer_t *renderer, const char *asset_id, const char *file_path, bulk_data_renderbuffer_t *renderbuffers)
{
    if ((shgetp_null(asset_store->skinned_model_map, asset_id)) == NULL) {
        uint32_t slot = bulk_data_allocate_slot_skinned_model_t(asset_store->skinned_models);
        skinned_model_t *model = bulk_data_getp_null_skinned_model_t(asset_store->skinned_models, slot);
        if (model) {
            if (!create_skinned_model(model, file_path, asset_id, renderer, asset_store, renderbuffers)) {
                LOGE("Unable to load skinned model from file: %s", file_path);
                bulk_data_delete_item_skinned_model_t(asset_store->skinned_models, slot);
                return;
            }
            shput(asset_store->skinned_model_map, asset_id, slot);
        }
    }
}

void asset_store_remove_skinned_model(asset_store_t *asset_store, uint32_t index) {
    skinned_model_t *model = bulk_data_getp_null_skinned_model_t(asset_store->skinned_models, index);
    //!TODO: call destructor here
    if (model) {
        for (uint32_t i = 0; i < model->animation_count;i++) {
            memory_dealloc(model->animations[i].samplers);
            memory_dealloc(model->animations[i].channels);
        }
    }
    bulk_data_delete_item_skinned_model_t(asset_store->skinned_models,index);
}

uint32_t asset_store_get_asset_index(asset_store_t *asset_store, const char *asset_id, asset_type_e type)
{
    uint32_t result = UINT32_MAX;
    string_hash_entry_t *map = NULL;

    switch(type)
    {
        case ASSET_TYPE_TEXTURE:
            map = asset_store->texture_map;
            break;
        case ASSET_TYPE_SKINNED_MODEL:
            map = asset_store->skinned_model_map;
            break;
        default:
            LOGE("Unknown asset type");
            return result;
    }

    if (shgetp_null(map, asset_id)) {
        result = shget(map, asset_id);
    }
    return result;
}

void *asset_store_get_asset_ptr_null(asset_store_t *asset_store, const char *asset_id, asset_type_e type)
{
    void *result = NULL;
    uint32_t index = asset_store_get_asset_index(asset_store, asset_id, type);
    if (index != UINT32_MAX) {
        switch (type) 
        {
            case ASSET_TYPE_TEXTURE:
                result = bulk_data_getp_null_texture_t(asset_store->textures, index);
                break;
            case ASSET_TYPE_SKINNED_MODEL:
                result = bulk_data_getp_null_skinned_model_t(asset_store->skinned_models, index);
                break;
            default:
                assert(false && "This should not happen");
        }
    }
    return result;
}

