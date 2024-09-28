#ifndef RENDERER_H_
#define RENDERER_H_

#include "renderer_types.h"
#include "../../platform/platform.h"

bool renderer_initialize(renderer_t *renderer, window_t *window, renderer_config_t config);
void renderer_shutdown(renderer_t *renderer);
void renderer_destroy_window(renderer_t *renderer, window_t *window);
bool renderer_frame_prepare(renderer_t *renderer, frame_data_t *frame_data);

bool renderer_begin_rendering(renderer_t *renderer);
bool renderer_end_rendering(renderer_t *renderer);

bool renderer_set_viewport(renderer_t *renderer, float w, float h, float minz, float maxz);
bool renderer_set_scissor(renderer_t *renderer, float x, float y, float w, float h);

bool renderer_create_texture(renderer_t *renderer, texture_t *texture, const char *file_path);
bool renderer_create_renderbuffer(renderer_t *renderer, renderbuffer_t *renderbuffer, renderbuffer_type_e type, shader_t *shader, uint8_t *data, uint32_t size);
void renderer_copy_to_renderbuffer(renderer_t *renderer, renderbuffer_t *renderbuffer, void *src, uint32_t size);

bool renderer_create_shader(renderer_t *renderer, renderer_shader_type_e shader_type);
bool renderer_use_shader(renderer_t *renderer, renderer_shader_type_e shader_type);

bool renderer_bind_vertex_buffers(renderer_t *renderer, renderbuffer_t *vertex_buffer);
bool renderer_bind_index_buffers(renderer_t *renderer, renderbuffer_t *index_buffer);


#endif


