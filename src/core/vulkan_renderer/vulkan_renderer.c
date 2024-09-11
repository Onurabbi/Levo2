#include "vulkan_renderer.h"
#include "vulkan_texture.h"
#include "vulkan_common.h"
#include "vulkan_buffer.h"

#include "../logger/logger.h"
#include "../math/math_utils.h"
#include "../game/game_types.h"
#include "../game/collision.h"
#include "../core/containers/containers.h"
#include "../game/animation.h"
#include "../memory/memory.h"
#include "../utils/utils.h"

#include <math.h>

#include <SDL2/SDL_vulkan.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define MAX_FRAMES_IN_FLIGHT 2

static void create_vulkan_instance(vulkan_renderer_t *renderer, SDL_Window *window)
{
    assert(window);

    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0,1,0,0);
    appInfo.engineVersion = VK_MAKE_API_VERSION(0,1,0,0);
    appInfo.pEngineName = "LeventEngine";
    appInfo.apiVersion = VK_MAKE_API_VERSION(0,1,0,0);  

    uint32_t sdl_extension_count = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, NULL);
    const char **sdl_extensions = memory_alloc(sizeof(const char*) * sdl_extension_count, MEM_TAG_TEMP);
    assert(sdl_extensions);

    SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, sdl_extensions);

    uint32_t validation_layer_count = 0;
    const char **enabled_layer_names = NULL;

#ifdef NDEBUG
#else
    uint32_t layer_count;
    const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";

    VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, NULL));

    VkLayerProperties *layer_properties = memory_alloc(layer_count * sizeof(VkLayerProperties), MEM_TAG_TEMP);
    assert(layer_properties);

    VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, layer_properties));

    bool validation_layer_found = false;

    for (uint32_t i = 0; i < layer_count; i++)
    {
        if (strcmp(validation_layer_name, layer_properties[i].layerName) == 0)
        {
            validation_layer_found = true;
            break;
        }
    }

    if (validation_layer_found)
    {
        validation_layer_count = 1;
        enabled_layer_names = &validation_layer_name;
    }
#endif

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.flags = 0;
    instanceInfo.enabledExtensionCount   = sdl_extension_count;
    instanceInfo.ppEnabledExtensionNames = sdl_extensions;
    instanceInfo.enabledLayerCount = 0;
    instanceInfo.ppEnabledLayerNames = enabled_layer_names;
    instanceInfo.enabledLayerCount = validation_layer_count;

    VK_CHECK(vkCreateInstance(&instanceInfo, NULL, &renderer->instance));
}

static void create_physical_device(vulkan_renderer_t *renderer)
{
    uint32_t physical_device_count;
    VK_CHECK(vkEnumeratePhysicalDevices(renderer->instance, &physical_device_count, NULL));

    VkPhysicalDevice *physical_devices = memory_alloc(physical_device_count * sizeof(VkPhysicalDevice), MEM_TAG_TEMP);
    assert(physical_devices);

    VK_CHECK(vkEnumeratePhysicalDevices(renderer->instance, &physical_device_count, physical_devices));

    renderer->physical_device = physical_devices[0];

    vkGetPhysicalDeviceProperties(renderer->physical_device, &renderer->device_properties);
    vkGetPhysicalDeviceFeatures(renderer->physical_device, &renderer->device_features);
    vkGetPhysicalDeviceMemoryProperties(renderer->physical_device, &renderer->device_memory_properties);
    
    uint32_t queue_family_property_count;
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queue_family_property_count, NULL);

    VkQueueFamilyProperties *queue_family_properties = memory_alloc(queue_family_property_count * sizeof(VkQueueFamilyProperties), MEM_TAG_TEMP);
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queue_family_property_count, queue_family_properties);

    uint32_t graphics_family   = UINT32_MAX;
    uint32_t compute_family    = UINT32_MAX;
    uint32_t transfer_family   = UINT32_MAX;
    uint32_t compute_fallback  = UINT32_MAX;
    uint32_t transfer_fallback = UINT32_MAX;

    for (uint32_t i = 0; i < queue_family_property_count; i++)
    {
        //get the first one that supports graphics
        if (graphics_family == UINT32_MAX)
        {
            if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphics_family = i;
            }
        }

        //otherwise try to find a dedicated compute queue
        if (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            if (compute_fallback == UINT32_MAX)
            {
                compute_fallback = i;
            }
            //if dedicated
            if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
            {
                compute_family = i;
            }
        }

        if (queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            if (transfer_fallback == UINT32_MAX)
            {
                transfer_fallback = i;
            }
           if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
            {
                transfer_family = i;
            }
        }
    }

    if (compute_family == UINT32_MAX) compute_family = compute_fallback;
    if (transfer_family == UINT32_MAX) transfer_family = transfer_fallback;

    renderer->graphics_family = graphics_family;
    renderer->compute_family  = compute_family;
    renderer->transfer_family = transfer_family;
}

