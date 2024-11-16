#ifndef VULKAN_COMMON_H_
#define VULKAN_COMMON_H_

#include <vulkan_types.h>
#include <stdbool.h>

bool get_memory_type(VkPhysicalDeviceMemoryProperties *memProperties,
                     uint32_t typeFilter, 
                     VkMemoryPropertyFlags properties, 
                     uint32_t *result);

VkCommandBuffer begin_command_buffer(VkDevice device, VkCommandPool cmd_pool);
void end_command_buffer(VkCommandBuffer *buffer, VkDevice device, VkCommandPool pool, VkQueue queue);
#endif

