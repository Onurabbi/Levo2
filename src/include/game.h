#ifndef GAME_H_
#define GAME_H_

#include <game_types.h>
#include <bulk_data_types.h>

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

void game_init(game_t *game);
void game_run(game_t *game);
#endif

