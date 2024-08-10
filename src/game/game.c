#include "game.h"
#include "input.h"


#include "collision.h"
#include "entity.h"
#include "player.h"

#include "../logger/logger.h"
#include "../memory/memory_arena.h"
#include "../cJSON/cJSON.h"
#include "../containers/string.h"

#include <SDL2/SDL.h>
#include <assert.h>

#define DELTA_TIME 1.0/60.0

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

static void load_level(game_t *game, const char *data_file_path)
{
    FILE *file = fopen(data_file_path, "r");
    if (!file)
    {
        LOGE("Unable to load %s\n", data_file_path);
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_buf = memory_arena_push_array(scratch_allocator(), char, file_size + 1);
    assert(file_buf);

    fread(file_buf, 1, file_size, file);
    file_buf[file_size] = '\0';
    fclose(file);

    cJSON *root = cJSON_Parse(file_buf);
    if (!root)
    {
        LOGE("Unable to parse json file");
        return;
    }

    //! NOTE: Load assets
    cJSON *assets = cJSON_GetObjectItem(root, "assets");
    if (assets)
    {
        cJSON *file_paths = cJSON_GetObjectItem(assets, "filepaths");
        if (file_paths)
        {
            cJSON *pair;
            cJSON_ArrayForEach(pair, file_paths)
            {
                const char *key = string_duplicate(cJSON_GetStringValue(cJSON_GetObjectItem(pair, "key")));
                const char *value = string_duplicate(cJSON_GetStringValue(cJSON_GetObjectItem(pair, "value")));

                if (key && value)
                {
                    LOGI("key: %s value: %s", key, value);
                    asset_store_add_texture(&game->asset_store, &game->renderer, key, value);
                }
                else
                {
                    LOGE("Unable to parse key value pair");
                }
            }
        }
        else
        {
            LOGE("Unable to parse file paths");
            goto exit;
        }
    }
    else
    {
        LOGE("Unable to parse assets node");
        goto exit;
    }

    //! NOTE: Load Entities
    cJSON *entities = cJSON_GetObjectItem(root, "entities");
    if (entities)
    {
        //! Load the player
        cJSON* player_node = cJSON_GetObjectItem(entities, "player");
        if (player_node)
        {
            entity_t *player_entity = bulk_data_push_struct(&game->entities);
            player_t *player = &game->player_data;

            game->player_entity = player_entity;
            player_entity->data = player;

            player_entity->id = game->entity_id++;
            player_entity->type = ENTITY_TYPE_PLAYER;
            player_entity->dp = (vec2f_t){0.0f, 0.0f};

            cJSON *node = NULL;
            node = cJSON_GetObjectItem(player_node, "position");
            player_entity->p.x = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "x"));
            player_entity->p.y = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "y"));

            node = cJSON_GetObjectItem(player_node, "size");
            player_entity->size.x = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "x"));
            player_entity->size.y = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "y"));
            player_entity->z_index = (int32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(player_node, "z_index"));

            player->entity = player_entity;
            player->velocity = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(player_node, "velocity"));

            cJSON *child = NULL;
            node = cJSON_GetObjectItem(player_node, "animations");
            cJSON_ArrayForEach(child, node)
            {
                animation_t *animation = bulk_data_push_struct(&game->animations);
                player->animations[player->animation_count++] = animation;
                const char *texture_id = cJSON_GetStringValue(cJSON_GetObjectItem(child, "texture"));
                animation->texture = asset_store_get_texture(&game->asset_store, texture_id);
                animation->duration = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(child, "duration"));
                animation->sprite_count = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(child, "sprite_count"));

                cJSON *grand_child = cJSON_GetObjectItem(child, "offset");
                uint32_t offset_x = cJSON_GetNumberValue(cJSON_GetObjectItem(grand_child, "x"));
                uint32_t offset_y = cJSON_GetNumberValue(cJSON_GetObjectItem(grand_child, "y"));

                grand_child = cJSON_GetObjectItem(child, "size");
                float w = cJSON_GetNumberValue(cJSON_GetObjectItem(grand_child, "x"));
                float h = cJSON_GetNumberValue(cJSON_GetObjectItem(grand_child, "y"));
                uint32_t stride = cJSON_GetNumberValue(cJSON_GetObjectItem(child, "stride"));

                for (uint32_t i = 0; i < animation->sprite_count; i++)
                {
                    animation->sprites[i] = (rect_t){{offset_x + i * stride, offset_y}, {w,h}};
                }
            }
        }
        else
        {
            LOGE("NOOOO");
        }

        //! Load tiles
        cJSON *tiles = cJSON_GetObjectItem(root, "tilemap");
        if (tiles)
        {
            const char *map_file_path = cJSON_GetStringValue(cJSON_GetObjectItem(tiles, "map_file"));
            if (!map_file_path)
            {
                LOGE("NOOOOO");
                goto exit;
            }

            FILE *map_file = fopen(map_file_path, "r");
            if (!map_file)
            {
                LOGE("NOOOOOOOOOO");
                goto exit;
            }

            uint32_t rows  = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(tiles, "row_count"));
            uint32_t cols  = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(tiles, "col_count"));
            uint32_t tile_size = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(tiles, "tile_size"));
            float scale = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(tiles, "scale"));
            
            const char *texture_path = cJSON_GetStringValue(cJSON_GetObjectItem(tiles, "texture"));
            vulkan_texture_t *tiletex = asset_store_get_texture(&game->asset_store, texture_path);
            assert(tiletex);

            for (uint32_t row = 0; row < rows; row++)
            {
                for (uint32_t col = 0; col < cols; col++)
                {
                    int tile_row = fgetc(map_file) - '0';
                    int tile_col = fgetc(map_file) - '0';
                    if (tile_row == EOF || tile_col == EOF)
                    {
                        LOGE("NOOOOOO");
                        fclose(map_file);
                        goto exit;
                    }

                    fgetc(map_file);
                }
                fgetc(map_file);
            }
        }
        else
        {
            LOGE("NOOOOOO");
        }
    }
    else
    {
        LOGE("Unable to parse entities node");
        goto exit;
    }
