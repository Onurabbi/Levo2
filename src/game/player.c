#include "player.h"
#include "entity.h"

#include <assert.h>
#include <SDL2/SDL.h>

static void animate_player(player_t *player, 
                          float      delta_time, 
                          uint32_t   anim_index, 
                          bulk_data_animation_t *animations)
{
    //then animate it
    if (anim_index != player->current_animation)
    {
        player->current_animation   = anim_index;
        player->anim_timer          = 0;
    }
    else
    {
        player->anim_timer += delta_time;
    }

    animation_t *playing_animation = bulk_data_getp_raw(animations, player->current_animation);

    if (player->anim_timer >= playing_animation->duration) player->anim_timer -= playing_animation->duration;
    
    float sec_per_frame = playing_animation->duration / (float)playing_animation->sprite_count;

    player->current_animation_frame = floorf(player->anim_timer / sec_per_frame);
}

void update_player(entity_t              *e, 
                   input_t               *input, 
                   float                  delta_time, 
                   bulk_data_entity_t    *entities,
                   bulk_data_animation_t *animations)
{
    player_t *player = (player_t*)e->data;

    vec2f_t dp = {0.0f, 0.0f};
    if (input->keyboard_state[SDL_SCANCODE_W])
    {
        dp.y = -player->velocity;
    }
    if (input->keyboard_state[SDL_SCANCODE_S])
    {
        dp.y = player->velocity;
    }
    if (input->keyboard_state[SDL_SCANCODE_A])
    {
        dp.x = -player->velocity;
    }
    if (input->keyboard_state[SDL_SCANCODE_D])
    {
        dp.x = player->velocity;
    }

    dp.x *= delta_time;
    dp.y *= delta_time;

    uint32_t anim_index = 0;

    switch (e->state)
    {
        case(ENTITY_STATE_IDLE):
        {
            if (dp.x > 0.0f || dp.y > 0.0f)
            {
                e->state = ENTITY_STATE_RUN;
                anim_index = 0;
            }
            break;
        }
        case(ENTITY_STATE_RUN):
        {
            anim_index = 0;
            if (dp.x == 0.0f && dp.y == 0.0f)
            {
                e->state = ENTITY_STATE_IDLE;
                anim_index = 0;
            }
            break;
        }
        default:
            assert(false && "unknown entity state");
            break;
    }

    animate_player(player, delta_time, anim_index, animations);
    //first move the entity
    move_entity(e, dp, entities);
}

