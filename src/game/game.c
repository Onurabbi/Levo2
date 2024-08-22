#include "game.h"
#include "input.h"


#include "collision.h"
#include "entity.h"
#include "player.h"
#include "animation.h"

#include "../logger/logger.h"
#include "../memory/memory_arena.h"
#include "../cJSON/cJSON.h"
#include "../containers/string.h"
#include "../math/math_utils.h"

#include <SDL2/SDL.h>
#include <assert.h>

#define DELTA_TIME 1.0/60.0

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define TILE_WIDTH 64
#define TILE_HEIGHT 64

static entity_type_t 
entity_type_from_string(const char *str)
{
    if (strcmp(str, "player") == 0) return ENTITY_TYPE_PLAYER;
    if (strcmp(str, "weapon") == 0) return ENTITY_TYPE_WEAPON;
    if (strcmp(str, "off-hand") == 0) return ENTITY_TYPE_WEAPON;
    if (strcmp(str, "enemy") == 0) return ENTITY_TYPE_ENEMY;
    if (strcmp(str, "tile") == 0) return ENTITY_TYPE_TILE;
    return ENTITY_TYPE_UNKNOWN;
}

//! NOTE: Text should be on permanent storage
static void add_text_to_render(game_t *game, const char *text, font_t *font, vec2f_t p)
{

}

static uint32_t load_animations_from_json(game_t             *game, 
                                          cJSON              *node, 
                                          animation_chunk_t **animation_chunks, 
                                          uint32_t            animation_chunk_capacity, 
                                          vec2f_t             entity_size)
{
    uint32_t chunk_count = 0;
    cJSON *child = NULL;

    cJSON_ArrayForEach(child, node)
    {
        animation_chunk_t *current_chunk = animation_chunks[chunk_count];
        if (!current_chunk)
        {
            current_chunk = bulk_data_push_struct(&game->animation_chunks);
            current_chunk->count = 0;
            animation_chunks[chunk_count++] = current_chunk;
        }
        else if (current_chunk->count == MAX_ANIMATIONS_PER_CHUNK)
        {
            current_chunk = bulk_data_push_struct(&game->animation_chunks);
            current_chunk->count = 0;
            animation_chunks[chunk_count++] = current_chunk;
        }

        animation_t *animation = &current_chunk->animations[current_chunk->count++];

        assert(chunk_count <= animation_chunk_capacity && "Unable to add more animations");
        const char *state_str = cJSON_GetStringValue(cJSON_GetObjectItem(child, "state"));
        animation->state = get_animation_state_from_string(state_str);

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
    return chunk_count;
}

static void load_entities_from_json(game_t *game, cJSON *root)
{
    //! NOTE: Load Entities
    cJSON *entities_node = cJSON_GetObjectItem(root, "entities");
    if (!entities_node) 
    {
        LOGE("NOOOOOOOO");
        return;
    }

    cJSON *entity_node = NULL;
    cJSON_ArrayForEach(entity_node, entities_node)
    {
        entity_t *entity = bulk_data_push_struct(&game->entities);
        entity->id = game->entity_id++;
        const char *entity_type = cJSON_GetStringValue(cJSON_GetObjectItem(entity_node, "type"));
        entity->type = entity_type_from_string(entity_type);
        entity->data = NULL;

        cJSON *node = NULL;
        node = cJSON_GetObjectItem(entity_node, "position");
        entity->p.x = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "x"));
        entity->p.y = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "y"));

        node = cJSON_GetObjectItem(entity_node, "size");
        entity->size.x = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "x"));
        entity->size.y = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "y"));

        entity->z_index = (int32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(entity_node, "z_index"));
        
        switch (entity->type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                game->player_entity = entity;
                player_t *player = &game->player_data;
                player->entity = entity;
                entity->data   = player;
                player->velocity = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(entity_node, "velocity"));
                player->dp.x = 0;
                player->dp.y = 0;
                entity->flags = ENTITY_CAN_COLLIDE;

                cJSON *anim_node = cJSON_GetObjectItem(entity_node, "animations");
                player->animation_chunk_count = load_animations_from_json(game, 
                                                                          anim_node, 
                                                                          player->animation_chunks,  
                                                                          MAX_ANIMATION_CHUNKS_PER_ENTITY, 
                                                                          entity->size);
                player->current_animation = &player->animation_chunks[0]->animations[0];
                break;
            }
            case ENTITY_TYPE_WEAPON:
            {
                weapon_t *weapon = bulk_data_push_struct(&game->weapons);
                entity->data = weapon;
                entity->flags = 0x0;

                weapon->entity = entity;  
                cJSON *sprite_node = cJSON_GetObjectItem(entity_node, "sprite");
                const char *texture_id = cJSON_GetStringValue(cJSON_GetObjectItem(sprite_node, "texture"));
                
                cJSON *offset_node = cJSON_GetObjectItem(sprite_node, "offset");
                float px = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(offset_node, "x"));
                float py = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(offset_node, "y"));

                cJSON *size_node = cJSON_GetObjectItem(sprite_node, "size");
                float sx = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(size_node, "x"));
                float sy = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(size_node, "y"));

                weapon->sprite.texture  = asset_store_get_texture(&game->asset_store, texture_id);
                weapon->sprite.src_rect.min.x = px;
                weapon->sprite.src_rect.min.y = py;
                weapon->sprite.src_rect.size.x = sx;
                weapon->sprite.src_rect.size.y = sy;

                if (strcmp(entity_type, "weapon") == 0)
                {
                    weapon->slot = WEAPON_SLOT_MAIN_HAND;
                }
                else if (strcmp(entity_type, "off-hand") == 0)
                {
                    weapon->slot =  WEAPON_SLOT_OFF_HAND;
                }
                break;
            }
            default:
            {
                LOGI("Unknown entity type!");
                assert(false);
                break;
            }
        }
    }
}