exit:
    cJSON_Delete(root);
}

static void setup(game_t *game)
{
    load_level(game, "./assets/data/level1.json");

    entity_t *tile = bulk_data_push_struct(&game->entities);
    tile->id   = game->entity_id++;
    tile->type = ENTITY_TYPE_TILE;
    tile->p    = (vec2f_t){128.0f,128.0f};
    tile->size = (vec2f_t){64.0f, 64.0f};
    tile->dp   = (vec2f_t){0.0f};
    tile->z_index = 0;

    tile_t *tile_data = bulk_data_push_struct(&game->tiles);
    tile_data->entity = tile;
    tile_data->sprite.texture  = asset_store_get_texture(&game->asset_store, "dungeon-prison-tiles");
    tile_data->sprite.src_rect = (rect_t){{0,0},{32,32}};
    tile->data = tile_data;

    tile = bulk_data_push_struct(&game->entities);
    tile->id   = game->entity_id++;
    tile->type = ENTITY_TYPE_TILE;
    tile->p    = (vec2f_t){192.0f,128.0f};
    tile->size = (vec2f_t){64.0f, 64.0f};
    tile->dp   = (vec2f_t){0.0f};
    tile->z_index = 0;

    tile_data = bulk_data_push_struct(&game->tiles);
    tile_data->entity = tile;
    tile_data->sprite.texture  = asset_store_get_texture(&game->asset_store, "dungeon-prison-tiles");
    tile_data->sprite.src_rect = (rect_t){{0,0},{64,64}};
    tile->data = tile_data;

    tile = bulk_data_push_struct(&game->entities);
    tile->id   = game->entity_id++;
    tile->type = ENTITY_TYPE_TILE;
    tile->p    = (vec2f_t){256.0f,128.0f};
    tile->size = (vec2f_t){64.0f, 64.0f};
    tile->dp   = (vec2f_t){0.0f};
    tile->z_index = 0;

    tile_data = bulk_data_push_struct(&game->tiles);
    tile_data->entity = tile;
    tile_data->sprite.texture = asset_store_get_texture(&game->asset_store, "dungeon-prison-tiles");
    tile_data->sprite.src_rect = (rect_t){{0,0},{32,32}};
    tile->data = tile_data;

    tile = bulk_data_push_struct(&game->entities);
    tile->id   = game->entity_id++;
    tile->type = ENTITY_TYPE_TILE;
    tile->p    = (vec2f_t){320.0f,128.0f};
    tile->size = (vec2f_t){64.0f, 64.0f};
    tile->dp   = (vec2f_t){0.0f};
    tile->z_index  = 0;

    tile_data = bulk_data_push_struct(&game->tiles);
    tile_data->entity = tile;
    tile_data->sprite.texture  = asset_store_get_texture(&game->asset_store, "dungeon-prison-tiles");
    tile_data->sprite.src_rect = (rect_t){{0,0},{32,32}};
    tile->data = tile_data;

    game->performance_freq = SDL_GetPerformanceFrequency();
    game->previous_counter = SDL_GetPerformanceCounter();

    game->camera.size.x = WINDOW_WIDTH;
    game->camera.size.y = WINDOW_HEIGHT;
    game->camera.min.x  = game->player_entity->p.x - WINDOW_WIDTH / 2;
    game->camera.min.y  = game->player_entity->p.y - WINDOW_HEIGHT / 2;

    LOGI("Setup complete");
}

