#include "game.h"
#include "input.h"


#include "collision.h"
#include "entity.h"
#include "player.h"

#include "../logger/logger.h"
#include "../memory/memory_arena.h"

#include <SDL2/SDL.h>
#include <assert.h>

#define DELTA_TIME 1.0/60.0

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720


static void setup(game_t *game)
{
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

    entity_t *player = bulk_data_push_struct(&game->entities);
    player->id   = game->entity_id++;
    player->type = ENTITY_TYPE_PLAYER;
    player->p    = (vec2f_t){0.0f};
    player->size = (vec2f_t){64.0f, 64.0f};
    player->dp   = (vec2f_t){0.0f};
    player->data = &game->player_data;

    game->player_entity          = player;
    game->player_data.entity     = game->player_entity;
    game->player_data.velocity   = 100.0f;
    game->player_data.anim_timer = 0.0f;

    asset_store_add_texture(&game->asset_store, &game->renderer, "knight-run", "./assets/textures/pixel_crawler/Heroes/Knight/Run/Run-Sheet.png");
    asset_store_add_texture(&game->asset_store, &game->renderer, "knight-idle", "./assets/textures/pixel_crawler/Heroes/Knight/Idle/Idle-Sheet.png");
    asset_store_add_texture(&game->asset_store, &game->renderer, "statue", "./assets/textures/statue.jpg");

    game->player_data.current_animation = 0;
    game->player_data.anim_timer = 0.0f;

    animation_t *animation = bulk_data_push_struct(&game->animations);
    game->player_data.animations[0] = animation;
    animation->duration     = 1.0f;
    animation->sprite_count = 0;

    //now do the sprites
    for (uint32_t i = 0; i < 4; i++)
    {
        sprite_t *sprite = bulk_data_push_struct(&game->sprites);
        animation->sprites[animation->sprite_count++] = sprite;

        sprite->texture  = asset_store_get_texture(&game->asset_store, "knight-idle");
        sprite->src_rect = (rect_t){{i * 32,0},{32,32}};
        sprite->z_index  = 5;
    }

    entity_t *tile = bulk_data_push_struct(&game->entities);
    tile->id   = game->entity_id++;
    tile->type = ENTITY_TYPE_TILE;
    tile->p    = (vec2f_t){128.0f,128.0f};
    tile->size = (vec2f_t){64.0f, 64.0f};
    tile->dp   = (vec2f_t){0.0f};
    tile_t *tile_data = bulk_data_push_struct(&game->tiles);
    tile_data->entity = tile;
    tile_data->sprite.texture  = asset_store_get_texture(&game->asset_store, "statue");
    tile_data->sprite.src_rect = (rect_t){{0,0},{960,720}};
    tile_data->sprite.z_index  = 0;
    tile->data = tile_data;

    tile = bulk_data_push_struct(&game->entities);
    tile->id   = game->entity_id++;
    tile->type = ENTITY_TYPE_TILE;
    tile->p    = (vec2f_t){192.0f,128.0f};
    tile->size = (vec2f_t){64.0f, 64.0f};
    tile->dp   = (vec2f_t){0.0f};
    tile_data = bulk_data_push_struct(&game->tiles);
    tile_data->entity = tile;
    tile_data->sprite.texture  = asset_store_get_texture(&game->asset_store, "statue");
    tile_data->sprite.src_rect = (rect_t){{0,0},{960,720}};
    tile_data->sprite.z_index  = 0;
    tile->data = tile_data;

    tile = bulk_data_push_struct(&game->entities);
    tile->id   = game->entity_id++;
    tile->type = ENTITY_TYPE_TILE;
    tile->p    = (vec2f_t){256.0f,128.0f};
    tile->size = (vec2f_t){64.0f, 64.0f};
    tile->dp   = (vec2f_t){0.0f};
    tile_data = bulk_data_push_struct(&game->tiles);
    tile_data->entity = tile;
    tile_data->sprite.texture = asset_store_get_texture(&game->asset_store, "statue");
    tile_data->sprite.src_rect = (rect_t){{0,0},{960,720}};
    tile_data->sprite.z_index  = 0;
    tile->data = tile_data;

    tile = bulk_data_push_struct(&game->entities);
    tile->id   = game->entity_id++;
    tile->type = ENTITY_TYPE_TILE;
    tile->p    = (vec2f_t){320.0f,128.0f};
    tile->size = (vec2f_t){64.0f, 64.0f};
    tile->dp   = (vec2f_t){0.0f};
    tile_data = bulk_data_push_struct(&game->tiles);
    tile_data->entity = tile;
    tile_data->sprite.texture  = asset_store_get_texture(&game->asset_store, "statue");
    tile_data->sprite.src_rect = (rect_t){{0,0},{960,720}};
    tile_data->sprite.z_index  = 0;
    tile->data = tile_data;

    game->performance_freq = SDL_GetPerformanceFrequency();
    game->previous_counter = SDL_GetPerformanceCounter();

    game->camera.size.x = WINDOW_WIDTH;
    game->camera.size.y = WINDOW_HEIGHT;
    game->camera.min.x  = player->p.x - WINDOW_WIDTH / 2;
    game->camera.min.y  = player->p.y - WINDOW_HEIGHT / 2;

    game->is_running = true;
}

