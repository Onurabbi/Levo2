#include "vulkan_types.h"
#include "vulkan_backend.h"
#include "vulkan_buffer.h"
#include "vulkan_texture.c"
#include "vulkan_common.c"
#include "vulkan_buffer.c"

#include "../../logger/logger.h"
#include "../../memory/memory.h"
#include "../../utils/utils.h"
#include "../../containers/containers.h"
#include "../../string/string.h"
#include "../../platform/platform.h"

#include "../../../game_types.h"
#include "../../../systems/collision.h" 
#include "../../../systems/animation.h"

#include <SDL2/SDL_vulkan.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

static VkShaderModule create_shader_module(VkDevice device, const char* filePath)
{
    long fsize;
    uint8_t *data = read_whole_file(filePath, &fsize, MEM_TAG_TEMP);
    
    VkShaderModule result;
    VkShaderModuleCreateInfo shaderModule = {};
    shaderModule.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModule.flags    = 0;
    shaderModule.pNext    = NULL;
    shaderModule.codeSize = (size_t)fsize;
    shaderModule.pCode    = (uint32_t*)data;

    assert(vkCreateShaderModule(device, &shaderModule, NULL, &result) == VK_SUCCESS);
    return result;
}

static void create_vulkan_instance(vulkan_context_t *ctx, window_t *window)
{
    assert(window);

    uint32_t api_version = 0;
    vkEnumerateInstanceVersion(&api_version);
    ctx->api_major = VK_API_VERSION_MAJOR(api_version);
    ctx->api_minor = VK_API_VERSION_MINOR(api_version);
    ctx->api_patch = VK_API_VERSION_PATCH(api_version);

    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0,1,0,0);
    appInfo.engineVersion = VK_MAKE_API_VERSION(0,1,0,0);
    appInfo.pEngineName = "LeventEngine";
    appInfo.apiVersion = VK_MAKE_API_VERSION(0,ctx->api_major,ctx->api_minor,ctx->api_patch);  

    uint32_t sdl_extension_count = 0;
    SDL_Vulkan_GetInstanceExtensions(window->window, &sdl_extension_count, NULL);
    const char **sdl_extensions = memory_alloc(sizeof(const char*) * sdl_extension_count, MEM_TAG_TEMP);
    assert(sdl_extensions);

    SDL_Vulkan_GetInstanceExtensions(window->window, &sdl_extension_count, sdl_extensions);

    uint32_t validation_layer_count = 0;
    const char **enabled_layer_names = NULL;

    if (ctx->validation_enabled) {
        uint32_t layer_count;
        const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";

        VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, NULL));

        VkLayerProperties *layer_properties = memory_alloc(layer_count * sizeof(VkLayerProperties), MEM_TAG_TEMP);
        assert(layer_properties);

        VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, layer_properties));

        bool validation_layer_found = false;

        for (uint32_t i = 0; i < layer_count; i++) {
            if (strcmp(validation_layer_name, layer_properties[i].layerName) == 0) {
                validation_layer_found = true;
                break;
            }
        }

        if (validation_layer_found) {
            validation_layer_count = 1;
            enabled_layer_names = &validation_layer_name;
        }
    }

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.flags = 0;
    instanceInfo.enabledExtensionCount   = sdl_extension_count;
    instanceInfo.ppEnabledExtensionNames = sdl_extensions;
    instanceInfo.enabledLayerCount = 0;
    instanceInfo.ppEnabledLayerNames = enabled_layer_names;
    instanceInfo.enabledLayerCount = validation_layer_count;

    VK_CHECK(vkCreateInstance(&instanceInfo, NULL, &ctx->instance));
}

