#ifndef VULKAN_TYPES_H_
#define VULKAN_TYPES_H_

#include <vulkan/vulkan.h>
#include "../containers/container_types.h"
#include "../math/math_types.h"

#define MAX_FRAMES_IN_FLIGHT   2
#define MAX_TEXTURE_COUNT      128
#define MAX_SPRITES_PER_BATCH  1024

#define VK_CHECK(fn) (assert((fn) == VK_SUCCESS))

typedef struct 
{
    VkBuffer        buffer;
    VkDeviceMemory  memory;
    VkDeviceSize    buffer_size;
    void           *mapped;
}vulkan_buffer_t;

typedef struct
{
    uint32_t        w,h;
    uint32_t        mip_levels;
    VkImage         image;
    VkImageView     view;
    VkSampler       sampler;
    VkDeviceMemory  memory;
    VkDescriptorSet descriptor_sets[MAX_FRAMES_IN_FLIGHT];
}vulkan_texture_t;

typedef struct 
{
    vulkan_texture_t *tex;
    rect_t            src_rect;
    rect_t            dst_rect;
    uint32_t          z_index;
} drawable_t;

typedef struct
{
    vec2f_t pos;
    vec2f_t tex_coord;
}vertex_t;

typedef struct 
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice logical_device;

    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    VkPhysicalDeviceMemoryProperties device_memory_properties;

    uint32_t graphics_family;
    uint32_t compute_family;
    uint32_t transfer_family;

    VkQueue graphics_queue;

    VkFormat color_format;
    VkColorSpaceKHR color_space;

    VkFormat        depth_format;
    VkImage         depth_image;
    VkImageView     depth_image_view;
    VkDeviceMemory  depth_memory;

    VkExtent2D swapchain_extent;
    VkSwapchainKHR swapchain;
    VkSurfaceKHR surface;

    VkImage *swapchain_images;
    VkImageView *swapchain_image_views;
    uint32_t swapchain_image_count;

    VkFramebuffer *framebuffers;
    uint32_t framebuffer_count;

    VkCommandPool command_pool;
    
    VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore render_complete_semaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore present_complete_semaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence wait_fences[MAX_FRAMES_IN_FLIGHT];
    uint32_t current_frame;
    uint32_t image_index;
    
    VkRenderPass render_pass;

    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_set_layout;
    
    vulkan_buffer_t index_buffer;
    vulkan_buffer_t vertex_buffers[MAX_FRAMES_IN_FLIGHT];
    vulkan_buffer_t uniform_buffers[MAX_FRAMES_IN_FLIGHT];

    vulkan_texture_t *textures[MAX_TEXTURE_COUNT];
    uint64_t          offsets[MAX_TEXTURE_COUNT];
    uint32_t          current_texture;

    float tile_width;
    float tile_height;
    
}vulkan_renderer_t;

BulkDataTypes(vulkan_texture_t)

#endif
