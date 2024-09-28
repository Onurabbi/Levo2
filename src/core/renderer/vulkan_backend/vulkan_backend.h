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
bool vulkan_backend_set_viewport(struct renderer_backend_t *backend, float w, float h, float minz, float maxz);
bool vulkan_backend_set_scissor(struct renderer_backend_t *backend, float x, float y, float w, float h);
uint32_t vulkan_backend_get_number_of_frames_in_flight(struct renderer_backend_t *backend);

bool vulkan_backend_create_renderbuffer(struct renderer_backend_t *backend, renderbuffer_t *renderbuffer, renderbuffer_type_e renderbuffer_type, shader_t *shader,uint8_t *buffer_data, uint32_t size);
void vulkan_backend_copy_to_renderbuffer(struct renderer_backend_t *backend, renderbuffer_t *renderbuffer, void *src, uint32_t size);
bool vulkan_backend_bind_vertex_buffers(struct renderer_backend_t *backend, renderbuffer_t *vertex_buffer);
bool vulkan_backend_bind_index_buffers(struct renderer_backend_t *backend, renderbuffer_t *index_buffer);

bool vulkan_backend_create_shader(struct renderer_backend_t *backend, shader_t *shader);
bool vulkan_backend_use_shader(renderer_backend_t *backend, shader_t *shader);
bool vulkan_backend_create_texture(struct renderer_backend_t *backend, texture_t *texture, const char *file_path);

//! Not yet implemented
bool vulkan_renderer_viewport_set(struct renderer_backend_t *backend, vec4f_t rect);
bool vulkan_renderer_viewport_reset(struct renderer_backend_t *backend);
bool vulkan_renderer_scissor_set(struct renderer_backend_t *backend, vec4f_t rect);
bool vulkan_renderer_scissor_reset(struct renderer_backend_t *backend);

void vulkan_renderer_winding_set(struct renderer_backend_t *backend, renderer_winding_e winding);
void vulkan_renderer_set_stencil_test_enabled(struct renderer_backend_t *backend, bool enabled);
void vulkan_renderer_set_depth_test_enabled(struct renderer_backend_t *backend, bool enabled);
void vulkan_renderer_set_depth_write_enabled(struct renderer_backend_t *backend, bool enabled);
void vulkan_renderer_set_stencil_reference(struct renderer_backend_t *backend, uint32_t reference);
void vulkan_renderer_set_stencil_op(struct renderer_backend_t *backend, renderer_stencil_op_e fail_op, renderer_stencil_op_e pass_op, renderer_stencil_op_e depth_fail_ip, renderer_compare_op_e compare_op);

#endif
