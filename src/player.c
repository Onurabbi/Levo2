#include "player.h"
#include "entity.h"

#include <animation.h>
#include <math_utils.h>
#include <string_utils.h>
#include <logger.h>

#include <assert.h>
#include <SDL2/SDL.h>

void update_player(entity_t              *e, 
                   input_t               *input, 
                   float                  delta_time, 
                   bulk_data_entity_t    *entities)
{
    player_t *player = (player_t*)e->data;

    vec2f_t dp = {0.0f, 0.0f};
    if (input->keyboard_state[SDL_SCANCODE_W]) {
        dp.y = -1.0f;
    }
    if (input->keyboard_state[SDL_SCANCODE_S]) {
        dp.y = 1.0f;
    }
    if (input->keyboard_state[SDL_SCANCODE_A]) {
        dp.x = -1.0f;
    }
    if (input->keyboard_state[SDL_SCANCODE_D]) {
        dp.x = 1.0f;
    }

    dp = vec2_normalize(dp);
    dp = vec2_multiply(dp, (vec2f_t){player->velocity * delta_time, player->velocity * delta_time});

    switch (e->state)
    {
        case(ENTITY_STATE_IDLE): 
            if (dp.x != 0.0f || dp.y != 0.0f) {
                e->state = ENTITY_STATE_RUN;
            }
            break;
        case(ENTITY_STATE_RUN):
            if (dp.x == 0.0f && dp.y == 0.0f) {
                e->state = ENTITY_STATE_IDLE;
            }
            break;
        default:
            assert(false && "unknown entity state");
            break;
    }
    
    //first move the entity
    move_entity(e, dp, entities);
}