static void create_logical_device(vulkan_renderer_t *renderer)
{
    VkDeviceQueueCreateInfo queue_create_infos[3];
    uint32_t queue_create_info_count = 0;

    float priority = 1.0f;

    {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = renderer->graphics_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        queue_create_infos[queue_create_info_count++] = queue_info;
    }

    if (renderer->compute_family != renderer->graphics_family)
    {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = renderer->compute_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        queue_create_infos[queue_create_info_count++] = queue_info;
    }

    if (renderer->transfer_family != renderer->graphics_family &&
        renderer->transfer_family != renderer->compute_family)
    {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = renderer->transfer_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        queue_create_infos[queue_create_info_count++] = queue_info;
    }

    //the only extension we are using is the swapchain
    const char* swapchain_extension_name = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.enabledExtensionCount = 1;
    device_create_info.ppEnabledExtensionNames = &swapchain_extension_name;
    device_create_info.pEnabledFeatures  = &device_features;
    device_create_info.queueCreateInfoCount = queue_create_info_count;
    device_create_info.pQueueCreateInfos    = queue_create_infos;
    VK_CHECK(vkCreateDevice(renderer->physical_device, &device_create_info, NULL, &renderer->logical_device));

    //queues are created along with the device
    vkGetDeviceQueue(renderer->logical_device, renderer->graphics_family, 0, &renderer->graphics_queue);
}

static void create_swapchain(vulkan_renderer_t *renderer)
{
    //swapchain
    //check present
    VkBool32 present_support = false;

    vkGetPhysicalDeviceSurfaceSupportKHR(renderer->physical_device, renderer->graphics_family, renderer->surface, &present_support);
    if (present_support)
    {
        
    }
    else
    {
        LOGE("Graphics family doesn't support present :(");
        assert(false);
        return;
    }

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physical_device, renderer->surface, &format_count, NULL);

    VkSurfaceFormatKHR *surface_formats = memory_alloc(sizeof(VkSurfaceFormatKHR) * format_count, MEM_TAG_TEMP);
    assert(surface_formats);

    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physical_device, renderer->surface, &format_count, surface_formats);

    renderer->color_format = surface_formats[0].format;
    renderer->color_space  = surface_formats[0].colorSpace;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->physical_device, renderer->surface, &surfaceCapabilities);

    renderer->swapchain_extent = surfaceCapabilities.currentExtent;

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->physical_device, renderer->surface, &present_mode_count, NULL);

    VkPresentModeKHR *present_modes = memory_alloc(sizeof(VkPresentModeKHR)*present_mode_count, MEM_TAG_TEMP);
    assert(present_modes);
    
    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->physical_device, renderer->surface, &present_mode_count, present_modes);
    
    //use VK_PRESENT_MODE_FIFO_KHR as fallback
    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    for (uint32_t i = 0; i < present_mode_count; i++)
    {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
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
    for (uint32_t i = 0; i < sizeof(composite_alpha_bits) / sizeof(composite_alpha_bits[0]); i++)
    {
        if (surfaceCapabilities.supportedCompositeAlpha & composite_alpha_bits[i])
        {
            selected_composite_alpha = composite_alpha_bits[i];
            break;
        }
    }

    VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    VkSurfaceTransformFlagBitsKHR pre_transform;
    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        pre_transform = surfaceCapabilities.currentTransform;
    }

    VkSwapchainCreateInfoKHR swapchainCI = {};
    swapchainCI.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.surface = renderer->surface;
    swapchainCI.imageColorSpace = renderer->color_space;
    swapchainCI.imageFormat = renderer->color_format;
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
    swapchainCI.imageExtent = renderer->swapchain_extent;

    VK_CHECK(vkCreateSwapchainKHR(renderer->logical_device, &swapchainCI, NULL, &renderer->swapchain));

    //get swapchain images
    VK_CHECK(vkGetSwapchainImagesKHR(renderer->logical_device, renderer->swapchain, &renderer->swapchain_image_count, NULL));
    renderer->swapchain_images = malloc(renderer->swapchain_image_count * sizeof(VkImage));
    VK_CHECK(vkGetSwapchainImagesKHR(renderer->logical_device, renderer->swapchain, &renderer->swapchain_image_count, renderer->swapchain_images));
    renderer->swapchain_image_views = malloc(renderer->swapchain_image_count * sizeof(VkImageView));

    for (uint32_t i = 0; i < renderer->swapchain_image_count; i++)
    {
        VkImageViewCreateInfo viewCI = {};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.components = (VkComponentMapping){
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        };

        viewCI.format   = renderer->color_format;
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.image    = renderer->swapchain_images[i];
        viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.baseMipLevel = 0,
        viewCI.subresourceRange.levelCount = 1;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount = 1;
        viewCI.flags = 0;
        VK_CHECK(vkCreateImageView(renderer->logical_device, &viewCI, NULL, &renderer->swapchain_image_views[i]));
    }
}

