#include <vulkan_common.h>
#include <assert.h>

bool get_memory_type(VkPhysicalDeviceMemoryProperties *memProperties,
                     uint32_t typeFilter, 
                     VkMemoryPropertyFlags properties, 
                     uint32_t *result)
{
    for (uint32_t i = 0; i < memProperties->memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties->memoryTypes[i].propertyFlags & properties) == properties) {
            *result = i;
            return true;
        }
    }
    *result = UINT32_MAX;
    return false;
}

VkCommandBuffer begin_command_buffer(VkDevice device, VkCommandPool cmd_pool)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmd_pool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer result;
    VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &result));

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(result,&beginInfo));
    return result;
}

void end_command_buffer(VkCommandBuffer *buffer, VkDevice device, VkCommandPool pool, VkQueue queue)
{
    vkEndCommandBuffer(*buffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, pool, 1, buffer);
}

