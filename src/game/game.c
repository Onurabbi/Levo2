#include "game.h"
#include "../logger/logger.h"
#include "collision.h"

#include "../memory/memory_arena.h"

#include <SDL2/SDL.h>
#include <assert.h>

static void process_input(game_t *game)
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_QUIT:
                game->is_running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    game->is_running = false;
                }
                break;
            default:
                break;
        }
    }

    game->keyboard_state = SDL_GetKeyboardState(NULL);
}

static void setup(game_t *game)
{

    LOGI("Setup complete");
}

#define DELTA_TIME 1.0/60.0

static int compare_collisions(const void *a, const void *b)
{
    uint_float_pair *pair1 = (uint_float_pair*)a;
    uint_float_pair *pair2 = (uint_float_pair*)b;

    if (pair1->f < pair2->f)return -1;
    if (pair1->f == pair2->f) return 0;
    else return 1;
}

static void update(game_t *game)
{
    memory_arena_begin(scratch_allocator());

    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t elapsed = now - game->previous_counter;
    game->previous_counter = now;
    
    double sec = (double)elapsed / game->performance_freq;
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
                        vec2f_t dp = {0.0f, 0.0f};
                        if (game->keyboard_state[SDL_SCANCODE_W])
                        {
                            dp.y = -100.00f;
                        }
                        if (game->keyboard_state[SDL_SCANCODE_S])
                        {
                            dp.y = 100.0f;
                        }
                        if (game->keyboard_state[SDL_SCANCODE_A])
                        {
                            dp.x = -100.0f;
                        }
                        if (game->keyboard_state[SDL_SCANCODE_D])
                        {
                            dp.x = 100.0f;
                        }



                        dp.x *= DELTA_TIME;
                        dp.y *= DELTA_TIME;


                        //first pass
                        uint_float_pair pairs[4];
                        entity_t *colliding_entities[4];
                        uint32_t  colliding_entity_count = 0;

                        for (uint32_t j = i; j < bulk_data_size(&game->entities); j++)
                        {
                            if (colliding_entity_count == 4) break;
                            if (j == i) continue;

                            entity_t *other = bulk_data_getp_null(&game->entities, j);
                            if (other)
                            {
                                vec2f_t contact_normal, contact_position;
                                float time;
                                if (resolve_dyn_rect_vs_rect(e->rect, other->rect, dp, &contact_position, &contact_normal, &time))
                                {
                                    uint_float_pair *pair = &pairs[colliding_entity_count];
                                    pair->i = colliding_entity_count;
                                    pair->f = time;
                                    colliding_entities[colliding_entity_count++] = other;
                                }
                            }
                        }

                        //sort collisions in ascending time order
                        qsort(pairs, colliding_entity_count, sizeof(pairs[0]), compare_collisions);

                        //second pass to resolve collisions
                        for (uint32_t j = 0; j < colliding_entity_count; j++)
                        {
                            uint32_t entity_index = pairs[j].i;
                            rect_t r_st = colliding_entities[entity_index]->rect;
                            vec2f_t contact_normal, contact_position;
                            float time;
                            if (resolve_dyn_rect_vs_rect(e->rect, r_st, dp, &contact_position, &contact_normal, &time))
                            {
                                dp.x += contact_normal.x * fabs(dp.x) * (1 - time);
                                dp.y += contact_normal.y * fabs(dp.y) * (1 - time); 
                            }
                        }

                        e->p.x += dp.x;
                        e->p.y +=  dp.y;

                        break;
                    }
                    default: 
                        break;
                }
            }
        }

        game->accumulator -= 1.0 / 59.0;
        if (game->accumulator < 0) game->accumulator = 0.0;
    }
}


static void render(game_t *game)
{
    vulkan_renderer_begin(&game->renderer);
    vulkan_renderer_render_entities(&game->renderer, &game->entities);
    vulkan_renderer_present(&game->renderer);
}

void game_run(game_t *game)
{
    setup(game);
    while(game->is_running)
    {
        process_input(game);
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
        1280,
        720,
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

    entity_t *player = bulk_data_push_struct(&game->entities);
    player->id   = game->entity_id++;
    player->type = ENTITY_TYPE_PLAYER;
    player->p    = (vec2f_t){0.0f};
    player->size = (vec2f_t){64.0f,64.0f};
    player->dp   = (vec2f_t){0.0f};
    player->data = &game->player_data;

    game->player_entity = player;

    game->player_data.entity       = game->player_entity;
    game->player_data.sprite_count = 0;
    game->player_data.sprites[0].texture = asset_store_add_texture(&game->asset_store, &game->renderer, "statue", "./assets/textures/statue.jpg");
    game->player_data.sprites[0].src_rect = (rect_t){{0, 0}, {960, 720}};
    
    entity_t *tile = bulk_data_push_struct(&game->entities);
    tile->id   = game->entity_id++;
    tile->type = ENTITY_TYPE_TILE;
    tile->p    = (vec2f_t){128.0f,128.0f};
    tile->size = (vec2f_t){64.0f, 64.0f};
    tile->dp   = (vec2f_t){0.0f};
    tile_t *tile_data = bulk_data_push_struct(&game->tiles);
    tile_data->entity = tile;
    tile_data->sprite.texture = asset_store_get_texture(&game->asset_store, "statue");
    tile_data->sprite.src_rect = (rect_t){{0,0},{960,720}};
    tile->data = tile_data;

    tile = bulk_data_push_struct(&game->entities);
    tile->id   = game->entity_id++;
    tile->type = ENTITY_TYPE_TILE;
    tile->p    = (vec2f_t){192.0f,128.0f};
    tile->size = (vec2f_t){64.0f, 64.0f};
    tile->dp   = (vec2f_t){0.0f};
    tile_data = bulk_data_push_struct(&game->tiles);
    tile_data->entity = tile;
    tile_data->sprite.texture = asset_store_get_texture(&game->asset_store, "statue");
    tile_data->sprite.src_rect = (rect_t){{0,0},{960,720}};
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
    tile->data = tile_data;

    tile = bulk_data_push_struct(&game->entities);
    tile->id   = game->entity_id++;
    tile->type = ENTITY_TYPE_TILE;
    tile->p    = (vec2f_t){320.0f,128.0f};
    tile->size = (vec2f_t){64.0f, 64.0f};
    tile->dp   = (vec2f_t){0.0f};
    tile_data = bulk_data_push_struct(&game->tiles);
    tile_data->entity = tile;
    tile_data->sprite.texture = asset_store_get_texture(&game->asset_store, "statue");
    tile_data->sprite.src_rect = (rect_t){{0,0},{960,720}};
    tile->data = tile_data;

    game->performance_freq = SDL_GetPerformanceFrequency();
    game->previous_counter = SDL_GetPerformanceCounter();
    game->is_running = true;
}

