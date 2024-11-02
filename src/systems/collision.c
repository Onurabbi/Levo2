#include "collision.h"

#include "../core/math/math_utils.h"
#include "../core/logger/logger.h"
#include "../game_types.h"

#include <math.h>
#include <stdint.h>

static void swapf(float *a, float *b)
{
    float temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

bool ray_vs_rect(vec2f_t ray_origin, vec2f_t ray_dir, rect_t target, vec2f_t *contact_point, vec2f_t *contact_normal, float *t_hit_near)
{
    float t_near_x = ((float)target.min.x - ray_origin.x) / ray_dir.x;
    float t_near_y = ((float)target.min.y - ray_origin.y) / ray_dir.y;
    float t_far_x  = ((float)target.min.x + (float)target.size.x - ray_origin.x) / ray_dir.x;
    float t_far_y  = ((float)target.min.y + (float)target.size.y - ray_origin.y) / ray_dir.y;

    if (isnan(t_far_y) || isnan(t_far_x)) return false;
    if (isnan(t_near_y) || isnan(t_near_x)) return false;

    if (t_near_x > t_far_x) swapf(&t_near_x, &t_far_x);
    if (t_near_y > t_far_y) swapf(&t_near_y, &t_far_y);

    if (t_near_x > t_far_y || t_near_y > t_far_x) return false;

    *t_hit_near = MAX(t_near_x, t_near_y);
    float t_hit_far = MIN(t_far_x, t_far_y);

    if (t_hit_far < 0) return false;
    
    *contact_point = (vec2f_t){ray_origin.x + ray_dir.x * (*t_hit_near), ray_origin.y + ray_dir.y * (*t_hit_near)};

    if (t_near_x > t_near_y) {
        if (ray_dir.x < 0) {
            *contact_normal = (vec2f_t){1,0};
        } else {
            *contact_normal = (vec2f_t){-1,0};
        }
    } else if (t_near_x < t_near_y) {
        if (ray_dir.y < 0) {
            *contact_normal = (vec2f_t){0,1};
        } else {
            *contact_normal = (vec2f_t){0,-1};
        }
    }
    return (*t_hit_near >= 0.0f && *t_hit_near < 1.0f);
}


bool resolve_dyn_rect_vs_rect(rect_t r_dyn, 
                              rect_t r_st, 
                              vec2f_t dp,
                              vec2f_t *contact_point,
                              vec2f_t *contact_normal, 
                              float *t_hit)
{
    vec2f_t ray_origin = (vec2f_t){r_dyn.min.x + r_dyn.size.x / 2, r_dyn.min.y + r_dyn.size.y / 2};
    rect_t target      = (rect_t){{r_st.min.x - r_dyn.size.x / 2, r_st.min.y - r_dyn.size.y / 2}, {r_dyn.size.x + r_st.size.x, r_dyn.size.y + r_st.size.x}};

    return ray_vs_rect(ray_origin, dp, target, contact_point, contact_normal, t_hit);
}

bool rect_vs_rect(rect_t r1, rect_t r2)
{
    if (r1.min.x + r1.size.x <= r2.min.x || r1.min.x >= r2.min.x + r2.size.x) return false;
    if (r1.min.y + r1.size.y <= r2.min.y || r1.min.y >= r2.min.y + r2.size.y) return false;
    return true;
}
