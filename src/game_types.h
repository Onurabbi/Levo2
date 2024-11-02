#ifndef GAME_TYPES_H_
#define GAME_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "core/math/math_types.h"
#include "core/renderer/vulkan_backend/vulkan_types.h"
#include  "core/asset_store/asset_types.h"
#include "core/asset_store/asset_store.h"

#define MAX_VISIBLE_TEXT_PER_SCENE         32

typedef struct 
{
    entity_t     *entity;
    text_label_t *name;

    float         velocity;
    vec2f_t       dp;

    uint32_t      animation_chunk_count;
    float         anim_timer;

    entity_t     *weapon;
} player_t;

typedef struct
{
    uint32_t i;
    float    f;
} uint_float_pair;

struct SDL_Window;

typedef struct 
{
    const uint8_t *keyboard_state;
    int32_t        mouse_x,mouse_y;
    bool           quit;
} input_t;

typedef struct
{
    bulk_data_entity_t        entities;
    bulk_data_tile_t          tiles;
    bulk_data_weapon_t        weapons;
    bulk_data_widget_t        widgets;
    bulk_data_texture_t       textures;
    bulk_data_skinned_model_t skinned_models;
    bulk_data_renderbuffer_t  renderbuffers; 
}bulk_data_t;

typedef struct 
{
    struct SDL_Window *window;
    
    int32_t            window_width;
    int32_t            window_height;

    asset_store_t      asset_store;
    renderer_t         renderer;
    bulk_data_t        bulk_data;
    
    //temporary
    uint32_t skinned_model_index;
    //one player per game
    player_t           player_data;
    entity_t          *player_entity;

    uint64_t           entity_id;

    //text rendering
    font_t font;
    input_t           input;
    //timing
    uint64_t          previous_counter;
    uint64_t          performance_freq;
    double            accumulator;
    bool              is_running;
} game_t;


#endif

