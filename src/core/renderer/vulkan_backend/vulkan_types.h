#ifndef VULKAN_TYPES_H_
#define VULKAN_TYPES_H_

#include <vulkan/vulkan.h>
#include "../../containers/bulk_data_types.h"
#include "../../math/math_types.h"

#define MAX_FRAMES_IN_FLIGHT             2
#define MAX_TEXTURE_COUNT                128
//! One combined image sampler per texture
#define MAX_COMBINED_IMAGE_SAMPLER_COUNT 512
//! One ssbo per skin
#define MAX_SSBO_COUNT                   256

#define MAX_SPRITES_PER_BATCH            1024
#define MAX_SWAPCHAIN_IMAGE_COUNT        4

#define VK_CHECK(fn) (assert((fn) == VK_SUCCESS))

typedef struct
{
    mat4f_t projection;
    mat4f_t view;
    //model matrix is a push constant
    vec4f_t light_pos;
}scene_uniform_data_t;

/**
 *  @brief: this contains vulkan-specific data that belongs to the skinned model.
 *          One descriptor set per active frame, and indices of joint matrices and the sampled texture
 */
typedef struct 
{
    //! @brief one per frame
    VkDescriptorSet  descriptor_sets[3];
    //! @brief one per frame
    uint32_t         ssbo_indices[3];
    //! @brief same texture index will be used for all three buffers
    uint32_t         texture_index;
}vulkan_skinned_model_render_data_t;

/**
 * @brief: This contains vulkan-specific data for the scene uniforms
 *         one descriptor set per active frame and the index of the uniform buffer.
 */
typedef struct
{
    //! @brief one per frame
    VkDescriptorSet  descriptor_sets[3];
    //! @brief one per frame
    uint32_t         buffer_indices[3];
}vulkan_uniform_buffer_render_data_t;

typedef struct
{
    //one layout for globals, one for instances
    VkDescriptorSetLayout layouts[2];
    uint32_t              layout_count;
    
    VkPipelineLayout      pipeline_layout;
    VkPipeline            pipeline;
    VkPipelineCache       pipeline_cache;
}vulkan_shader_t;

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

    VkPhysicalDeviceProperties       properties;
    VkPhysicalDeviceFeatures         features;
    VkPhysicalDeviceMemoryProperties memory_properties;

    const char *device_extension_names[6];
    uint32_t    device_extension_name_count;

    //swapchain
    VkImage          swapchain_images[4];
    VkImageView      swapchain_image_views[4];
    uint32_t         swapchain_image_count;
        
    /**
     * @brief: per frame resources
     */
    //sync objects
    VkSemaphore      *render_complete_semaphores;
    VkSemaphore      *present_complete_semaphores;
    VkFence          *wait_fences;

    VkCommandBuffer  *command_buffers;

    uint32_t         max_frames_in_flight;
    uint32_t         current_frame; //for command buffers, fences etc.
    uint32_t         image_index; //next swapchain image index
    bool             validation_enabled;

    bulk_data_vulkan_texture_t textures;
    bulk_data_vulkan_buffer_t  buffers;
}vulkan_context_t;


#endif
