#ifndef VULKAN_TYPES_H_
#define VULKAN_TYPES_H_

#include <vulkan/vulkan.h>
#include "../../containers/container_types.h"
#include "../../math/math_types.h"

#define MAX_FRAMES_IN_FLIGHT             2
#define MAX_TEXTURE_COUNT                128
//! One combined image sampler per texture
#define MAX_COMBINED_IMAGE_SAMPLER_COUNT 512
//! One ssbo per skin
#define MAX_SSBO_COUNT                   256

#define MAX_SPRITES_PER_BATCH            1024

#define VK_CHECK(fn) (assert((fn) == VK_SUCCESS))

typedef struct
{
    mat4f_t projection;
    mat4f_t view;
    //model matrix is a push constant
}uniform_data_t;

typedef struct 
{
    VkDescriptorSet descriptor_set;
    VkBuffer        buffer;
    VkDeviceMemory  memory;
    VkDeviceSize    buffer_size;
    void           *mapped;
}vulkan_buffer_t;

enum 
{
    SCENE_MATRIX_LAYOUT,
    JOINT_MATRIX_LAYOUT,
    TEXTURE_LAYOUT
};

typedef struct
{
    VkDescriptorSetLayout layouts[3];

    VkPipelineLayout      pipeline_layout;
    VkPipeline            pipeline;
    VkPipelineCache       pipeline_cache;
}vulkan_shader_t;

typedef struct
{
    uint32_t        w,h;
    uint32_t        mip_levels;
    VkImage         image;
    VkImageView     view;
    VkSampler       sampler;
    VkDeviceMemory  memory;
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

BulkDataTypes(vulkan_texture_t);
/**
 * @brief Vulkan rendering backend internal context
 */
typedef struct
{
    //instance (required) api version
    uint32_t         api_major;
    uint32_t         api_minor;
    uint32_t         api_patch;        
    
    //device api (supported by physical gpu) version
    uint32_t         device_api_major;
    uint32_t         device_api_minor;
    
    VkInstance       instance;
    VkPhysicalDevice physical_device;
    VkDevice         logical_device;

    uint32_t         graphics_family;
    uint32_t         compute_family;
    uint32_t         transfer_family;

    VkQueue          graphics_queue;
    VkCommandPool    graphics_command_pool;

    VkFormat         color_format;
    VkColorSpaceKHR  color_space;
    
    VkFormat         depth_format;
    VkImage          depth_image;
    VkImageView      depth_image_view;
    VkDeviceMemory   depth_memory;

    VkSurfaceKHR     window_surface;
    VkSwapchainKHR   swapchain;
    VkExtent2D       swapchain_extent;

    VkDescriptorPool descriptor_pool;
    
    //buffers and descriptors for projviewmodel matrices (one per frame in flight)
    vulkan_buffer_t  *uniform_buffers;

    VkPhysicalDeviceProperties       properties;
    VkPhysicalDeviceFeatures         features;
    VkPhysicalDeviceMemoryProperties memory_properties;

    const char *device_extension_names[6];
    uint32_t    device_extension_name_count;

    //swapchain
    VkImage          *swapchain_images;
    VkImageView      *swapchain_image_views;
    uint32_t          swapchain_image_count;
        
    /**
     * @brief: per frame resources
     */
    //sync objects
    VkSemaphore      *render_complete_semaphores;
    VkSemaphore      *present_complete_semaphores;
    VkFence          *wait_fences;

    vulkan_buffer_t  *render_buffers;
    VkCommandBuffer  *command_buffers;

    uint32_t         max_frames_in_flight;
    uint32_t         current_frame; //for command buffers, fences etc.
    uint32_t         image_index; //next swapchain image index
    bool             validation_enabled;

    bulk_data_vulkan_texture_t textures;
}vulkan_context_t;


#endif
