#ifndef ANIMATION_H_
#define ANIMATION_H_

#include "game_types.h"

vec2f_t animation_get_current_weapon_socket(animation_t *animation, weapon_slot_t slot, float timer);
uint32_t animation_get_current_frame(animation_t *animation, float timer);
float animation_update(animation_t **animations,
                       uint32_t      animation_count,
                       uint32_t      current_animation_index,
                       uint32_t      new_animation_index,
                       float         delta_time,
                       float         timer);
#endif
