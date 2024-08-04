#ifndef GAME_TYPES_H_
#define GAME_TYPES_H_

#include <stdint.h>
#include <stddef.h>

#include "../containers/container_types.h"
#include "../asset_store/asset_store.h"

#define MAX_SPRITES_PER_ENTITY 4

typedef enum
{
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_TILE,
    ENTITY_TYPE_ENEMY,
}entity_type;

typedef struct
{
    uint64_t     id;
    union {
        rect_t rect;
        struct{
            vec2f_t p;
            vec2f_t size;
        };
    };
    vec2f_t      dp;
    entity_type  type;
    void        *data;
}entity_t;

typedef struct 
{
    entity_t *entity;
    sprite_t  sprites[MAX_SPRITES_PER_ENTITY];
    uint32_t  sprite_count;
}player_t;

typedef struct
{
    entity_t *entity;
    sprite_t sprite;
}tile_t;

typedef struct
{
    uint32_t i;
    float    f;
}uint_float_pair;

BulkDataTypes(entity_t)
BulkDataTypes(tile_t)

struct SDL_Window;

typedef struct 
{
    struct SDL_Window *window;
    asset_store_t      asset_store;
    vulkan_renderer_t  renderer;
    
    bulk_data_entity_t entities;
    uint64_t           entity_id;

    bulk_data_tile_t   tiles;

    //one player per game
    player_t           player_data;
    entity_t          *player_entity;

    uint64_t          previous_counter;
    uint64_t          performance_freq;
    double            accumulator;
    //input
    const uint8_t     *keyboard_state;
    int32_t            mouse_x, mouse_y;
    bool               is_running;
}game_t;

#endif