static void create_command_pool(vulkan_renderer_t *renderer)
{
    //create command pool
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = renderer->graphics_family;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(renderer->logical_device, &cmdPoolInfo, NULL, &renderer->command_pool));
}

static void allocate_command_buffers(vulkan_renderer_t *renderer)
{
    VkCommandBufferAllocateInfo cmd_buf_info = {0};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_info.commandPool = renderer->command_pool;
    cmd_buf_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    cmd_buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK_CHECK(vkAllocateCommandBuffers(renderer->logical_device, &cmd_buf_info, renderer->command_buffers));
}

static void create_sync_primitives(vulkan_renderer_t *renderer)
{
    renderer->current_frame = 0;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkSemaphoreCreateInfo sem_info = {0};
        sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        
        VkFenceCreateInfo fence_info = {0};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_CHECK(vkCreateSemaphore(renderer->logical_device, &sem_info, NULL, &renderer->render_complete_semaphores[i]));
        VK_CHECK(vkCreateSemaphore(renderer->logical_device, &sem_info, NULL, &renderer->present_complete_semaphores[i]));
        VK_CHECK(vkCreateFence(renderer->logical_device, &fence_info, NULL, &renderer->wait_fences[i]));
    }
}

static void setup_depth_stencil(vulkan_renderer_t *renderer)
{
    //create depth image
    VkFormat formatList[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    VkBool32 formatFound = VK_FALSE;
    for (uint32_t i = 0; i < sizeof(formatList)/sizeof(formatList[0]); i++)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(renderer->physical_device, formatList[i], &formatProps);
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            renderer->depth_format = formatList[i];
            formatFound = VK_TRUE;
            break;
        }
    }

    assert(formatFound == VK_TRUE);

    VkImageCreateInfo imageCI = {};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.mipLevels = 1;
    imageCI.extent = (VkExtent3D){renderer->swapchain_extent.width, renderer->swapchain_extent.height, 1};
    imageCI.format = renderer->depth_format;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VK_CHECK(vkCreateImage(renderer->logical_device, &imageCI, NULL, &renderer->depth_image));

    VkMemoryRequirements memReqs = {};
    vkGetImageMemoryRequirements(renderer->logical_device, renderer->depth_image, &memReqs);

    uint32_t memTypeIndex;
    assert(get_memory_type(&renderer->device_memory_properties, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memTypeIndex));

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memTypeIndex;
    VK_CHECK(vkAllocateMemory(renderer->logical_device, &allocInfo, NULL, &renderer->depth_memory));
    VK_CHECK(vkBindImageMemory(renderer->logical_device, renderer->depth_image, renderer->depth_memory, 0));

    VkImageViewCreateInfo imageViewCI = {};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.image = renderer->depth_image;
    imageViewCI.format = renderer->depth_format;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    VK_CHECK(vkCreateImageView(renderer->logical_device, &imageViewCI, NULL, &renderer->depth_image_view));
}

