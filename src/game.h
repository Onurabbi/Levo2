#ifndef GAME_H_
#define GAME_H_

#include "core/asset_store/asset_store.h"
#include "core/vulkan_renderer/vulkan_renderer.h"
#include "game_types.h"

void game_init(game_t *game);
void game_run(game_t *game);
#endif