static void load_tiles_from_json(game_t *game, cJSON *root)
{
    cJSON *tilemap_node = cJSON_GetObjectItem(root, "tilemap");
    if (!tilemap_node)
    {
        LOGE("NOOOOOOOOOOOOOO");
        return;
    }

    const char *map_file_path = cJSON_GetStringValue(cJSON_GetObjectItem(tilemap_node, "map_file"));
    if (!map_file_path)
    {
        LOGE("NOOOOO");
        return;
    }

    FILE *map_file = fopen(map_file_path, "r");
    if (!map_file)
    {
        LOGE("NOOOOOOOOOO");
        return;
    }

    uint32_t rows  = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(tilemap_node, "row_count"));
    uint32_t cols  = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(tilemap_node, "col_count"));
    uint32_t tile_size = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(tilemap_node, "tile_size"));
    
    const char *texture_path = cJSON_GetStringValue(cJSON_GetObjectItem(tilemap_node, "texture"));

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
                return;
            }
            
            //create tile entity
            entity_t *tile_entity = bulk_data_push_struct(&game->entities);
            tile_entity->id = game->entity_id++;
            tile_entity->type = ENTITY_TYPE_TILE;
            tile_entity->p = (vec2f_t){(float)col * TILE_WIDTH, (float)row * TILE_HEIGHT};
            tile_entity->size = (vec2f_t){(float)TILE_WIDTH, (float)TILE_HEIGHT};
            tile_entity->z_index = 0;

            tile_t *tile_data = bulk_data_push_struct(&game->tiles);
            tile_data->entity = tile_entity;
            tile_data->sprite.texture = asset_store_get_texture(&game->asset_store, texture_path);
            assert(tile_data->sprite.texture);
            tile_data->sprite.src_rect = (rect_t){{(float)tile_col * tile_size, (float)tile_row * tile_size},{tile_size,tile_size}};
            tile_entity->data = tile_data;

            int collision = fgetc(map_file) - '0';
            if (collision == 1)
            {
                tile_entity->flags = 0x1;
            }
            else
            {
                tile_entity->flags = 0x0;
            }

            fgetc(map_file);
        }
        fgetc(map_file);
    }

    //create tile entity
    entity_t *tile_entity = bulk_data_push_struct(&game->entities);
    tile_entity->id = game->entity_id++;
    tile_entity->type = ENTITY_TYPE_TILE;
    tile_entity->p = (vec2f_t){512,512};
    tile_entity->size = (vec2f_t){13, 24};
    tile_entity->z_index = 5;

    tile_t *tile_data = bulk_data_push_struct(&game->tiles);
    tile_data->entity = tile_entity;
    tile_data->sprite.texture = asset_store_get_texture(&game->asset_store, "liberation-mono-regular-24");
    assert(tile_data->sprite.texture);
    tile_data->sprite.src_rect = (rect_t){{416,0},{13,24}};
    tile_entity->data = tile_data;
}

