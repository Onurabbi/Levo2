#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "vulkan_types.h"
#include "../game/game_types.h"

#include <SDL2/SDL.h>

void vulkan_renderer_init(vulkan_renderer_t *renderer, SDL_Window *window, float tile_width, float tile_height);
void vulkan_renderer_render(vulkan_renderer_t *renderer, 
                            game_t *game);
#endif