static bool select_physical_device(vulkan_context_t *ctx)
{
    uint32_t physical_device_count;
    VK_CHECK(vkEnumeratePhysicalDevices(ctx->instance, &physical_device_count, NULL));

    VkPhysicalDevice *physical_devices = memory_alloc(physical_device_count * sizeof(VkPhysicalDevice), MEM_TAG_TEMP);
    assert(physical_devices);

    VK_CHECK(vkEnumeratePhysicalDevices(ctx->instance, &physical_device_count, physical_devices));

    //fallback device
    bool device_adequate = false;

    for (uint32_t i = 0; i < physical_device_count; i++) {
        //we need a device that supports vulkan 1.3 or at least supports dynamic state by an extension
        //we prefer if it's a discrete gpu
        //properties
        VkPhysicalDeviceProperties2 properties2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        VkPhysicalDeviceDriverProperties driver_properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};
        properties2.pNext = &driver_properties;
        vkGetPhysicalDeviceProperties2(physical_devices[i], &properties2);
        VkPhysicalDeviceProperties properties = properties2.properties;
        
        //features
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

        VkPhysicalDeviceFeatures2 features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamic_state_next = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT};
        features2.pNext = &dynamic_state_next;
        vkGetPhysicalDeviceFeatures2(physical_devices[i], &features2);

        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &mem_properties);
        
        //! TODO: apple gpus are not discrete
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            //! need to support vulkan 1.3 or dynamic state by an extension
            if (VK_API_VERSION_MAJOR(properties.apiVersion) == ctx->api_major &&
                VK_API_VERSION_MINOR(properties.apiVersion) == ctx->api_minor) {
                //good to go
                device_adequate = true;
            } else if (dynamic_state_next.extendedDynamicState) {
                ctx->device_extension_names[ctx->device_extension_name_count++] = VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME;
                ctx->device_extension_names[ctx->device_extension_name_count++] = VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME;
                device_adequate = true;
            } else {
                LOGE("Device does not support dynamic state");
            }
        } else {
            LOGI("Integrated GPU at index %u, skipping", i);
        }

        if (device_adequate) {
            ctx->physical_device = physical_devices[i];
            ctx->device_api_major = VK_API_VERSION_MAJOR(properties.apiVersion);
            ctx->device_api_minor = VK_API_VERSION_MINOR(properties.apiVersion);

            uint32_t queue_family_property_count;
            vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &queue_family_property_count, NULL);

            VkQueueFamilyProperties *queue_family_properties = memory_alloc(queue_family_property_count * sizeof(VkQueueFamilyProperties), MEM_TAG_TEMP);
            vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &queue_family_property_count, queue_family_properties);

            uint32_t graphics_family   = UINT32_MAX;
            uint32_t compute_family    = UINT32_MAX;
            uint32_t transfer_family   = UINT32_MAX;
            uint32_t compute_fallback  = UINT32_MAX;
            uint32_t transfer_fallback = UINT32_MAX;

            for (uint32_t i = 0; i < queue_family_property_count; i++) {
                //get the first one that supports graphics
                if (graphics_family == UINT32_MAX) {
                    if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                        graphics_family = i;
                    }
                }

                //otherwise try to find a dedicated compute queue
                if (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    if (compute_fallback == UINT32_MAX) {
                        compute_fallback = i;
                    }
                    //if dedicated
                    if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) {
                        compute_family = i;
                    }
                }

                if (queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
                    if (transfer_fallback == UINT32_MAX){
                        transfer_fallback = i;
                    }
                }
                if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0){
                    transfer_family = i;
                }
            }

            if (compute_family == UINT32_MAX) compute_family = compute_fallback;
            if (transfer_family == UINT32_MAX) transfer_family = transfer_fallback;

            ctx->graphics_family   = graphics_family;
            ctx->compute_family    = compute_family;
            ctx->transfer_family   = transfer_family;
            ctx->properties        = properties;
            ctx->features          = features;
            ctx->memory_properties = mem_properties;
            
            break;
        }
    }

    return device_adequate;
}

static void create_logical_device(vulkan_context_t *ctx)
{
    VkDeviceQueueCreateInfo queue_create_infos[3];
    uint32_t queue_create_info_count = 0;

    float priority = 1.0f;

    {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = ctx->graphics_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        queue_create_infos[queue_create_info_count++] = queue_info;
    }

    if (ctx->compute_family != ctx->graphics_family) {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = ctx->compute_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        queue_create_infos[queue_create_info_count++] = queue_info;
    }

    if (ctx->transfer_family != ctx->graphics_family &&
        ctx->transfer_family != ctx->compute_family) {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = ctx->transfer_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        queue_create_infos[queue_create_info_count++] = queue_info;
    }

    //the only extension we are using is the swapchain
    ctx->device_extension_names[ctx->device_extension_name_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    VkPhysicalDeviceFeatures2 device_features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR};
    {
        device_features.features.samplerAnisotropy = VK_TRUE;
        //dynamic rendering
        VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_ext = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
        dynamic_rendering_ext.dynamicRendering = VK_TRUE;
        device_features.pNext = &dynamic_rendering_ext;

        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT};
        extended_dynamic_state.extendedDynamicState = VK_TRUE;
        dynamic_rendering_ext.pNext = &extended_dynamic_state;

        VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT};
        descriptor_indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
        extended_dynamic_state.pNext = &descriptor_indexing_features;
    }

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.enabledExtensionCount = ctx->device_extension_name_count;
    device_create_info.ppEnabledExtensionNames = ctx->device_extension_names;
    device_create_info.pEnabledFeatures  = NULL;
    device_create_info.queueCreateInfoCount = queue_create_info_count;
    device_create_info.pQueueCreateInfos    = queue_create_infos;
    device_create_info.pNext = &device_features;

    VK_CHECK(vkCreateDevice(ctx->physical_device, &device_create_info, NULL, &ctx->logical_device));

    if (ctx->api_major == ctx->device_api_major &&
        ctx->api_minor == ctx->device_api_minor) {
        LOGI("This device supports vulkan 1.3");
    } else {
        //! TODO: implement dynamic function rendering here
        LOGI("This device does not support vulkan 1.3. Loading dynamic rendering functions dynamically");
    }
    //queues are created along with the device
    vkGetDeviceQueue(ctx->logical_device, ctx->graphics_family, 0, &ctx->graphics_queue);
}

static void create_command_pool(vulkan_context_t *ctx)
{
    //create command pool
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = ctx->graphics_family;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(ctx->logical_device, &cmdPoolInfo, NULL, &ctx->graphics_command_pool));
}

