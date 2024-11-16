#include "renderer.h"
#include <memory.h>
#include <logger.h>
#include <asset_store.h>
#include <vulkan_backend.h>

enum
{
    MAX_RENDERBUFFER_COUNT = 32,
    MAX_SHADER_COUNT = 32,
};

static uint32_t get_number_of_frames(renderer_backend_t *backend)
{
    return backend->get_num_frames_in_flight(backend);
}

bool renderer_initialize(renderer_t *renderer, window_t *window, renderer_config_t config, struct bulk_data_renderbuffer_t *renderbuffers, struct bulk_data_texture_t *textures)
{
    renderer->renderbuffers = renderbuffers;
    renderer->textures = textures;
    renderer->current_frame = 0;
    renderer->uniform_buffer_index = 0;
    renderer->current_shader = NULL;
    renderer->backend = memory_alloc(sizeof(renderer_backend_t), MEM_TAG_PERMANENT); 
    renderer->backend->initialize = NULL;
#if defined (VULKAN_BACKEND)
    renderer->backend->initialize = vulkan_backend_initialize;
    renderer->backend->shutdown   = vulkan_backend_shutdown;
    renderer->backend->window_destroy = vulkan_backend_window_destroy;
    renderer->backend->frame_prepare = vulkan_backend_frame_prepare;
    renderer->backend->frame_submit = vulkan_backend_frame_submit;
    renderer->backend->begin_rendering = vulkan_backend_begin_rendering;
    renderer->backend->end_rendering = vulkan_backend_end_rendering;
    renderer->backend->set_viewport = vulkan_backend_set_viewport;
    renderer->backend->set_scissor = vulkan_backend_set_scissor;
    renderer->backend->get_num_frames_in_flight = vulkan_backend_get_number_of_frames_in_flight;
    renderer->backend->create_renderbuffer = vulkan_backend_create_renderbuffer;
    renderer->backend->create_shader = vulkan_backend_create_shader;
    renderer->backend->initialize_shader = vulkan_backend_initialize_shader;
    renderer->backend->use_shader = vulkan_backend_use_shader;
    renderer->backend->shader_bind_resource = vulkan_backend_shader_bind_resource;
    renderer->backend->create_render_data = vulkan_backend_create_render_data;
    renderer->backend->create_texture = vulkan_backend_create_texture;
    renderer->backend->copy_to_renderbuffer = vulkan_backend_copy_to_renderbuffer;
    renderer->backend->bind_index_buffers = vulkan_backend_bind_index_buffers;
    renderer->backend->bind_vertex_buffers = vulkan_backend_bind_vertex_buffers;
    renderer->backend->push_constants = vulkan_backend_push_constants;
    renderer->backend->draw_indexed = vulkan_backend_draw_indexed;
#endif
    if (!renderer->backend->initialize) {
        LOGE("No rendering backend provided. Aborting");
        return false;
    }

    if (!renderer->backend->initialize(renderer->backend, window, &config)) {
        LOGE("Error Initializing renderer");
        return  false;
    }

    renderer->max_frames_in_flight = get_number_of_frames(renderer->backend);

    renderer->shaders = memory_alloc(sizeof(shader_t) * MAX_SHADER_COUNT, MEM_TAG_PERMANENT);
    renderer->shader_count = MAX_SHADER_COUNT;
    
    return true;
}

void renderer_shutdown(renderer_t *renderer)
{
    renderer->backend->shutdown(renderer->backend);
    renderer->backend = NULL;
}

void renderer_destroy_window(renderer_t *renderer, window_t *window)
{
    renderer->backend->window_destroy(renderer->backend, window);
}

bool renderer_frame_submit(renderer_t *renderer, frame_data_t *frame_data)
{
    bool success = renderer->backend->frame_submit(renderer->backend, NULL);
    if (success) {
        renderer->current_frame = (renderer->current_frame + 1) % renderer->max_frames_in_flight;
    }
    return success;
}

bool renderer_frame_prepare(renderer_t *renderer, frame_data_t *frame_data)
{
    return renderer->backend->frame_prepare(renderer->backend, frame_data);
}

bool renderer_end_rendering(renderer_t *renderer)
{
    return renderer->backend->end_rendering(renderer->backend);
}

bool renderer_begin_rendering(renderer_t *renderer)
{
    return renderer->backend->begin_rendering(renderer->backend);
}

bool renderer_set_viewport(renderer_t *renderer, float w, float h, float minz, float maxz)
{
    return renderer->backend->set_viewport(renderer->backend, w, h, minz, maxz);
}

bool renderer_set_scissor(renderer_t *renderer, float x, float y, float w, float h)
{
    return renderer->backend->set_scissor(renderer->backend, x, y, w, h);
}

