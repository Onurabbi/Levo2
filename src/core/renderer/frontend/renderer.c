#include "renderer.h"
#include "../../memory/memory.h"
#include "../../logger/logger.h"

#include "../vulkan_backend/vulkan_backend.h"

enum
{
    MAX_RENDERBUFFER_COUNT = 32,
    MAX_SHADER_COUNT = 32,
};

static uint32_t get_number_of_frames(renderer_backend_t *backend)
{
    return backend->get_num_frames_in_flight(backend);
}

bool renderer_initialize(renderer_t *renderer, window_t *window, renderer_config_t config)
{
    renderer->current_frame = 0;

    renderer->backend = memory_alloc(sizeof(renderer_backend_t), MEM_TAG_PERMANENT); 
    renderer->backend->initialize = NULL;
#if defined (VULKAN_BACKEND)
    renderer->backend->initialize = vulkan_backend_initialize;
    renderer->backend->shutdown   = vulkan_backend_shutdown;
    renderer->backend->window_destroy = vulkan_backend_window_destroy;
    renderer->backend->frame_prepare = vulkan_backend_frame_prepare;
    renderer->backend->begin_rendering = vulkan_backend_begin_rendering;
    renderer->backend->end_rendering = vulkan_backend_end_rendering;
    renderer->backend->set_viewport = vulkan_backend_set_viewport;
    renderer->backend->get_num_frames_in_flight = vulkan_backend_get_number_of_frames_in_flight;
    renderer->backend->create_renderbuffer = vulkan_backend_create_renderbuffer;
    renderer->backend->create_shader = vulkan_backend_create_shader;
    renderer->backend->create_texture = vulkan_backend_create_texture;
    renderer->backend->copy_to_renderbuffer = vulkan_backend_copy_to_renderbuffer;
    renderer->backend->use_shader = vulkan_backend_use_shader;
    renderer->backend->bind_index_buffers = vulkan_backend_bind_index_buffers;
    renderer->backend->bind_vertex_buffers = vulkan_backend_bind_vertex_buffers;
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

    renderer->renderbuffers = memory_alloc(sizeof(renderbuffer_t) * MAX_RENDERBUFFER_COUNT, MEM_TAG_PERMANENT);
    renderer->renderbuffer_count = 0;

    renderer->shaders = memory_alloc(sizeof(shader_t) * MAX_SHADER_COUNT, MEM_TAG_PERMANENT);

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

bool renderer_frame_prepare(renderer_t *renderer, frame_data_t *frame_data)
{
    renderer->current_frame++;
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
    return renderer->backend->use_shader(renderer->backend, &renderer->shaders[shader_type]);
}

bool renderer_create_shader(renderer_t *renderer, renderer_shader_type_e shader_type)
{
    if (shader_type < 0 || shader_type >= MAX_SHADER_COUNT) return false;
    return renderer->backend->create_shader(renderer->backend, &renderer->shaders[shader_type]);
}

bool renderer_create_texture(renderer_t *renderer, texture_t *texture, const char *file_path)
{
    return renderer->backend->create_texture(renderer->backend, texture, file_path);
}

bool renderer_create_renderbuffer(renderer_t *renderer, 
                                  renderbuffer_t *renderbuffer, 
                                  renderbuffer_type_e type, 
                                  shader_t *shader,
                                  uint8_t *buffer_data, 
                                  uint32_t size)
{
    return renderer->backend->create_renderbuffer(renderer->backend, renderbuffer, type, shader, buffer_data, size);
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
