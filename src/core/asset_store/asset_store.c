#include "asset_store.h"

#include "../renderer/frontend/renderer.h"
#include "../containers/containers.h"

#include "../logger/logger.h"
#include "../string/string.h"

#define STB_DS_IMPLEMENTATION
#include <stb/stb_ds.h>

#include <assert.h>
#include "json_loader.c"

void asset_store_init(asset_store_t *asset_store)
{
    bulk_data_init(&asset_store->textures, texture_t);
}

void asset_store_add_texture(asset_store_t     *store,
                             renderer_t *renderer, 
                             const char *asset_id, 
                             const char *file_path)
{
    if ((shgetp_null(store->texture_map, asset_id)) == NULL) {
        texture_t texture = {0};
        if (!renderer_create_texture(renderer, &texture, file_path)) {
            LOGE("Unable to load texture from file: %s", file_path);
            return;
        }
        
        uint32_t slot = bulk_data_push(&store->textures);
        bulk_data_seti(&store->textures, slot, texture);
        shput(store->texture_map, asset_id, slot);
    }
}

texture_t *asset_store_get_texture(asset_store_t *store, const char* asset_id)
{
    if (shgetp_null(store->texture_map, asset_id)) {
        return bulk_data_getp_null(&store->textures, shget(store->texture_map, asset_id));
    }
    return NULL;
}