bool renderer_use_shader(renderer_t *renderer, renderer_shader_type_e shader_type)
{
    if (shader_type < 0 || shader_type >= MAX_SHADER_COUNT) return false;
    bool success = renderer->backend->use_shader(renderer->backend, &renderer->shaders[shader_type]);
    if (success)
    {
        renderer->current_shader = &renderer->shaders[shader_type];
    }
    return success;
}

bool renderer_create_shader(renderer_t *renderer, renderer_shader_type_e shader_type)
{
    if (shader_type < 0 || shader_type >= MAX_SHADER_COUNT) return false;

    shader_t *shader = &renderer->shaders[shader_type];
    shader->type = shader_type;

    const char *vert_code = NULL;
    const char *frag_code = NULL;
    
    switch (shader_type)
    {
        case SHADER_TYPE_PBR_SKINNED:
            vert_code = "./assets/shaders/pbr.vert.spv";
            frag_code = "./assets/shaders/pbr.frag.spv";
            break;
        case SHADER_TYPE_SKINNED_GEOMETRY:
            vert_code = "./assets/shaders/skinnedmodel.vert.spv";
            frag_code = "./assets/shaders/skinnedmodel.frag.spv";
            break;
        case SHADER_TYPE_STATIC_GEOMETRY:
        default:
            assert(false && "Only pbr and skinned geometry shaders are currently supported");
            break;
    }

    bool success =  renderer->backend->create_shader(renderer->backend, shader, vert_code, frag_code);

    if (renderer->uniform_buffer_index == 0){
        renderer->uniform_buffer_index = bulk_data_allocate_slot_renderbuffer_t(renderer->renderbuffers);
        renderbuffer_t *renderbuffer = bulk_data_getp_null_renderbuffer_t(renderer->renderbuffers, renderer->uniform_buffer_index);
        renderer_create_renderbuffer(renderer, 
                                     renderbuffer, 
                                     RENDERBUFFER_TYPE_UNIFORM_BUFFER,
                                     NULL, 
                                     sizeof(scene_uniform_data_t));

        renderer->uniform_buffer_render_data = renderer_create_render_data(renderer, RENDER_DATA_SCENE_UNIFORMS, renderbuffer);
    }
    return success;
}

bool renderer_create_texture(renderer_t *renderer, texture_t *texture, const char *file_path)
{
    return renderer->backend->create_texture(renderer->backend, texture, file_path);
}

bool renderer_create_renderbuffer(renderer_t *renderer, 
                                  renderbuffer_t *renderbuffer,
                                  renderbuffer_type_e type, 
                                  uint8_t *buffer_data, 
                                  uint32_t size)
{
    renderbuffer->type = type;
    renderbuffer->size = size;
    return renderer->backend->create_renderbuffer(renderer->backend, renderbuffer, type, buffer_data, size);
}

void renderer_copy_to_renderbuffer(renderer_t *renderer, renderbuffer_t *renderbuffer, void *src, uint32_t size)
{
    return renderer->backend->copy_to_renderbuffer(renderer->backend, renderbuffer, src, size);
}

bool renderer_bind_vertex_buffers(renderer_t *renderer, renderbuffer_t *vertex_buffer)
{
    return renderer->backend->bind_vertex_buffers(renderer->backend, vertex_buffer);
}

bool renderer_bind_index_buffers(renderer_t *renderer, renderbuffer_t *index_buffer)
{
    return renderer->backend->bind_index_buffers(renderer->backend, index_buffer);
}

bool renderer_push_constants(renderer_t *renderer, shader_t *shader, const void *data, uint32_t size, uint32_t offset, renderer_shader_stage_e shader_stage)
{
    return renderer->backend->push_constants(renderer->backend, shader, data, size, offset, shader_stage);
}

bool renderer_draw_indexed(renderer_t *renderer, int32_t vertex_offset, uint32_t first_index, uint32_t index_count, uint32_t first_instance, uint32_t instance_count)
{
    return renderer->backend->draw_indexed(renderer->backend, vertex_offset, first_index, index_count, first_instance, instance_count);
}

void *renderer_create_render_data(renderer_t *renderer, render_data_type_e type, void *data)
{
    return renderer->backend->create_render_data(renderer->backend, renderer->renderbuffers, type, data);
}

bool renderer_initialize_shader(renderer_t *renderer, renderer_shader_type_e shader_type, shader_resource_list_t *resources)
{
    return renderer->backend->initialize_shader(renderer->backend, &renderer->shaders[shader_type], resources);
}

bool renderer_shader_bind_resource(renderer_t *renderer, renderer_shader_type_e shader_type, render_data_type_e type, void *render_data)
{
    return renderer->backend->shader_bind_resource(renderer->backend, &renderer->shaders[shader_type], type, render_data);
}