static bool setup_depth_stencil(vulkan_context_t *ctx)
{
    bool found = false;

    //create depth image
    VkFormat formatList[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (uint32_t i = 0; i < sizeof(formatList)/sizeof(formatList[0]); i++) {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(ctx->physical_device, formatList[i], &formatProps);
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            ctx->depth_format = formatList[i];
            found = VK_TRUE;
            break;
        }
    }

    if (found) {
        VkImageCreateInfo imageCI = {};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.mipLevels = 1;
        imageCI.extent = (VkExtent3D){ctx->swapchain_extent.width, ctx->swapchain_extent.height, 1};
        imageCI.format = ctx->depth_format;
        imageCI.arrayLayers = 1;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        VK_CHECK(vkCreateImage(ctx->logical_device, &imageCI, NULL, &ctx->depth_image));

        VkMemoryRequirements memReqs = {};
        vkGetImageMemoryRequirements(ctx->logical_device, ctx->depth_image, &memReqs);

        uint32_t memTypeIndex;
        assert(get_memory_type(&ctx->memory_properties, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memTypeIndex));

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = memTypeIndex;
        VK_CHECK(vkAllocateMemory(ctx->logical_device, &allocInfo, NULL, &ctx->depth_memory));
        VK_CHECK(vkBindImageMemory(ctx->logical_device, ctx->depth_image, ctx->depth_memory, 0));

        VkImageViewCreateInfo imageViewCI = {};
        imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCI.image = ctx->depth_image;
        imageViewCI.format = ctx->depth_format;
        imageViewCI.subresourceRange.baseMipLevel = 0;
        imageViewCI.subresourceRange.levelCount = 1;
        imageViewCI.subresourceRange.baseArrayLayer = 0;
        imageViewCI.subresourceRange.layerCount = 1;
        imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        VK_CHECK(vkCreateImageView(ctx->logical_device, &imageViewCI, NULL, &ctx->depth_image_view));  
    }
   
    return found;
}

static bool create_swapchain(vulkan_context_t *ctx, window_t *window)
{
    //create surface first
    if (!SDL_Vulkan_CreateSurface(window->window, ctx->instance, &ctx->window_surface)) {
        LOGE("Unable to create vulkan surface");
        return false;
    }

    //check present
    VkBool32 present_support = false;

    vkGetPhysicalDeviceSurfaceSupportKHR(ctx->physical_device, ctx->graphics_family, ctx->window_surface, &present_support);
    if (present_support) {
        
    } else {
        LOGE("Graphics family doesn't support present :(");
        return false;
    }

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_device, ctx->window_surface, &format_count, NULL);

    VkSurfaceFormatKHR *surface_formats = memory_alloc(sizeof(VkSurfaceFormatKHR) * format_count, MEM_TAG_TEMP);
    assert(surface_formats);

    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_device, ctx->window_surface, &format_count, surface_formats);

    ctx->color_format = surface_formats[0].format;
    ctx->color_space  = surface_formats[0].colorSpace;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physical_device, ctx->window_surface, &surfaceCapabilities);

    ctx->swapchain_extent = surfaceCapabilities.currentExtent;

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physical_device, ctx->window_surface, &present_mode_count, NULL);

    VkPresentModeKHR *present_modes = memory_alloc(sizeof(VkPresentModeKHR)*present_mode_count, MEM_TAG_TEMP);
    assert(present_modes);
    
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physical_device, ctx->window_surface, &present_mode_count, present_modes);
    
    //use VK_PRESENT_MODE_FIFO_KHR as fallback
    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    for (uint32_t i = 0; i < present_mode_count; i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchain_present_mode = present_modes[i];
            break;
        }
    }

    //select the first composite alpha available.
    VkCompositeAlphaFlagBitsKHR composite_alpha_bits[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };

    VkCompositeAlphaFlagBitsKHR selected_composite_alpha = 0;
    for (uint32_t i = 0; i < sizeof(composite_alpha_bits) / sizeof(composite_alpha_bits[0]); i++) {
        if (surfaceCapabilities.supportedCompositeAlpha & composite_alpha_bits[i]) {
            selected_composite_alpha = composite_alpha_bits[i];
            break;
        }
    }

    VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    VkSurfaceTransformFlagBitsKHR pre_transform;
    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        pre_transform = surfaceCapabilities.currentTransform;
    }

    VkSwapchainCreateInfoKHR swapchainCI = {};
    swapchainCI.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.surface = ctx->window_surface;
    swapchainCI.imageColorSpace = ctx->color_space;
    swapchainCI.imageFormat = ctx->color_format;
    swapchainCI.minImageCount = surfaceCapabilities.minImageCount + 1;
    swapchainCI.imageUsage = usage_flags;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.queueFamilyIndexCount = 0;
    swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCI.presentMode = swapchain_present_mode;
    swapchainCI.compositeAlpha = selected_composite_alpha;
    swapchainCI.clipped = VK_TRUE;
    swapchainCI.preTransform = pre_transform;
    swapchainCI.oldSwapchain = VK_NULL_HANDLE;
    swapchainCI.imageExtent = ctx->swapchain_extent;

    VK_CHECK(vkCreateSwapchainKHR(ctx->logical_device, &swapchainCI, NULL, &ctx->swapchain));

    //get swapchain images
    VK_CHECK(vkGetSwapchainImagesKHR(ctx->logical_device, ctx->swapchain, &ctx->swapchain_image_count, NULL));
    ctx->swapchain_images = malloc(ctx->swapchain_image_count * sizeof(VkImage));
    VK_CHECK(vkGetSwapchainImagesKHR(ctx->logical_device, ctx->swapchain, &ctx->swapchain_image_count, ctx->swapchain_images));
    ctx->swapchain_image_views = malloc(ctx->swapchain_image_count * sizeof(VkImageView));

    for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
        VkImageViewCreateInfo viewCI = {};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.components = (VkComponentMapping){
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        };

        viewCI.format   = ctx->color_format;
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.image    = ctx->swapchain_images[i];
        viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.baseMipLevel = 0,
        viewCI.subresourceRange.levelCount = 1;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount = 1;
        viewCI.flags = 0;
        VK_CHECK(vkCreateImageView(ctx->logical_device, &viewCI, NULL, &ctx->swapchain_image_views[i]));
    }

    return true;
}

