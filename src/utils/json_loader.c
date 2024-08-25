#include "../game/game.h"
#include "json_loader.h"
#include "../logger/logger.h"
#include "../containers/string.h"
#include "utils.h"
#include "../cJSON/cJSON.h"
#include <assert.h>

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


static entity_type_t 
entity_type_from_string(const char *str)
{
    if (strcmp(str, "player") == 0) return ENTITY_TYPE_PLAYER;
    if (strcmp(str, "weapon") == 0) return ENTITY_TYPE_WEAPON;
    if (strcmp(str, "off-hand") == 0) return ENTITY_TYPE_WEAPON;
    if (strcmp(str, "enemy") == 0) return ENTITY_TYPE_ENEMY;
    if (strcmp(str, "tile") == 0) return ENTITY_TYPE_TILE;
    if (strcmp(str, "widget") == 0) return ENTITY_TYPE_WIDGET;
    return ENTITY_TYPE_UNKNOWN;
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
            tile_entity->p = (vec2f_t){(float)col * game->tile_width, (float)row * game->tile_width};
            tile_entity->size = (vec2f_t){(float)game->tile_width, (float)game->tile_width};
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
}

void load_assets_from_json(game_t *game, cJSON *root)
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
                const char *texture_id = cJSON_GetStringValue(cJSON_GetObjectItem(font, "texture"));
                game->font.texture = asset_store_get_texture(&game->asset_store, texture_id);
                assert(game->font.texture);
                
                const char *file_name = cJSON_GetStringValue(cJSON_GetObjectItem(font, "vertices"));
                char *buf = read_whole_file(file_name, NULL);
                char *tok = strtok(buf, ",");
                while(tok)
                {
                    float x1 = strtof(tok, NULL);
                    tok = strtok(NULL, ",");
                    float x2 = strtof(tok, NULL);
                    tok = strtok(NULL, ",");
                    float y1 = strtof(tok, NULL);
                    tok = strtok(NULL, ",");
                    float y2 = strtof(tok, NULL);
                    tok = strtok(NULL, ",");

                    rect_t *glyph = &game->font.glyphs[glyph_count++];
                    glyph->min.x  = x1;
                    glyph->min.y  = y1;
                    glyph->size.x = x2 - x1;
                    glyph->size.y = y2 - y1; 
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

uint32_t load_animations_from_json(game_t             *game, 
                                   struct cJSON       *node, 
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

void load_player_from_json(entity_t *entity, player_t *player, cJSON *entity_node, game_t *game)
{
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
    const char *player_name = cJSON_GetStringValue(cJSON_GetObjectItem(entity_node, "name"));
    if (player_name)
    {
        //player has a name
        player->name = bulk_data_push_struct(&game->text_labels);
        player->name->text = string_duplicate(player_name);
        player->name->font = &game->font;
        player->name->timer = 0.0f;
        player->name->duration = 0.0f;
        player->name->offset = (vec2f_t){game->tile_width / 4,-game->tile_width / 4};
    }
    else
    {
        LOGE("Player has no name");
        assert(false);
    }
}

static void load_entity_from_json(entity_t *entity, cJSON *entity_node, game_t *game)
{
    entity->id = game->entity_id++;
    const char *entity_type = cJSON_GetStringValue(cJSON_GetObjectItem(entity_node, "type"));
    entity->type = entity_type_from_string(entity_type);
    entity->data = NULL;

    cJSON *node = NULL;
    node = cJSON_GetObjectItem(entity_node, "position");
    entity->p.x = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "x"));
    entity->p.y = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "y"));

    node = cJSON_GetObjectItem(entity_node, "size");
    entity->size.x = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "x")) * game->tile_width;
    entity->size.y = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(node, "y")) * game->tile_width;

    entity->z_index = (int32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(entity_node, "z_index"));

    switch (entity->type)
    {
        case ENTITY_TYPE_PLAYER:
        {
            load_player_from_json(entity, &game->player_data, entity_node, game);
            game->player_entity = entity;
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
            LOGE("Unknown entity type!");
            assert(false);
            break;
        }
    }
}

void load_entities_from_json(game_t *game, cJSON *root)
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
        load_entity_from_json(entity, entity_node, game);
    }
}

void load_level_from_json(game_t *game, const char *data_file_path)
{
    char *file_buf = read_whole_file(data_file_path, NULL);
    cJSON *root = cJSON_Parse(file_buf);
    if (!root)
    {
        LOGE("Unable to parse json file");
        return;
    }

    load_assets_from_json(game, root);
    load_entities_from_json(game, root);
    load_tiles_from_json(game, root);

    cJSON_Delete(root);
}

