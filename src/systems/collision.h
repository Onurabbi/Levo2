#ifndef COLLISION_H_
#define COLLISION_H_

#include "../core/math/math_types.h"
#include <stdbool.h>

bool rect_vs_rect(rect_t r1, rect_t r2);
bool ray_vs_rect(vec2f_t ray_origin, vec2f_t ray_dir, rect_t target, vec2f_t *contact_point, vec2f_t *contact_normal, float *t_hit_near);
bool resolve_dyn_rect_vs_rect(rect_t r_dyn, rect_t r_st, vec2f_t dp,vec2f_t *contact_point,vec2f_t *contact_normal, float *t_hit);
#endif
