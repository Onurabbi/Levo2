#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_

#include <vulkan_types.h>

void create_vulkan_buffer(vulkan_buffer_t *buffer, 
                           vulkan_context_t *context,
                           VkDeviceSize size,
                           VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties);
                           
#endif
