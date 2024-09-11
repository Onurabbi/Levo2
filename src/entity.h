#ifndef ENTITY_H_
#define ENTITY_H_

#include "game_types.h"
#include "../cJSON/cJSON.h"

#include "../core/containers/container_types.h"

void move_entity(entity_t *e, vec2f_t dp, bulk_data_entity_t *bd);
#endif