static void create_sync_primitives(vulkan_context_t *ctx)
{
    ctx->max_frames_in_flight = ctx->swapchain_image_count - 1;
    ctx->current_frame = 0;

    ctx->render_complete_semaphores = memory_alloc(ctx->max_frames_in_flight * sizeof(VkSemaphore), MEM_TAG_PERMANENT);
    ctx->present_complete_semaphores = memory_alloc(ctx->max_frames_in_flight * sizeof(VkSemaphore), MEM_TAG_PERMANENT);
    ctx->wait_fences = memory_alloc(ctx->max_frames_in_flight * sizeof(VkFence), MEM_TAG_PERMANENT);

    for (uint32_t i = 0; i < ctx->max_frames_in_flight; i++) {
        VkSemaphoreCreateInfo sem_info = {0};
        sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        
        VkFenceCreateInfo fence_info = {0};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_CHECK(vkCreateSemaphore(ctx->logical_device, &sem_info, NULL, &ctx->render_complete_semaphores[i]));
        VK_CHECK(vkCreateSemaphore(ctx->logical_device, &sem_info, NULL, &ctx->present_complete_semaphores[i]));
        VK_CHECK(vkCreateFence(ctx->logical_device, &fence_info, NULL, &ctx->wait_fences[i]));
    }
}

static void create_command_buffers(vulkan_context_t *context)
{
    context->command_buffers = memory_alloc(context->max_frames_in_flight * sizeof(VkCommandBuffer), MEM_TAG_PERMANENT);

    VkCommandBufferAllocateInfo cmd_buf_info = {0};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_info.commandPool = context->graphics_command_pool;
    cmd_buf_info.commandBufferCount = context->max_frames_in_flight;
    cmd_buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK_CHECK(vkAllocateCommandBuffers(context->logical_device, &cmd_buf_info, context->command_buffers));
}

static void create_descriptor_pool(vulkan_context_t *context)
{
    VkDescriptorPoolSize pool_sizes[3];
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[0].descriptorCount = MAX_COMBINED_IMAGE_SAMPLER_COUNT * context->max_frames_in_flight;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_sizes[1].descriptorCount = MAX_SSBO_COUNT * context->max_frames_in_flight;
    pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[2].descriptorCount = context->max_frames_in_flight; //one uniform buffer per frame in flight
    
    VkDescriptorPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
    pool_info.pPoolSizes = pool_sizes;
    pool_info.maxSets = MAX_COMBINED_IMAGE_SAMPLER_COUNT + MAX_SSBO_COUNT + 2; //one uniform buffer per frame in flight
    VK_CHECK(vkCreateDescriptorPool(context->logical_device, &pool_info, NULL, &context->descriptor_pool));
}

uint32_t vulkan_backend_get_number_of_frames_in_flight(renderer_backend_t *backend)
{
    vulkan_context_t *context = (vulkan_context_t*)backend->internal_context;
    return context->max_frames_in_flight;
}

bool vulkan_backend_initialize(renderer_backend_t *backend, window_t *window, renderer_config_t *config)
{
    backend->internal_context = memory_alloc(sizeof(vulkan_context_t), MEM_TAG_PERMANENT);

    vulkan_context_t *ctx = (vulkan_context_t *)backend->internal_context;
    bulk_data_init(&ctx->textures, vulkan_texture_t);

    if (config->flags & RENDERER_CONFIG_FLAG_ENABLE_VALIDATION) {
        ctx->validation_enabled = true;
    }

    create_vulkan_instance(ctx, window);
    if (!select_physical_device(ctx)) {
        LOGE("Unable to select physical device");
        return false;
    }

    create_logical_device(ctx);
    create_command_pool(ctx);

    if (!create_swapchain(ctx, window)) {
        LOGE("Unable to create swapchain.");
        return false;
    }

    if(!setup_depth_stencil(ctx)) {
        LOGE("No suitable format found for depth stencil attachments");
        return false;
    }

    create_sync_primitives(ctx);
    create_descriptor_pool(ctx);
    create_command_buffers(ctx);
    return true;
}

void vulkan_backend_shutdown(struct renderer_backend_t *backend)
{
    vulkan_context_t *context = (vulkan_context_t *)backend->internal_context;
    vkDeviceWaitIdle(context->logical_device);

    //destroy device
    context->graphics_queue = NULL;
    vkDestroyCommandPool(context->logical_device, context->graphics_command_pool, NULL);
    vkDestroyDevice(context->logical_device, NULL);
    context->logical_device = NULL;
    context->physical_device = NULL;

    vkDestroyInstance(context->instance, NULL);
    LOGI("vulkan backend shutdown");
}

