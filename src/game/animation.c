#include "animation.h"
#include <assert.h>
#include <math.h>

uint32_t animation_get_current_frame(animation_t *animation, float timer)
{
    float sec_per_frame = animation->duration / (float)animation->sprite_count;
    return floorf(timer / sec_per_frame);
}

float animation_update(animation_t **animations,
                       uint32_t      animation_count,
                       uint32_t      current_animation_index,
                       uint32_t      new_animation_index,
                       float         delta_time,
                       float         timer)
{
    if (new_animation_index != current_animation_index)
    {
        return 0.0f;
    }
    assert(new_animation_index < animation_count);
    timer += delta_time;
    animation_t *playing_animation = animations[current_animation_index];
    if (timer >= playing_animation->duration) timer -= playing_animation->duration;
    return timer;
}

