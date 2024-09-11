#include "vulkan_texture.h"
#include "vulkan_renderer.h"
#include "vulkan_buffer.h"
#include "vulkan_common.h"

#include "../memory/memory.h"
#include "../logger/logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <ktx.h>
#include <ktxvulkan.h>

static void copy_buffer_to_image(VkDevice device,
                                 VkCommandPool cmd_pool,
                                 VkQueue queue,
                                 VkBuffer buffer, 
                                 VkImage image, 
                                 uint32_t w, 
                                 uint32_t h,
                                 VkBufferImageCopy *buffer_copy_regions,
                                 uint32_t buffer_copy_region_count)
{
    VkCommandBuffer commandBuffer = begin_command_buffer(device, cmd_pool);
    vkCmdCopyBufferToImage(commandBuffer, 
                           buffer, 
                           image, 
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                           buffer_copy_region_count, 
                           buffer_copy_regions);
    end_command_buffer(&commandBuffer, device, cmd_pool, queue);
}

static void transition_image_layout(VkDevice device,
                                    VkCommandPool pool,
                                    VkQueue queue,
                                    VkImage image,
                                    VkFormat format,
                                    VkImageLayout old_layout,
                                    VkImageLayout new_layout,
                                    uint32_t mip_levels)
{
    VkCommandBuffer commandBuffer = begin_command_buffer(device, pool);

    VkImageMemoryBarrier barrier = {};
    barrier.sType     = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout,
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destinationStage = 0;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && 
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, 
                         sourceStage,
                         destinationStage,
                         0, 
                         0, NULL,
                         0, NULL,
                         1, &barrier);

    end_command_buffer(&commandBuffer, device, pool, queue); 
}

static void create_descriptor_sets(vulkan_texture_t *tex, vulkan_renderer_t *renderer)
{
    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {renderer->descriptor_set_layout, renderer->descriptor_set_layout};
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = renderer->descriptor_pool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;
    
    VK_CHECK(vkAllocateDescriptorSets(renderer->logical_device, &allocInfo, tex->descriptor_sets));

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorImageInfo imageInfos;
        imageInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos.imageView = tex->view;
        imageInfos.sampler   = tex->sampler;

        VkWriteDescriptorSet descriptorWrites[1];
        memset(descriptorWrites, 0, sizeof(descriptorWrites));
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = tex->descriptor_sets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfos;

        vkUpdateDescriptorSets(renderer->logical_device, 1, descriptorWrites, 0, NULL);
    }
}

static void create_image(vulkan_texture_t *texture,
                         VkDevice device,
                         VkFormat format, 
                         VkImageTiling tiling,
                         VkImageUsageFlags usage,
                         VkPhysicalDeviceMemoryProperties *memProperties,
                         VkMemoryPropertyFlags properties)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = texture->w;
    imageInfo.extent.height = texture->h;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = texture->mip_levels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateImage(device, &imageInfo, NULL, &texture->image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, texture->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    get_memory_type(memProperties, memRequirements.memoryTypeBits, properties, &allocInfo.memoryTypeIndex);

    VK_CHECK(vkAllocateMemory(device, &allocInfo, NULL, &texture->memory));
    vkBindImageMemory(device, texture->image, texture->memory, 0);
}