static char *read_whole_file(const char *file_path)
{
    FILE *file = fopen(file_path, "r");
    if (!file)
    {
        LOGE("Unable to load %s\n", file_path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_buf = memory_arena_push_array(scratch_allocator(), char, file_size + 1);
    assert(file_buf);

    fread(file_buf, 1, file_size, file);
    file_buf[file_size] = '\0';
    fclose(file);

    return file_buf;
}

static void load_assets_from_json(game_t *game, cJSON *root)
{
    cJSON *assets = cJSON_GetObjectItem(root, "assets");
    if (assets)
    {
        cJSON *file_paths = cJSON_GetObjectItem(assets, "textures");
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
            LOGE("Unable to parse texture file paths");
            return;
        }

        uint32_t glyph_count = 0;
        
        cJSON *fonts = cJSON_GetObjectItem(assets, "fonts");
        if (fonts)
        {
            cJSON *font = NULL;
            cJSON_ArrayForEach(font, fonts)
            {
                float width = cJSON_GetNumberValue(cJSON_GetObjectItem(font, "width"));
                float height = cJSON_GetNumberValue(cJSON_GetObjectItem(font, "height"));

                const char *file_name = cJSON_GetStringValue(cJSON_GetObjectItem(font, "vertices"));
                char *buf = read_whole_file(file_name);
                char *tok = strtok(buf, ",");
                while(tok)
                {
                    float x = strtof(tok, NULL);
                    tok = strtok(NULL, ",");
                    float y = strtof(tok, NULL);
                    tok = strtok(NULL, ",");
                    float dx = strtof(tok, NULL);
                    tok = strtok(NULL, ",");
                    float dy = strtof(tok, NULL);
                    tok = strtok(NULL, ",");

                    rect_t *glyph = &game->font.glyphs[glyph_count++];
                    glyph->min.x  = x;
                    glyph->min.y  = y;
                    glyph->size.x = dx;
                    glyph->size.y = dy; 
                }
            }
        }
    }
    else
    {
        LOGE("Unable to parse assets node");
        return;
    }
}

static void load_level(game_t *game, const char *data_file_path)
{
    char *file_buf = read_whole_file(data_file_path);
    cJSON *root = cJSON_Parse(file_buf);
    if (!root)
    {
        LOGE("Unable to parse json file");
        return;
    }

    load_assets_from_json(game, root);
    load_entities_from_json(game, root);
    load_tiles_from_json(game, root);

    LOGI("Success");

    cJSON_Delete(root);
}

static void setup(game_t *game)
{
    load_level(game, "./assets/data/level1.json");

    game->performance_freq = SDL_GetPerformanceFrequency();
    game->previous_counter = SDL_GetPerformanceCounter();

    game->camera.size.x = WINDOW_WIDTH + TILE_WIDTH;
    game->camera.size.y = WINDOW_HEIGHT + TILE_HEIGHT;
    game->camera.min.x  = game->player_entity->p.x + game->player_entity->size.x / 2 - WINDOW_WIDTH / 2;
    game->camera.min.y  = game->player_entity->p.y + game->player_entity->size.y / 2 - WINDOW_HEIGHT / 2;

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
        //reset all text
        game->visible_text_count = 0;

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
                        update_player(e, &game->input, DELTA_TIME, &game->entities);
                        break;
                    }
                    case(ENTITY_TYPE_WEAPON):
                    {
                        e->p = game->player_entity->p;
                        break;
                    }
                    default: 
                        break;
                }
            }
        }

        //add some text to render

        //update camera location
        game->camera.min.x = game->player_entity->p.x + game->player_entity->size.x / 2 - game->camera.size.x / 2;
        game->camera.min.y = game->player_entity->p.y + game->player_entity->size.y / 2 - game->camera.size.y / 2;

        game->accumulator -= 1.0 / 59.0;
        if (game->accumulator < 0) game->accumulator = 0.0;
    }
}


static void render(game_t *game)
{
    vulkan_renderer_render(&game->renderer, game);
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
    vulkan_renderer_init(&game->renderer, game->window, TILE_WIDTH, TILE_HEIGHT);

    game->entity_id = 0;

    bulk_data_init(&game->entities, entity_t);
    bulk_data_init(&game->tiles, tile_t);
    bulk_data_init(&game->weapons, weapon_t);
    bulk_data_init(&game->animation_chunks, animation_chunk_t);
    bulk_data_init(&game->sprites, sprite_t);

    game->is_running = true;
}

