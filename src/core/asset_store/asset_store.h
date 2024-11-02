#ifndef ASSET_STORE_H_
#define ASSET_STORE_H_

#include "../renderer/frontend/renderer_types.h"
#include "../containers/containers.h"
#include "asset_types.h"

void asset_store_init(asset_store_t *asset_store, bulk_data_texture_t *textures, bulk_data_skinned_model_t *skinned_models);
void asset_store_add_texture(asset_store_t *asset_store, renderer_t *renderer, const char *asset_id, const char *file_path);
void asset_store_add_skinned_model(asset_store_t *asset_store, renderer_t *renderer, const char *asset_id, const char *file_path, bulk_data_renderbuffer_t *renderbuffers);
uint32_t asset_store_get_asset_index(asset_store_t *asset_store, const char *asset_id, asset_type_e type);
void *asset_store_get_asset_ptr_null(asset_store_t *asset_store, const char *asset_id, asset_type_e type);
#endif

