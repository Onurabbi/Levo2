#ifndef ASSET_STORE_H_
#define ASSET_STORE_H_

#include "../renderer/vulkan_types.h"
#include "../containers/containers.h"

typedef struct 
{
    bulk_data_vulkan_texture_t textures;
    string_hash_entry_t *texture_map;
}asset_store_t;

void asset_store_init(asset_store_t *asset_store);
vulkan_texture_t *asset_store_add_texture(asset_store_t *asset_store, vulkan_renderer_t *renderer, const char *asset_id, const char *file_path);
vulkan_texture_t *asset_store_get_texture(asset_store_t *store, const char* asset_id);
#endif

