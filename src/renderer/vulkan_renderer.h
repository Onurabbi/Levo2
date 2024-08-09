#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "vulkan_types.h"
#include "../game/game_types.h"

#include <SDL2/SDL.h>

void vulkan_renderer_init(vulkan_renderer_t *renderer, SDL_Window *window);
void vulkan_renderer_present(vulkan_renderer_t *renderer);
void vulkan_renderer_copy_texture(vulkan_renderer_t *renderer, 
                                  vulkan_texture_t *texture,
                                  rect_t *src,
                                  rect_t *dst);
void vulkan_renderer_render_entities(vulkan_renderer_t *renderer, 
                                     bulk_data_entity_t *entities, 
                                     bulk_data_animation_t *animations, 
                                     bulk_data_sprite_t *sprites, 
                                     rect_t camera);
#endif
