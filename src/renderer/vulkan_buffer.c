#include "vulkan_buffer.h"
#include "vulkan_common.h"

#include <assert.h>

void create_vulkan_buffer(vulkan_buffer_t *buffer, 
                          vulkan_renderer_t *renderer,
                          VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(renderer->logical_device, &bufferInfo, NULL, &buffer->buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(renderer->logical_device, buffer->buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    get_memory_type(&renderer->device_memory_properties, memRequirements.memoryTypeBits, properties, &allocInfo.memoryTypeIndex);
    VK_CHECK(vkAllocateMemory(renderer->logical_device, &allocInfo, NULL, &buffer->memory));

    vkBindBufferMemory(renderer->logical_device, buffer->buffer, buffer->memory, 0);    
}