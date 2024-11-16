#ifndef SKINNED_MODEL_H_
#define SKINNED_MODEL_H_

#include <asset_types.h>

bool skinned_model_create(skinned_model_t *skinned_model, const char *file_path, const char *asset_id, renderer_t *renderer, asset_store_t *asset_store);
void skinned_model_update_animation(skinned_model_t *model, renderer_t *renderer, float dt);
void skinned_model_draw(skinned_model_t *model, renderer_t *renderer, shader_t *shader);

#endif