static void create_render_pass(vulkan_renderer_t* renderer)
{
    //render pass
    VkAttachmentDescription attachments[2];
    attachments[0].flags = 0;
    attachments[0].format = renderer->color_format;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE; 
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[1].flags = 0;
    attachments[1].format = renderer->depth_format;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attachmentReferences[2];
    attachmentReferences[0].attachment = 0;
    attachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentReferences[1].attachment = 1;
    attachmentReferences[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.flags = 0;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &attachmentReferences[0];
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = NULL;
    subpassDescription.pDepthStencilAttachment = &attachmentReferences[1];
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pResolveAttachments = NULL;
    subpassDescription.pPreserveAttachments = NULL;

    VkSubpassDependency subpassDependencies[2];
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    subpassDependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    subpassDependencies[0].dependencyFlags = 0;
    
    subpassDependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].dstSubpass = 0;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].srcAccessMask = 0;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    subpassDependencies[1].dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = sizeof(attachments)/sizeof(attachments[0]);
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = sizeof(subpassDependencies)/sizeof(subpassDependencies[0]);
    renderPassInfo.pDependencies = subpassDependencies;

    VK_CHECK(vkCreateRenderPass(renderer->logical_device, &renderPassInfo, NULL, &renderer->render_pass));
}

static void create_framebuffers(vulkan_renderer_t *renderer)
{
    renderer->framebuffers = malloc(renderer->swapchain_image_count * sizeof(VkFramebuffer));
    assert(renderer->framebuffers);

    for (uint32_t i = 0; i < renderer->swapchain_image_count; i++)
    {
        VkImageView attachments[] = {
            renderer->swapchain_image_views[i],
            renderer->depth_image_view
        };

        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.flags = 0;
        fbInfo.attachmentCount = 2;
        fbInfo.pAttachments = attachments;
        fbInfo.renderPass = renderer->render_pass;
        fbInfo.layers = 1;
        fbInfo.width = renderer->swapchain_extent.width;
        fbInfo.height = renderer->swapchain_extent.height;
        
        VK_CHECK(vkCreateFramebuffer(renderer->logical_device, &fbInfo, NULL, &renderer->framebuffers[i]));
    }   
}

static VkShaderModule create_shader_module(VkDevice device, const char* filePath)
{
    long fsize;
    char *data = read_whole_file(filePath, &fsize, MEM_TAG_TEMP);
    
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

static void create_graphics_pipeline(vulkan_renderer_t *renderer)
{
   //load shaders
    VkShaderModule vertModule = create_shader_module(renderer->logical_device, "./assets/shaders/vert.spv");
    VkShaderModule fragModule = create_shader_module(renderer->logical_device, "./assets/shaders/frag.spv");

    //graphics pipeline
    VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2];
    shaderStageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfos[0].flags = 0;
    shaderStageCreateInfos[0].module = vertModule;
    shaderStageCreateInfos[0].pNext = NULL;
    shaderStageCreateInfos[0].pName = "main";
    shaderStageCreateInfos[0].pSpecializationInfo = NULL;
    shaderStageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;

    shaderStageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfos[1].flags = 0;
    shaderStageCreateInfos[1].module = fragModule;
    shaderStageCreateInfos[1].pNext = NULL;
    shaderStageCreateInfos[1].pName = "main";
    shaderStageCreateInfos[1].pSpecializationInfo = NULL;
    shaderStageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkVertexInputBindingDescription vertexBinding = {};
    vertexBinding.binding = 0;
    vertexBinding.stride = sizeof(vertex_t);
    vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[2];
    memset(attributeDescriptions, 0, sizeof(attributeDescriptions));

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(vertex_t, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(vertex_t, tex_coord);

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.flags = 0;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBinding;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.flags = 0;
    inputAssemblyStateCreateInfo.pNext = NULL;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)renderer->swapchain_extent.width;
    viewport.height = (float)renderer->swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.extent = renderer->swapchain_extent;
    scissor.offset = (VkOffset2D){0};

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.flags = 0;
    viewportState.pNext = NULL;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.pNext = NULL;
    rasterizer.flags = 0;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample = {};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.pNext = NULL;
    multisample.flags = 0;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample.sampleShadingEnable = VK_FALSE;
    multisample.alphaToCoverageEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.pNext = NULL;
    depthStencil.flags = 0;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable    = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = 0;
    colorBlending.flags = 0;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = NULL;
    dynamicState.flags  = 0;
    dynamicState.dynamicStateCount = 0;

    VkPushConstantRange range = {};
    range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    range.offset = 0;
    range.size = sizeof(int);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &renderer->descriptor_set_layout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &range;

    VK_CHECK(vkCreatePipelineLayout(renderer->logical_device, &pipelineLayoutInfo, NULL, &renderer->pipeline_layout));

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStageCreateInfos;
    pipelineInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = renderer->pipeline_layout;
    pipelineInfo.renderPass = renderer->render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.pDepthStencilState = &depthStencil;
    
    VK_CHECK(vkCreateGraphicsPipelines(renderer->logical_device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &renderer->graphics_pipeline));
    vkDestroyShaderModule(renderer->logical_device, fragModule, NULL);
    vkDestroyShaderModule(renderer->logical_device, vertModule, NULL);    
}

