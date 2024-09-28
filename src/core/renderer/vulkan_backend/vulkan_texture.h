#ifndef VULKAN_TEXTURE_H_
#define VULKAN_TEXTURE_H_

#include "vulkan_types.h"

bool vulkan_texture_from_file(vulkan_texture_t *texture, vulkan_context_t *renderer, const char *file_path);
bool vulkan_ktx_texture_from_file(vulkan_texture_t *texture, vulkan_context_t *renderer, const char *file_path);
void vulkan_texture_from_buffer(vulkan_texture_t *texture, 
                                vulkan_context_t *renderer, 
                                void *pixels, 
                                uint32_t w, 
                                uint32_t h, 
                                uint32_t channels,
                                uint32_t mip_levels);
#endif