static void update(game_t *game)
{
    //if quit was requested, quit now
    if (game->input.quit)
    {
        game->is_running = false;
        return;
    }

    memory_arena_begin(scratch_allocator());

    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t elapsed = now - game->previous_counter;
    game->previous_counter = now;
    
    double sec = (double)elapsed / game->performance_freq;
    //LOGI("FPS: %lf", 1.0 / sec);
    game->accumulator += sec;

    while (game->accumulator > 1.0 / 61.0)
    {
        //update all entities
        for (uint32_t i = 0; i < bulk_data_size(&game->entities); i++)
        {
            entity_t *e = bulk_data_getp_null(&game->entities, i);
            if (e)
            {
                switch (e->type)
                {
                    case(ENTITY_TYPE_PLAYER):
                    {
                        update_player(e, &game->input, DELTA_TIME, &game->entities, &game->animations);
                        break;
                    }
                    default: 
                        break;
                }
            }
        }

        //update camera location
        game->camera.min.x = game->player_entity->p.x - game->camera.size.x / 2;
        game->camera.min.y = game->player_entity->p.y - game->camera.size.y / 2;

        game->accumulator -= 1.0 / 59.0;
        if (game->accumulator < 0) game->accumulator = 0.0;
    }
}


static void render(game_t *game)
{
    vulkan_renderer_render_entities(&game->renderer, &game->entities, &game->animations, &game->sprites, game->camera);
    vulkan_renderer_present(&game->renderer);
}

void game_run(game_t *game)
{
    setup(game);
    while(game->is_running)
    {
        process_input(&game->input);
        update(game);
        render(game);
    }
}


void game_init(game_t *game)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        LOGE("Everything initializing SDL.");
        return;
    }

    game->window = SDL_CreateWindow(NULL,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN
    );

    assert(game->window);

    game->is_running = false;

    memory_arena_init();
    
    asset_store_init(&game->asset_store);
    vulkan_renderer_init(&game->renderer, game->window);

    game->entity_id = 0;

    bulk_data_init(&game->entities, entity_t);
    bulk_data_init(&game->tiles, tile_t);
    bulk_data_init(&game->animations, animation_t);
    bulk_data_init(&game->sprites, sprite_t);

    game->is_running = true;
}

