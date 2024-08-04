#include "asset_store.h"
#include "../renderer/vulkan_texture.h"
#include "../logger/logger.h"

#include <stb/stb_ds.h>

void asset_store_init(asset_store_t *asset_store)
{
    bulk_data_init(&asset_store->textures, vulkan_texture_t);
}

vulkan_texture_t *asset_store_add_texture(asset_store_t     *store,
                                       vulkan_renderer_t *renderer, 
                                       const char *asset_id, 
                                       const char *file_path)
{
    uint32_t slot;
    if ((shgetp_null(store->texture_map, asset_id)) == NULL)
    {
        vulkan_texture_t texture = {0};
        vulkan_texture_from_file(&texture, renderer, file_path);

        slot = bulk_data_push(&store->textures);
        bulk_data_seti(&store->textures, slot, texture);
        shput(store->texture_map, asset_id, slot);
    }
    else
    {
        slot = shget(store->texture_map, asset_id);
    }
    return bulk_data_getp_null(&store->textures, slot);
}

vulkan_texture_t *asset_store_get_texture(asset_store_t *store, const char* asset_id)
{
    if (shgetp_null(store->texture_map, asset_id))
    {
        return bulk_data_getp_null(&store->textures, shget(store->texture_map, asset_id));
    }
    return NULL;
}

