#ifndef PLAYER_H_
#define PLAYER_H_

#include "game_types.h"
#include "../cJSON/cJSON.h"

void update_player(entity_t              *e, 
                   input_t               *input, 
                   float                  delta_time, 
                   bulk_data_entity_t    *entities);
#endif

