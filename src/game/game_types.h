#ifndef GAME_TYPES_H_
#define GAME_TYPES_H_

#include <stdint.h>
#include <stddef.h>

#include "../containers/container_types.h"
#include "../asset_store/asset_store.h"

#define MAX_SPRITES_PER_ANIMATION 8
#define MAX_ANIMATIONS_PER_ENTITY 8

typedef enum
{
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_TILE,
    ENTITY_TYPE_ENEMY,
}entity_type_t;

typedef enum
{
    ENTITY_STATE_IDLE,
    ENTITY_STATE_RUN,
    ENTITY_STATE_MELEE,
    ENTITY_STATE_RANGED,
    ENTITY_STATE_HIT,
    ENTITY_STATE_DEAD,
}entity_state_t;

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
    vec2f_t        dp;
    int32_t        z_index;
    entity_state_t state;
    entity_type_t  type;
    void           *data;
}entity_t;

typedef struct
{
    vulkan_texture_t *texture;
    rect_t            src_rect;
}sprite_t;

typedef struct
{
    vulkan_texture_t *texture;
    rect_t            sprites[MAX_SPRITES_PER_ANIMATION];
    uint32_t          sprite_count;
    float             duration;
}animation_t;

typedef struct 
{
    entity_t *entity;
    float     velocity;
    
    //animation playback
    animation_t *animations[MAX_ANIMATIONS_PER_ENTITY];
    uint32_t    current_animation;
    uint32_t    animation_count;
    float       anim_timer;
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
BulkDataTypes(animation_t)
BulkDataTypes(sprite_t)

struct SDL_Window;

typedef struct 
{
    const uint8_t *keyboard_state;
    int32_t        mouse_x,mouse_y;
    bool           quit;
}input_t;

typedef struct 
{
    struct SDL_Window *window;
    asset_store_t      asset_store;
    vulkan_renderer_t  renderer;
    
    bulk_data_entity_t entities;
    uint64_t           entity_id;

    //one player per game
    player_t           player_data;
    entity_t          *player_entity;

    //entity specific data (other than player)
    bulk_data_tile_t   tiles;

    //timing
    uint64_t          previous_counter;
    uint64_t          performance_freq;
    double            accumulator;

    //resources
    bulk_data_animation_t animations;
    bulk_data_sprite_t    sprites;

    input_t input;
    rect_t  camera;

    bool    is_running;
}game_t;


#endif

