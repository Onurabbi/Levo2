#include "entity.h"

#include "player.h"
#include "game_types.h"

#include "systems/collision.h"
#include "core/math/math_types.h"
#include "core/logger/logger.h"

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

static int compare_collisions(const void *a, const void *b)
{
    uint_float_pair *pair1 = (uint_float_pair*)a;
    uint_float_pair *pair2 = (uint_float_pair*)b;

    if (pair1->f < pair2->f)return -1;
    if (pair1->f == pair2->f) return 0;
    else return 1;
}

void move_entity(entity_t *e, vec2f_t dp, bulk_data_entity_t *bd)
{
    //! TODO: Broadphase here 

    //first pass
    uint_float_pair pairs[4];
    entity_t *colliding_entities[4];
    uint32_t  colliding_entity_count = 0;

    for (uint32_t j = 0; j < bulk_data_size(bd); j++)
    {
        if (colliding_entity_count == 4) break;
        
        entity_t *other = bulk_data_getp_null(bd, j);

        if (other)
        {
            if (other == e) continue;
            if ((other->flags & ENTITY_CAN_COLLIDE) == 0) continue;
            
            vec2f_t contact_normal, contact_position;
            float time;
            if (resolve_dyn_rect_vs_rect(e->rect, other->rect, dp, &contact_position, &contact_normal, &time))
            {
                uint_float_pair *pair = &pairs[colliding_entity_count];
                pair->i = colliding_entity_count;
                pair->f = time;
                colliding_entities[colliding_entity_count++] = other;
            }
        }
    }

    //sort collisions in ascending time order
    qsort(pairs, colliding_entity_count, sizeof(pairs[0]), compare_collisions);

    //second pass to resolve collisions
    for (uint32_t j = 0; j < colliding_entity_count; j++)
    {
        uint32_t entity_index = pairs[j].i;
        rect_t r_st = colliding_entities[entity_index]->rect;
        vec2f_t contact_normal, contact_position;
        float time;
        if (resolve_dyn_rect_vs_rect(e->rect, r_st, dp, &contact_position, &contact_normal, &time))
        {
            dp.x += contact_normal.x * fabs(dp.x) * (1 - time);
            dp.y += contact_normal.y * fabs(dp.y) * (1 - time); 
        }
    }

    e->p.x += dp.x;
    e->p.y += dp.y;
}