void vulkan_texture_from_buffer(vulkan_texture_t *texture, 
                                vulkan_renderer_t *renderer, 
                                void *pixels, 
                                uint32_t w, 
                                uint32_t h, 
                                uint32_t num_comp,
                                uint32_t mip_levels)
{
    VkDeviceSize image_size = w * h * 4;

    texture->w = w;
    texture->h = h;
    texture->mip_levels = mip_levels;

    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    
    vulkan_buffer_t staging_buffer;
    create_vulkan_buffer(&staging_buffer,
                         renderer, 
                         image_size, 
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data;
    vkMapMemory(renderer->logical_device, staging_buffer.memory, 0, image_size, 0, &data);
#if 1
    if (num_comp == 1)
    {
        uint8_t *dst_pixels = (uint8_t*)data;
        uint8_t *src_pixels = (uint8_t*)pixels;

        for (uint32_t row = 0; row < h; row++)
        {
            for (uint32_t col = 0; col < w; col++)
            {
                uint8_t red = *src_pixels++;
                *dst_pixels++ = red;
                *dst_pixels++ = red;
                *dst_pixels++ = red;
                *dst_pixels++ = red ? 0xFF : 0;
            }
        }
    }
    else if (num_comp == 3)
    {
        uint8_t *dst_pixels = (uint8_t*)data;
        uint8_t *src_pixels = (uint8_t*)pixels;

        for (uint32_t row = 0; row < h; row++)
        {
            for (uint32_t col = 0; col < w; col++)
            {
                *dst_pixels++ = *src_pixels++;
                *dst_pixels++ = *src_pixels++;
                *dst_pixels++ = *src_pixels++;
                *dst_pixels++ = 0xFF;
            }
        }
    }
    else
    {
#endif
        memcpy(data, pixels, image_size);
#if 1
    }
#endif  
    vkUnmapMemory(renderer->logical_device, staging_buffer.memory);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (VkOffset3D){0,0,0};
    region.imageExtent = (VkExtent3D){w,h,1};
    
    create_image(texture,
                 renderer->logical_device,
                 format, 
                 VK_IMAGE_TILING_OPTIMAL, 
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 &renderer->device_memory_properties, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    transition_image_layout(renderer->logical_device,
                          renderer->command_pool,
                          renderer->graphics_queue,
                          texture->image,
                          format,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          texture->mip_levels);
    
    copy_buffer_to_image(renderer->logical_device,
                      renderer->command_pool,
                      renderer->graphics_queue,
                      staging_buffer.buffer,
                      texture->image,
                      w,h, &region, 1);

    transition_image_layout(renderer->logical_device,
                          renderer->command_pool,
                          renderer->graphics_queue,
                          texture->image,
                          format,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          texture->mip_levels);

    vkDestroyBuffer(renderer->logical_device, staging_buffer.buffer,NULL);
    vkFreeMemory(renderer->logical_device, staging_buffer.memory, NULL);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = texture->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format   = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mip_levels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(renderer->logical_device, &viewInfo, NULL, &texture->view));

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = renderer->device_properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VK_CHECK(vkCreateSampler(renderer->logical_device, &samplerInfo, NULL, &texture->sampler));
}


bool vulkan_texture_from_file(vulkan_texture_t *texture, vulkan_renderer_t *renderer, const char *file_path)
{
    int w,h,channels;
    stbi_uc *pixels = stbi_load(file_path, &w, &h, &channels, 0);
    if (!pixels)
    {
        LOGE("Failed to load image");
        return false;
    }

    //uint32_t mip_levels = (uint32_t)floorf(log2f((float)MAX(w,h)));

    vulkan_texture_from_buffer(texture,renderer, pixels, w, h, channels, 1);
    stbi_image_free(pixels);
    create_descriptor_sets(texture, renderer);
    return true;
}


bool vulkan_ktx_texture_from_file(vulkan_texture_t *texture, vulkan_renderer_t *renderer, const char *file_path)
{
    ktxTexture *ktx_texture;
    ktxResult result = ktxTexture_CreateFromNamedFile(file_path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
    assert(result == KTX_SUCCESS);

    texture->w = ktx_texture->baseWidth;
    texture->h = ktx_texture->baseHeight;
    texture->mip_levels = ktx_texture->numLevels;

    ktx_uint8_t *ktx_texture_data = ktxTexture_GetData(ktx_texture);
    ktx_size_t ktx_texture_size = ktxTexture_GetDataSize(ktx_texture);

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    vulkan_buffer_t staging_buffer;
    create_vulkan_buffer(&staging_buffer, 
                         renderer, 
                         ktx_texture_size,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void *data;
    vkMapMemory(renderer->logical_device, staging_buffer.memory, 0, ktx_texture_size, 0, (void**)&data);
    memcpy(data, ktx_texture_data, ktx_texture_size);
    vkUnmapMemory(renderer->logical_device, staging_buffer.memory);

    //max texture size 4096 * 4096 => 12 mip levels
    VkBufferImageCopy *buffer_copy_regions = memory_alloc(sizeof(VkBufferImageCopy) * texture->mip_levels, MEM_TAG_TEMP);
    uint32_t buffer_copy_region_count = 0;
    for (uint32_t i = 0; i < texture->mip_levels; i++)
    {
        ktx_size_t offset;
        KTX_error_code result = ktxTexture_GetImageOffset(ktx_texture, i, 0, 0, &offset);
        assert(result == KTX_SUCCESS);

        VkBufferImageCopy *buffer_copy_region = &buffer_copy_regions[buffer_copy_region_count++];
        buffer_copy_region->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        buffer_copy_region->imageSubresource.mipLevel   = i;
        buffer_copy_region->imageSubresource.baseArrayLayer = 0;
        buffer_copy_region->imageSubresource.layerCount = 1;
        buffer_copy_region->imageExtent.width = MAX(1U,ktx_texture->baseWidth >> i);
        buffer_copy_region->imageExtent.height = MAX(1U, ktx_texture->baseHeight >> i);
        buffer_copy_region->imageExtent.depth = 1;
        buffer_copy_region->bufferOffset = offset;
    }

    create_image(texture,
                 renderer->logical_device,
                 format, 
                 VK_IMAGE_TILING_OPTIMAL, 
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 &renderer->device_memory_properties, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    transition_image_layout(renderer->logical_device,
                          renderer->command_pool,
                          renderer->graphics_queue,
                          texture->image,
                          format,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          texture->mip_levels);
    copy_buffer_to_image(renderer->logical_device,
                      renderer->command_pool,
                      renderer->graphics_queue,
                      staging_buffer.buffer,
                      texture->image,
                      texture->w,
                      texture->h,
                      buffer_copy_regions,
                      buffer_copy_region_count);

    transition_image_layout(renderer->logical_device,
                           renderer->command_pool,
                           renderer->graphics_queue,
                           texture->image,
                           format,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           texture->mip_levels);
    
    vkDestroyBuffer(renderer->logical_device, staging_buffer.buffer,NULL);
    vkFreeMemory(renderer->logical_device, staging_buffer.memory, NULL);

    ktxTexture_Destroy(ktx_texture);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = texture->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format   = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = texture->mip_levels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(renderer->logical_device, &viewInfo, NULL, &texture->view));

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = renderer->device_properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VK_CHECK(vkCreateSampler(renderer->logical_device, &samplerInfo, NULL, &texture->sampler));

    create_descriptor_sets(texture, renderer);
    
    return true;
}
