#ifndef ANIMATION_H_
#define ANIMATION_H_

#include "game_types.h"

struct cJSON;

entity_state_t get_animation_state_from_string(const char *state_string);
uint32_t animation_get_current_frame(animation_t *animation, float timer);
animation_update_result_t animation_update(animation_chunk_t **animation_chunks,
                                           uint32_t            chunk_count,
                                           animation_t        *current_animation,
                                           entity_state_t      new_animation_state,
                                           float               delta_time,
                                           float               timer);
#endif
