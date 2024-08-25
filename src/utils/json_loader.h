#ifndef JSON_LOADER_H_
#define JSON_LOADER_H_

#include "../game/game.h"
#include "../cJSON/cJSON.h"

void load_level_from_json(game_t *game, const char *data_file_path);

#endif