void vulkan_backend_window_destroy(struct renderer_backend_t *backend, window_t *window)
{
    vulkan_context_t *context = (vulkan_context_t *)backend->internal_context;
    (void)context;
    LOGI("vulkan backend window destroy");
}

bool vulkan_backend_frame_prepare(struct renderer_backend_t *backend, frame_data_t *frame_data)
{
    vulkan_context_t *context = (vulkan_context_t *)backend->internal_context;
    vkWaitForFences(context->logical_device, 1, &context->wait_fences[context->current_frame], VK_TRUE, UINT64_MAX);
    VK_CHECK(vkResetFences(context->logical_device, 1, &context->wait_fences[context->current_frame]));
    VK_CHECK(vkAcquireNextImageKHR(context->logical_device, 
                                   context->swapchain, 
                                   UINT64_MAX, 
                                   context->present_complete_semaphores[context->current_frame],
                                   VK_NULL_HANDLE,
                                   &context->image_index));
    return true;
}

bool vulkan_backend_begin_rendering(struct renderer_backend_t *backend)
{
    vulkan_context_t *context = (vulkan_context_t *)backend->internal_context;
    VkCommandBufferBeginInfo cmd_buf_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_CHECK(vkBeginCommandBuffer(context->command_buffers[context->current_frame], &cmd_buf_info));

    //insert memory barriers for color and depth images
    //color image
    {
        VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.image = context->swapchain_images[context->image_index];
        barrier.subresourceRange = (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCmdPipelineBarrier(context->command_buffers[context->current_frame], 
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             0,
                             0, NULL,
                             0, NULL,
                             1, &barrier);
    }
    //depth image
    {
        VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.image = context->depth_image;
        barrier.subresourceRange = (VkImageSubresourceRange){VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1};

        vkCmdPipelineBarrier(context->command_buffers[context->current_frame],
                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                             0, 
                             0, NULL,
                             0, NULL,
                             1, &barrier);
    }

    //define attachments used in dynamic rendering
    VkRenderingAttachmentInfo color_attachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    color_attachment.imageView   = context->swapchain_image_views[context->image_index];
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue.color = (VkClearColorValue){.float32 = {0.0f, 0.0f, 0.0f, 0.0f}};
    color_attachment.resolveMode = VK_RESOLVE_MODE_NONE;
    color_attachment.resolveImageView = VK_NULL_HANDLE;
    color_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkRenderingAttachmentInfo depth_stencil_attachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depth_stencil_attachment.imageView = context->depth_image_view;
    depth_stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_stencil_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depth_stencil_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_stencil_attachment.resolveMode = VK_RESOLVE_MODE_NONE;
    depth_stencil_attachment.resolveImageView = VK_NULL_HANDLE;

    VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering_info.renderArea = (VkRect2D){(VkOffset2D){0.0f, 0.0f}, context->swapchain_extent};
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &color_attachment;
    rendering_info.pDepthAttachment = &depth_stencil_attachment;
    rendering_info.pStencilAttachment = &depth_stencil_attachment;

    vkCmdBeginRendering(context->command_buffers[context->current_frame], &rendering_info);
    return true;
}

bool vulkan_backend_set_viewport(struct renderer_backend_t *backend, float w, float h, float minz, float maxz)
{
    vulkan_context_t *context = (vulkan_context_t *)backend->internal_context;
    VkViewport viewport = (VkViewport){0, 0, w, h, minz, maxz};
    vkCmdSetViewport(context->command_buffers[context->current_frame], 0, 1, &viewport); 
    return true;
}

bool vulkan_backend_set_scissor(struct renderer_backend_t *backend, 
                                float x, float y, float w, float h)
{
    vulkan_context_t *context = (vulkan_context_t *)backend->internal_context;
    VkRect2D scissor = (VkRect2D){{x, y}, {w, h}};
    vkCmdSetScissor(context->command_buffers[context->current_frame], 0, 1, &scissor);
    return true;
}

void vulkan_backend_copy_to_renderbuffer(struct renderer_backend_t *backend, renderbuffer_t *renderbuffer, void *src, uint32_t size)
{
    vulkan_buffer_t *vulkan_buffers = (vulkan_buffer_t *)renderbuffer->internal_data;
    vulkan_context_t *context = (vulkan_context_t*)backend->internal_context;

    vulkan_buffer_t *vulkan_buffer = &vulkan_buffers[context->current_frame];
    memcpy(vulkan_buffer->mapped, src, size);
}

static bool vulkan_backend_create_device_local_buffer(vulkan_context_t *context, 
                                                      vulkan_buffer_t *vulkan_buffers, 
                                                      uint32_t count, 
                                                      uint32_t size,
                                                      uint8_t *data,
                                                      VkBufferUsageFlags flags,
                                                      VkMemoryPropertyFlags mem_flags)
{
    for (uint32_t i = 0; i < count; i++) {
        vulkan_buffer_t *buffer = &vulkan_buffers[i];

        vulkan_buffer_t staging_buffer;
        create_vulkan_buffer(&staging_buffer,
                            context,
                            size,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        void *data;
        vkMapMemory(context->logical_device, staging_buffer.memory, 0, size, 0, &data);
        memcpy(data, data, size);
        vkUnmapMemory(context->logical_device, staging_buffer.memory);

        create_vulkan_buffer(buffer, context, size, flags, mem_flags);
        
        VkCommandBuffer copy_cmd = begin_command_buffer(context->logical_device, context->graphics_command_pool);
        
        VkBufferCopy copy_region = {};
        copy_region.size = size;
        vkCmdCopyBuffer(copy_cmd, staging_buffer.buffer, buffer->buffer, 1, &copy_region);

        end_command_buffer(&copy_cmd, context->logical_device, context->graphics_command_pool, context->graphics_queue);

        vkDestroyBuffer(context->logical_device, staging_buffer.buffer, NULL);
        vkFreeMemory(context->logical_device, staging_buffer.memory, NULL);
    }
    return true;
}

static bool vulkan_backend_create_host_visible_buffer(vulkan_context_t *context,
                                                      vulkan_buffer_t *vulkan_buffers,
                                                      uint32_t count, 
                                                      uint32_t size,
                                                      renderbuffer_type_e type,
                                                      VkDescriptorSetLayout layout,
                                                      VkBufferUsageFlags flags,
                                                      VkMemoryPropertyFlags mem_flags)
{
    for (uint32_t i = 0; i < count; i++) {
        vulkan_buffer_t *vulkan_buffer = &vulkan_buffers[i];
        //! these require descriptor sets
        create_vulkan_buffer(vulkan_buffer, context, size, flags, mem_flags);
        vkMapMemory(context->logical_device, vulkan_buffer->memory, 0, size, 0, &vulkan_buffer->mapped);

        //do descriptor sets
        VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        alloc_info.descriptorPool = context->descriptor_pool;
        alloc_info.pSetLayouts = &layout;
        alloc_info.descriptorSetCount = 1;
        VK_CHECK(vkAllocateDescriptorSets(context->logical_device, &alloc_info, &vulkan_buffer->descriptor_set));

        VkDescriptorBufferInfo buffer_info = {0};
        buffer_info.buffer = vulkan_buffer->buffer;
        buffer_info.offset = 0;
        buffer_info.range  = vulkan_buffer->buffer_size;

        VkWriteDescriptorSet descriptor_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptor_write.dstSet = vulkan_buffer->descriptor_set;
        descriptor_write.descriptorType = (type == RENDERBUFFER_TYPE_STORAGE_BUFFER) ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.dstBinding = 0;
        descriptor_write.pBufferInfo = &buffer_info;
        descriptor_write.descriptorCount = 1;
        
        vkUpdateDescriptorSets(context->logical_device, 1, &descriptor_write, 0, NULL);
    }
    return true;
}

bool vulkan_backend_create_renderbuffer(struct renderer_backend_t *backend, 
                                        renderbuffer_t *renderbuffer, 
                                        renderbuffer_type_e type,
                                        shader_t *shader,
                                        uint8_t *buffer_data,
                                        uint32_t size)
{
    vulkan_context_t *context = (vulkan_context_t *)backend->internal_context;

    VkDeviceSize buf_size = (VkDeviceSize)size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags mem_flags;
    uint32_t buffer_count = 0;

    switch(type)
    {
        default:
            LOGE("Unknown renderbuffer type");
            assert(false);
            break;
        case (RENDERBUFFER_TYPE_STORAGE_BUFFER):
            usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            mem_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            buffer_count = context->max_frames_in_flight;
            break;
        case(RENDERBUFFER_TYPE_INDEX_BUFFER):
            usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            mem_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            buffer_count = 1;
            break;
        case(RENDERBUFFER_TYPE_VERTEX_BUFFER):
            usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            mem_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            buffer_count = 1;
            break;
        case(RENDERBUFFER_TYPE_UNIFORM_BUFFER):
            usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            mem_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            buffer_count = context->max_frames_in_flight;
            break;
    }
    
    renderbuffer->internal_data = memory_alloc(buffer_count * sizeof(vulkan_buffer_t), MEM_TAG_PERMANENT);
    vulkan_buffer_t *buffers = (vulkan_buffer_t *)renderbuffer->internal_data;
    assert(renderbuffer->internal_data);

    bool success = false;
    //! if device local buffer, create staging buffer first, and copy data to device local buffer,
    if (mem_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        assert(buffer_data);
        success = vulkan_backend_create_device_local_buffer(context, 
                                                            buffers, 
                                                            buffer_count, 
                                                            buf_size, 
                                                            buffer_data, 
                                                            usage, 
                                                            mem_flags);
    } else {
        //! TODO: this is not very good. Shaders need a better way of keeping track of 
        // which descriptor layout is for what. Because I know that there's only one shader, I know which layout is required.
        // I need to think about what to do when we have multiple shaders
        vulkan_shader_t *vk_shader = (vulkan_shader_t *)shader->internal_data;

        VkDescriptorSetLayout layout;
        if (type == RENDERBUFFER_TYPE_UNIFORM_BUFFER) {
            layout = vk_shader->layouts[SCENE_MATRIX_LAYOUT];
        } else {
            layout = vk_shader->layouts[JOINT_MATRIX_LAYOUT];
        }
        success = vulkan_backend_create_host_visible_buffer(context,
                                                            buffers,
                                                            buffer_count,
                                                            buf_size,
                                                            type,
                                                            layout, 
                                                            usage,
                                                            mem_flags);
    }
    
    return success;
}

bool vulkan_backend_create_shader(renderer_backend_t *vulkan_backend, shader_t *shader)
{
    vulkan_context_t *context = (vulkan_context_t *)vulkan_backend->internal_context;
    shader->internal_data = memory_alloc(sizeof(vulkan_shader_t), MEM_TAG_PERMANENT);
    vulkan_shader_t *vulkan_shader = (vulkan_shader_t *)shader->internal_data;

    //setup descriptor layouts
    VkDescriptorSetLayoutBinding matrix_layout_binding = {0};
    matrix_layout_binding.binding = 0;
    matrix_layout_binding.descriptorCount = 1;
    matrix_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    matrix_layout_binding.pImmutableSamplers = NULL;
    matrix_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = 1;
    layout_info.pBindings = &matrix_layout_binding;
    layout_info.flags = 0;
    
    VK_CHECK(vkCreateDescriptorSetLayout(context->logical_device, &layout_info, NULL, &vulkan_shader->layouts[SCENE_MATRIX_LAYOUT]));

    VkDescriptorSetLayoutBinding joint_layout_binding = {0};
    joint_layout_binding.binding = 0;
    joint_layout_binding.descriptorCount = 1;
    joint_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    joint_layout_binding.pImmutableSamplers = NULL;
    joint_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    layout_info.pBindings = &joint_layout_binding;

    VK_CHECK(vkCreateDescriptorSetLayout(context->logical_device, &layout_info, NULL, &vulkan_shader->layouts[JOINT_MATRIX_LAYOUT]));

    VkDescriptorSetLayoutBinding texture_layout_binding = {0};
    texture_layout_binding.binding = 0;
    texture_layout_binding.descriptorCount = 1;
    texture_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_layout_binding.pImmutableSamplers = NULL;
    texture_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    layout_info.pBindings = &texture_layout_binding;

    VK_CHECK(vkCreateDescriptorSetLayout(context->logical_device, &layout_info, NULL, &vulkan_shader->layouts[TEXTURE_LAYOUT]));

    //! NOTE: if context has no uniform buffer for scene matrices, the first shader should create them
    if (!context->uniform_buffers) {
        context->uniform_buffers = memory_alloc(context->max_frames_in_flight * sizeof(vulkan_buffer_t), MEM_TAG_PERMANENT);
        if (!vulkan_backend_create_host_visible_buffer(context, 
                                                       context->uniform_buffers, 
                                                       context->max_frames_in_flight,
                                                       sizeof(uniform_data_t),
                                                       RENDERBUFFER_TYPE_UNIFORM_BUFFER,
                                                       vulkan_shader->layouts[SCENE_MATRIX_LAYOUT],
                                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            return false;
        }


    }

    //pipeline      

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_ci.setLayoutCount = 3;
    pipeline_layout_ci.pSetLayouts = vulkan_shader->layouts;
    
    //for model matrix of meshes
    VkPushConstantRange push_constant_range = {0};
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constant_range.size       = sizeof(mat4f_t);
    push_constant_range.offset     = 0;

    pipeline_layout_ci.pushConstantRangeCount = 1;
    pipeline_layout_ci.pPushConstantRanges = &push_constant_range;
    VK_CHECK(vkCreatePipelineLayout(context->logical_device, &pipeline_layout_ci, NULL, &vulkan_shader->pipeline_layout));

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_ci = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly_state_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_ci.primitiveRestartEnable = false;
    input_assembly_state_ci.flags = 0;
    
    VkPipelineRasterizationStateCreateInfo rasterization_state = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state.flags = 0;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.lineWidth = 1.0;
    
    VkPipelineColorBlendAttachmentState blend_attachment = {0};
    blend_attachment.colorWriteMask = 0xf;
    blend_attachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo color_blending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &blend_attachment;

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil.back.compareOp = VK_COMPARE_OP_ALWAYS;

    VkPipelineViewportStateCreateInfo viewport_state = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;
    viewport_state.flags = 0;

    VkPipelineMultisampleStateCreateInfo multisample_state = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;//msaa?
    multisample_state.flags = 0;
    
    VkDynamicState dynamic_state_enables[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state.dynamicStateCount = sizeof(dynamic_state_enables) / sizeof(dynamic_state_enables[0]);
    dynamic_state.pDynamicStates = dynamic_state_enables;
    dynamic_state.flags = 0;

    VkVertexInputBindingDescription vertex_input_bindings = {0};
    vertex_input_bindings.binding = 0;
    vertex_input_bindings.stride = sizeof(skinned_vertex_t);
    vertex_input_bindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_input_attributes[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skinned_vertex_t, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skinned_vertex_t, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(skinned_vertex_t, uv)},
        {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(skinned_vertex_t, joint_indices)},
        {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(skinned_vertex_t, joint_weights)}
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_bindings;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = sizeof(vertex_input_attributes) / sizeof(vertex_input_attributes[0]);
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attributes;

    VkPipelineShaderStageCreateInfo shader_stages[2];
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].flags = 0;
    shader_stages[0].module = create_shader_module(context->logical_device, "./assets/shaders/skinnedmodel.vert.spv");
    shader_stages[0].pNext = NULL;
    shader_stages[0].pName = "main";
    shader_stages[0].pSpecializationInfo = NULL;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].flags = 0;
    shader_stages[1].module = create_shader_module(context->logical_device, "./assets/shaders/skinnedmodel.frag.spv");
    shader_stages[1].pNext = NULL;
    shader_stages[1].pName = "main";
    shader_stages[1].pSpecializationInfo = NULL;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    //create pipeline cache
    VkPipelineCacheCreateInfo pipeline_cache_ci = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    VK_CHECK(vkCreatePipelineCache(context->logical_device, &pipeline_cache_ci, NULL, &vulkan_shader->pipeline_cache));
    VkGraphicsPipelineCreateInfo pipeline_ci = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_ci.layout = vulkan_shader->pipeline_layout;
    pipeline_ci.renderPass = VK_NULL_HANDLE;
    pipeline_ci.flags = 0;
    pipeline_ci.basePipelineIndex = -1;
    pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_ci.pVertexInputState = &vertex_input_state_create_info;
    pipeline_ci.pInputAssemblyState = &input_assembly_state_ci;
    pipeline_ci.pRasterizationState = &rasterization_state;
    pipeline_ci.pColorBlendState = &color_blending;
    pipeline_ci.pMultisampleState = &multisample_state;
    pipeline_ci.pViewportState = &viewport_state;
    pipeline_ci.pDepthStencilState = &depth_stencil;
    pipeline_ci.pDynamicState = &dynamic_state;
    pipeline_ci.stageCount = sizeof(shader_stages) / sizeof(shader_stages[0]);
    pipeline_ci.pStages = shader_stages;

    VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
    pipeline_rendering_create_info.colorAttachmentCount = 1;
    pipeline_rendering_create_info.pColorAttachmentFormats = &context->color_format;
    pipeline_rendering_create_info.depthAttachmentFormat = context->depth_format;
    pipeline_rendering_create_info.stencilAttachmentFormat = context->depth_format;
    
    pipeline_ci.pNext = &pipeline_rendering_create_info;

    VK_CHECK(vkCreateGraphicsPipelines(context->logical_device, vulkan_shader->pipeline_cache, 1, &pipeline_ci, NULL, &vulkan_shader->pipeline));

    return true;
}

bool vulkan_backend_use_shader(renderer_backend_t *backend, shader_t *shader)
{
    vulkan_context_t *context = (vulkan_context_t *)backend->internal_context;
    vulkan_shader_t *vulkan_shader = (vulkan_shader_t *)shader->internal_data;

    VkDescriptorSet descriptor_sets[] = {
       context->uniform_buffers[context->current_frame].descriptor_set,
    };

    vkCmdBindDescriptorSets(context->command_buffers[context->current_frame],
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            vulkan_shader->pipeline_layout,
                            0, sizeof(descriptor_sets)/sizeof(descriptor_sets[0]), 
                            descriptor_sets, 0, NULL);
    vkCmdBindPipeline(context->command_buffers[context->current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_shader->pipeline);
    return true;
}

bool vulkan_backend_create_texture(renderer_backend_t *backend, texture_t *texture, const char *file_path)
{
    vulkan_context_t *context = (vulkan_context_t*)backend->internal_context;
    const char *file_extension = find_last_of(file_path, ".");

    //! TODO: fix this immediately
    vulkan_texture_t vulkan_texture = {0};
    
    if (strncmp(file_extension, "png", 3) == 0 ||
        strncmp(file_extension, "jpg", 3) == 0) {
        if (!vulkan_texture_from_file(&vulkan_texture, context, file_path)) {
            return false;
        }
    } else if (strncmp(file_extension, "ktx", 3) == 0)
    {
        if (!vulkan_ktx_texture_from_file(&vulkan_texture, context, file_path)) {
            return false;
        }
    } else {
        assert(false && "unknown texture file extension");
    }

    //descriptor stuff
    return true;
}

bool vulkan_backend_end_rendering(renderer_backend_t *backend)
{
    vulkan_context_t *context = (vulkan_context_t*)backend->internal_context;
    vkCmdEndRendering(context->command_buffers[context->current_frame]);
    
    //prepare image for present
    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.image = context->swapchain_images[context->image_index];
    barrier.subresourceRange = (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    
    vkCmdPipelineBarrier(context->command_buffers[context->current_frame], 
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 
                         0, NULL,
                         0, NULL,
                         1, &barrier);

    vkEndCommandBuffer(context->command_buffers[context->current_frame]);
    return true;
}

bool vulkan_backend_bind_vertex_buffers(struct renderer_backend_t *backend, renderbuffer_t *buffer)
{
    vulkan_context_t *context = (vulkan_context_t *)backend->internal_context;
    vulkan_buffer_t *vertex_buffer = (vulkan_buffer_t*)buffer->internal_data;

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(context->command_buffers[context->current_frame], 0, 1, &vertex_buffer->buffer, offsets);
    return true;
}

bool vulkan_backend_bind_index_buffers(struct renderer_backend_t *backend, renderbuffer_t *buffer)
{
    vulkan_context_t *context = (vulkan_context_t *)backend->internal_context;
    vulkan_buffer_t *index_buffer = (vulkan_buffer_t*)buffer->internal_data;
    vkCmdBindIndexBuffer(context->command_buffers[context->current_frame], index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
    return true;
}
