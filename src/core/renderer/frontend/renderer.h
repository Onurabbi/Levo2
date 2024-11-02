#ifndef RENDERER_H_
#define RENDERER_H_

#include "renderer_types.h"
#include "../../platform/platform.h"

bool renderer_initialize(renderer_t *renderer, window_t *window, renderer_config_t config);
void renderer_shutdown(renderer_t *renderer);
void renderer_destroy_window(renderer_t *renderer, window_t *window);

bool renderer_frame_prepare(renderer_t *renderer, frame_data_t *frame_data);
bool renderer_frame_submit(renderer_t *renderer, frame_data_t *frame_data);

bool renderer_begin_rendering(renderer_t *renderer);
bool renderer_end_rendering(renderer_t *renderer);

bool renderer_set_viewport(renderer_t *renderer, float w, float h, float minz, float maxz);
bool renderer_set_scissor(renderer_t *renderer, float x, float y, float w, float h);

bool renderer_create_texture(renderer_t *renderer, texture_t *texture, const char *file_path);

bool renderer_create_renderbuffer(renderer_t *renderer, renderbuffer_t *renderbuffer, renderbuffer_type_e type, uint8_t *data, uint32_t size);
void renderer_copy_to_renderbuffer(renderer_t *renderer, renderbuffer_t *renderbuffer, void *src, uint32_t size);

bool renderer_create_shader(renderer_t *renderer, bulk_data_renderbuffer_t *renderbuffers, renderer_shader_type_e shader_type);
bool renderer_use_shader(renderer_t *renderer, renderer_shader_type_e shader_type);
bool renderer_initialize_shader(renderer_t *renderer, renderer_shader_type_e shader_type, shader_resource_list_t *resource_list);

void *renderer_create_render_data(renderer_t *renderer, render_data_config_t *config);
bool renderer_shader_bind_resource(renderer_t *renderer, renderer_shader_type_e shader_type, render_data_type_e type, void *render_data);
bool renderer_bind_vertex_buffers(renderer_t *renderer, renderbuffer_t *vertex_buffer);
bool renderer_bind_index_buffers(renderer_t *renderer, renderbuffer_t *index_buffer);

bool renderer_push_constants(renderer_t *renderer, shader_t *shader, const void *data, uint32_t size, uint32_t offset, renderer_shader_stage_e shader_stage);
bool renderer_draw_indexed(renderer_t *renderer, int32_t vertex_offset, uint32_t first_index, uint32_t index_count, uint32_t first_instance, uint32_t instance_count);

#endif


