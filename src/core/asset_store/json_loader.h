#ifndef JSON_LOADER_H_
#define JSON_LOADER_H_

struct game;

void load_level_from_json(game_t *game, const char *data_file_path);
gltf_model_t *model_load_from_gltf(const char * path);
#endif