static void create_descriptor_set_layout(vulkan_renderer_t *renderer)
{
    VkDescriptorSetLayoutBinding layoutBindings[1];
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = sizeof(layoutBindings)/sizeof(layoutBindings[0]);
    layoutInfo.pBindings = layoutBindings;
    VK_CHECK(vkCreateDescriptorSetLayout(renderer->logical_device, &layoutInfo, NULL, &renderer->descriptor_set_layout));
}

static void create_vertex_buffers(vulkan_renderer_t *renderer, 
                                  uint32_t vertex_count)
{
    VkDeviceSize bufferSize = sizeof(vertex_t) * vertex_count;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        create_vulkan_buffer(&renderer->vertex_buffers[i], 
                            renderer, 
                            bufferSize, 
                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkMapMemory(renderer->logical_device, renderer->vertex_buffers[i].memory, 0, bufferSize, 0, &renderer->vertex_buffers[i].mapped);
    }
}

static void create_index_buffer(vulkan_renderer_t *renderer,
                                uint32_t        indexCount)
{
    assert(indexCount % 6 == 0);
    //initialize index buffer
    uint16_t *indices = memory_alloc(sizeof(uint16_t) * indexCount, MEM_TAG_TEMP);
    uint32_t index = 0;
    for (uint16_t i = 0; i < indexCount; i += 6)
    {

        indices[i]     = index;
        indices[i + 1] = index + 1;
        indices[i + 2] = index + 2;
        indices[i + 3] = index + 2;
        indices[i + 4] = index + 3;
        indices[i + 5] = index;

        index += 4;
    }

    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
    vulkan_buffer_t staging_buffer;
    create_vulkan_buffer(&staging_buffer,
                         renderer, 
                         bufferSize, 
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    void *data;
    vkMapMemory(renderer->logical_device, staging_buffer.memory, 0, bufferSize, 0, &data);
    memcpy(data, indices, bufferSize);
    vkUnmapMemory(renderer->logical_device, staging_buffer.memory);


    create_vulkan_buffer(&renderer->index_buffer,
                 renderer, 
                 bufferSize, 
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkCommandBuffer copyCmd = begin_command_buffer(renderer->logical_device, renderer->command_pool);

    VkBufferCopy copyRegion = {};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(copyCmd, staging_buffer.buffer, renderer->index_buffer.buffer, 1, &copyRegion);

    end_command_buffer(&copyCmd, renderer->logical_device, renderer->command_pool, renderer->graphics_queue);

    vkDestroyBuffer(renderer->logical_device, staging_buffer.buffer, NULL);
    vkFreeMemory(renderer->logical_device, staging_buffer.memory, NULL);
}

static void create_descriptor_pool(vulkan_renderer_t *renderer)
{
    VkDescriptorPoolSize pool_sizes[1];
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[0].descriptorCount = MAX_TEXTURE_COUNT * MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = pool_sizes;
    poolInfo.maxSets = MAX_TEXTURE_COUNT * MAX_FRAMES_IN_FLIGHT;
    VK_CHECK(vkCreateDescriptorPool(renderer->logical_device, &poolInfo, NULL, &renderer->descriptor_pool));
}

void vulkan_renderer_init(vulkan_renderer_t *renderer, SDL_Window *window, float tile_width, float tile_height)
{
    renderer->tile_width = tile_width;
    renderer->tile_height = tile_height;

    create_vulkan_instance(renderer, window);
    if (!SDL_Vulkan_CreateSurface(window, renderer->instance, &renderer->surface))
    {
        LOGE("Unable to create window surface");
        return;
    }

    create_physical_device(renderer);
    create_logical_device(renderer);
    create_swapchain(renderer);
    create_command_pool(renderer);
    allocate_command_buffers(renderer);
    create_sync_primitives(renderer);
    //! NOTE: remove this
    setup_depth_stencil(renderer);
    create_render_pass(renderer);
    create_framebuffers(renderer);
    create_descriptor_set_layout(renderer);
    create_graphics_pipeline(renderer);

    create_vertex_buffers(renderer, MAX_SPRITES_PER_BATCH * 4);
    create_index_buffer(renderer, MAX_SPRITES_PER_BATCH * 6);
    create_descriptor_pool(renderer);

}

static int compare_drawables(const void *a, const void *b)
{
    drawable_t *d1 = (drawable_t*)a;
    drawable_t *d2 = (drawable_t*)b;

    if (d1->z_index < d2->z_index) 
    {
        return -1;
    }
    else if (d1->z_index > d2->z_index)
    {
        return 1;
    }
    else
    {
        if (d1->tex < d2->tex)
        {
            return -1;
        }
        else if (d1->tex == d2->tex)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
}

static inline void draw_sprite(drawable_t *drawables, 
                               uint32_t *drawable_count,
                               vulkan_texture_t *texture,   
                               rect_t src_rect, 
                               rect_t dst_rect, 
                               rect_t camera,
                               float tile_width,
                               float tile_height,
                               uint32_t z_index,
                               bool is_fixed)
{
    uint32_t count = *drawable_count;

    drawable_t *drawable = &drawables[count++];
    drawable->tex = texture;
    drawable->src_rect = src_rect;
    drawable->dst_rect = dst_rect;
    drawable->dst_rect.min.x -= (is_fixed ? 0 : camera.min.x);
    drawable->dst_rect.min.y -= (is_fixed ? 0 : camera.min.y);
    drawable->dst_rect.min.x -= tile_width / 2;
    drawable->dst_rect.min.y -= tile_height / 2;
    drawable->z_index = z_index;

    *drawable_count = count;
}

static void draw_text(drawable_t *drawables,
                      uint32_t  *drawable_count, 
                      text_label_t *label,
                      vec2f_t p,
                      rect_t camera,
                      float tile_width, 
                      float tile_height,
                      uint32_t z_index,
                      bool is_fixed)
{
    uint32_t count = *drawable_count;

    const char *text = (label->text ? label->text : "NULL");
    uint32_t len = strlen(text);
    
    float x = p.x + label->offset.x - (is_fixed ? 0 : camera.min.x) - tile_width / 2;
    float y = p.y + label->offset.y - (is_fixed ? 0 : camera.min.y) - tile_height / 2;
    
    font_t *font = label->font;
    vulkan_texture_t *font_texture = font->texture;
    rect_t *glyphs = font->glyphs;

    for (uint32_t i = 0; i < len; i++)
    {
        rect_t src = glyphs[text[i] - ' '];
        rect_t dst = (rect_t){{x,y},{src.size.x, src.size.y}};

        drawable_t *drawable = &drawables[count++];
        drawable->tex = font_texture;
        drawable->src_rect = src;
        drawable->dst_rect = dst;
        drawable->z_index = z_index;

        x += src.size.x;
    }
    
    *drawable_count = count;
}

static uint32_t vulkan_renderer_render_entities(vulkan_renderer_t *renderer, 
                                                bulk_data_entity_t *entities,
                                                rect_t camera,
                                                vertex_t *vertices, 
                                                uint32_t vertex_count,
                                                uint32_t vertex_capacity)
{
    drawable_t *drawables = memory_alloc(sizeof(drawable_t) * MAX_SPRITES_PER_BATCH, MEM_TAG_RENDER);
    uint32_t drawable_count = 0;

    for (uint32_t i = 0; i < bulk_data_size(entities); i++)
    {
        entity_t *e = bulk_data_getp_null(entities, i);
        if (e)
        {
            assert(drawable_count < MAX_SPRITES_PER_BATCH);

            //rect vs rect against camera
            if (e->type == ENTITY_TYPE_WIDGET ||
                rect_vs_rect(e->rect, camera))
            {
                switch(e->type)
                {
                    case ENTITY_TYPE_PLAYER:
                    {
                        player_t *p = (player_t*)e->data;
                        sprite_animation_t *anim = p->current_animation;
                        uint32_t current_frame = animation_get_current_frame(anim, p->anim_timer);

                        draw_sprite(drawables, &drawable_count, anim->texture, anim->sprites[current_frame], e->rect, camera, renderer->tile_width, renderer->tile_height, e->z_index, false);
                        draw_text(drawables, &drawable_count, p->name, e->rect.min, camera, renderer->tile_width, renderer->tile_height, e->z_index, false);
                        break;
                    }
                    case ENTITY_TYPE_TILE:
                    {
                        tile_t *tile = (tile_t*)e->data;
                        draw_sprite(drawables, &drawable_count, tile->sprite.texture, tile->sprite.src_rect, e->rect, camera, renderer->tile_width, renderer->tile_height, e->z_index, false);
                        break;
                    }
                    case ENTITY_TYPE_WEAPON:
                    {
                        weapon_t *weapon = (weapon_t*)e->data;
                        draw_sprite(drawables, &drawable_count, weapon->sprite.texture, weapon->sprite.src_rect, e->rect, camera, renderer->tile_width, renderer->tile_height, e->z_index, false);
                        break;
                    }
                    case ENTITY_TYPE_WIDGET:
                    {
                        widget_t *widget = (widget_t*)e->data;
                        draw_text(drawables, &drawable_count, widget->text_label, e->rect.min, camera, renderer->tile_width, renderer->tile_height, e->z_index, true);
                        break;
                    }
                    default: 
                        assert(false);
                        break;
                }
            }
        }
    }
    
    //sort renders
    qsort(drawables, drawable_count, sizeof(drawables[0]), compare_drawables);

    float half_width = (float)renderer->swapchain_extent.width * 0.5f;
    float half_height = (float)renderer->swapchain_extent.height * 0.5f;

    for (uint32_t i = 0; i < drawable_count; i++)
    {
        assert(vertex_count <= vertex_capacity - 4);

        drawable_t *drawable = &drawables[i];
        rect_t dst = drawable->dst_rect;
        rect_t src = drawable->src_rect;
        vulkan_texture_t *texture = drawable->tex;

        if (renderer->textures[renderer->current_texture] == NULL)
        {
            renderer->textures[renderer->current_texture] = texture;
            renderer->offsets[renderer->current_texture] = 0;
        }

        if (renderer->textures[renderer->current_texture] != texture)
        {
            renderer->offsets[renderer->current_texture + 1]  = vertex_count;
            renderer->textures[renderer->current_texture + 1] = texture;
            renderer->current_texture++;
        }

        float left   = (dst.min.x - half_width) / half_width;
        float right  = (dst.min.x + dst.size.x - half_width) / half_width;
        float top    = (dst.min.y - half_height) / half_height;
        float bottom = (dst.min.y + dst.size.y - half_height) / half_height;

        float u  = src.min.x / texture->w;
        float v  = src.min.y / texture->h;
        float du = src.size.x / texture->w;
        float dv = src.size.y / texture->h;

        vertex_t *top_left = &vertices[vertex_count++];
        top_left->pos       = (vec2f_t){left, top};
        top_left->tex_coord = (vec2f_t){u,v};

        vertex_t *top_right = &vertices[vertex_count++];
        top_right->pos       = (vec2f_t){right, top};
        top_right->tex_coord = (vec2f_t){u + du, v};

        vertex_t *bottom_right = &vertices[vertex_count++];
        bottom_right->pos       = (vec2f_t){right, bottom};
        bottom_right->tex_coord = (vec2f_t){u + du, v + dv};

        vertex_t *bottom_left = &vertices[vertex_count++];
        bottom_left->pos         = (vec2f_t){left, bottom};
        bottom_left->tex_coord   = (vec2f_t){u, v + dv};
    }
    return vertex_count;
}

static void vulkan_renderer_present(vulkan_renderer_t *renderer, vertex_t *vertices, uint32_t vertex_count)
{
    if (renderer->current_texture == 0)
    {
        return;
    }
    //update vertex buffer
    memcpy(renderer->vertex_buffers[renderer->current_frame].mapped, vertices, sizeof(vertex_t) * vertex_count);

    vkWaitForFences(renderer->logical_device, 1, &renderer->wait_fences[renderer->current_frame], VK_TRUE, UINT64_MAX);
    vkAcquireNextImageKHR(renderer->logical_device, 
                          renderer->swapchain, 
                          UINT64_MAX, 
                          renderer->present_complete_semaphores[renderer->current_frame],
                          VK_NULL_HANDLE,
                          &renderer->image_index);

    vkResetFences(renderer->logical_device, 1, &renderer->wait_fences[renderer->current_frame]);
    vkResetCommandBuffer(renderer->command_buffers[renderer->current_frame], 0);
    
    //record command buffer
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK(vkBeginCommandBuffer(renderer->command_buffers[renderer->current_frame], &beginInfo));

    VkClearValue clearValues[2];
    clearValues[0].color = (VkClearColorValue){{1.0f, 0.0f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderer->render_pass;
    renderPassInfo.framebuffer = renderer->framebuffers[renderer->image_index];
    renderPassInfo.renderArea.offset = (VkOffset2D){0,0};
    renderPassInfo.renderArea.extent = renderer->swapchain_extent;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;
    vkCmdBeginRenderPass(renderer->command_buffers[renderer->current_frame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(renderer->command_buffers[renderer->current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->graphics_pipeline);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(renderer->command_buffers[renderer->current_frame], 0, 1, &renderer->vertex_buffers[renderer->current_frame].buffer, offsets);
    vkCmdBindIndexBuffer(renderer->command_buffers[renderer->current_frame], renderer->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);

    uint32_t first_index = 0;
    for (uint32_t i = 0; i <= renderer->current_texture; i++)
    {
        uint64_t next_offset = (i < (renderer->current_texture) ? renderer->offsets[i + 1] : vertex_count);

        uint64_t vert_count = next_offset - renderer->offsets[i];
        uint64_t index_count  = vert_count * 6 / 4;

        vkCmdBindDescriptorSets(renderer->command_buffers[renderer->current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline_layout, 0, 1, &renderer->textures[i]->descriptor_sets[renderer->current_frame],0,NULL);
        vkCmdDrawIndexed(renderer->command_buffers[renderer->current_frame], index_count, 1, first_index, 0, 0);
        first_index += index_count;
    }

    vkCmdEndRenderPass(renderer->command_buffers[renderer->current_frame]);
    VK_CHECK(vkEndCommandBuffer(renderer->command_buffers[renderer->current_frame]));

    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &renderer->present_complete_semaphores[renderer->current_frame];
    submitInfo.pWaitDstStageMask = &waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderer->command_buffers[renderer->current_frame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderer->render_complete_semaphores[renderer->current_frame];
    VK_CHECK(vkQueueSubmit(renderer->graphics_queue, 1, &submitInfo, renderer->wait_fences[renderer->current_frame]));
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderer->render_complete_semaphores[renderer->current_frame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &renderer->swapchain;
    presentInfo.pImageIndices = &renderer->image_index;
    VK_CHECK(vkQueuePresentKHR(renderer->graphics_queue, &presentInfo));
}


void vulkan_renderer_render(vulkan_renderer_t *renderer, game_t *game)
{
    //begin render
    vertex_t *vertices = memory_alloc(sizeof(vertex_t) * MAX_SPRITES_PER_BATCH * 4, MEM_TAG_RENDER);

    //reset entities
    renderer->current_frame   = (renderer->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    renderer->current_texture = 0;
    memset(renderer->textures, 0, sizeof(renderer->textures));
    memset(renderer->offsets, 0, sizeof(renderer->offsets));
    
    //reset text

    uint32_t vertex_count = vulkan_renderer_render_entities(renderer, &game->entities, game->camera, vertices, 0, MAX_SPRITES_PER_BATCH * 4);
    vulkan_renderer_present(renderer, vertices, vertex_count);
}


