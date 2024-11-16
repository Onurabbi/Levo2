#include <vulkan_buffer.h>
#include <vulkan_common.h>

#include <assert.h>

void create_vulkan_buffer(vulkan_buffer_t *buffer, 
                          vulkan_context_t *context,
                          VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(context->logical_device, &bufferInfo, NULL, &buffer->buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context->logical_device, buffer->buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    get_memory_type(&context->memory_properties, memRequirements.memoryTypeBits, properties, &allocInfo.memoryTypeIndex);
    VK_CHECK(vkAllocateMemory(context->logical_device, &allocInfo, NULL, &buffer->memory));

    vkBindBufferMemory(context->logical_device, buffer->buffer, buffer->memory, 0);    

    buffer->buffer_size = size;
}

