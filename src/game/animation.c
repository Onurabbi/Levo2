#include "animation.h"
#include "game_types.h"

#include <string.h>
#include <assert.h>
#include <math.h>

entity_state_t get_animation_state_from_string(const char *state_string)
{
    if (strcmp(state_string, "idle") == 0) return ENTITY_STATE_IDLE;
    if (strcmp(state_string, "run") == 0) return ENTITY_STATE_RUN;
    if (strcmp(state_string, "melee") == 0) return ENTITY_STATE_MELEE;
    if (strcmp(state_string, "ranged") == 0) return ENTITY_STATE_RANGED;
    if (strcmp(state_string, "hit") == 0) return ENTITY_STATE_HIT;
    if (strcmp(state_string, "dead") == 0) return ENTITY_STATE_DEAD;
    return ENTITY_STATE_MAX;
}

//! NOTE:Find an animation from an entities chunks, given its state (idle, run etc.)
//! TODO: This uses linear search, since we're only going to go through 4*8 elements 
static animation_t *get_animation_by_state(animation_chunk_t **animation_chunks, uint32_t chunk_count, entity_state_t state)
{
    for (uint32_t i = 0; i < chunk_count; i++)
    {
        animation_chunk_t *chunk = animation_chunks[i];
        for (uint32_t j = 0; j < chunk->count; j++)
        {
            animation_t *animation = &chunk->animations[j];
            if (animation->state == state)
            {
                return animation;
            }
        }
    }
    assert(false);
    return NULL;
}

uint32_t animation_get_current_frame(animation_t *animation, float timer)
{
    float sec_per_frame = animation->duration / (float)animation->sprite_count;
    return floorf(timer / sec_per_frame);
}

animation_update_result_t animation_update(animation_chunk_t **animation_chunks,
                                           uint32_t            chunk_count,
                                           animation_t        *current_animation,
                                           entity_state_t      new_animation_state,
                                           float               delta_time,
                                           float               timer)
{
    animation_update_result_t result = {0};
    if (current_animation->state != new_animation_state)
    {
        result.animation = get_animation_by_state(animation_chunks, chunk_count, new_animation_state);
        result.timer = 0.0f;
        return result;
    }

    timer += delta_time;
    if (timer >= current_animation->duration) timer -= current_animation->duration;
    
    result.timer = timer;
    result.animation = current_animation;
    return result;
}

