#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "vulkan_types.h"
#include "../frontend/renderer_types.h"
#include "../../../game_types.h"

#include <SDL2/SDL.h>

bool vulkan_backend_initialize(struct renderer_backend_t *backend, window_t *window, renderer_config_t *config);
void vulkan_backend_shutdown(struct renderer_backend_t *backend);
bool vulkan_backend_begin_rendering(struct renderer_backend_t *backend);
bool vulkan_backend_end_rendering(struct renderer_backend_t *backend);
void vulkan_backend_window_destroy(struct renderer_backend_t *backend, window_t *window);
bool vulkan_backend_frame_prepare(struct renderer_backend_t *backend, frame_data_t *frame_data);
bool vulkan_backend_frame_submit(struct renderer_backend_t *backend, frame_data_t *frame_data);
bool vulkan_backend_set_viewport(struct renderer_backend_t *backend, float w, float h, float minz, float maxz);
bool vulkan_backend_set_scissor(struct renderer_backend_t *backend, float x, float y, float w, float h);
uint32_t vulkan_backend_get_number_of_frames_in_flight(struct renderer_backend_t *backend);

bool vulkan_backend_create_renderbuffer(struct renderer_backend_t *backend, renderbuffer_t *renderbuffer, renderbuffer_type_e renderbuffer_type, uint8_t *buffer_data, uint32_t size);
void vulkan_backend_copy_to_renderbuffer(struct renderer_backend_t *backend, renderbuffer_t *renderbuffer, void *src, uint32_t size);

bool vulkan_backend_bind_vertex_buffers(struct renderer_backend_t *backend, renderbuffer_t *vertex_buffer);
bool vulkan_backend_bind_index_buffers(struct renderer_backend_t *backend, renderbuffer_t *index_buffer);

bool vulkan_backend_create_shader(struct renderer_backend_t *backend, shader_t *shader);
bool vulkan_backend_use_shader(struct renderer_backend_t *backend, shader_t *shader);
bool vulkan_backend_initialize_shader(renderer_backend_t *backend, shader_t *shader, shader_resource_list_t *resources);
bool vulkan_backend_shader_bind_resource(renderer_backend_t *backend, shader_t *shader, render_data_type_e type, void *render_data);

void *vulkan_backend_create_render_data(struct renderer_backend_t *backend, render_data_config_t *config);

bool vulkan_backend_create_texture(struct renderer_backend_t *backend, texture_t *texture, const char *file_path);

bool vulkan_backend_push_constants(struct renderer_backend_t *backend, shader_t *shader, const void *data, uint32_t size, uint32_t offset, renderer_shader_stage_e shader_stage);
bool vulkan_backend_draw_indexed(struct renderer_backend_t *backend, int32_t vertex_offset, uint32_t first_index, uint32_t index_count, uint32_t first_instance, uint32_t instance_count);

#endif
