#include "game.h"
#include "input.h"


#include "collision.h"
#include "entity.h"
#include "player.h"
#include "animation.h"
#include "widget.h"
#include "generator.h"

#include "../memory/memory.h"
#include "../logger/logger.h"
#include "../cJSON/cJSON.h"
#include "../containers/string.h"
#include "../math/math_utils.h"
#include "../utils/utils.h"
#include "../utils/json_loader.h"

#include <time.h>
#include <SDL2/SDL.h>
#include <assert.h>

#define DELTA_TIME 1.0/60.0
#define TILE_COUNT_X 30

static void setup(game_t *game)
{
    load_level_from_json(game, "./assets/data/level1.json");
    //create fps entity
#if 1
    entity_t *fps_entity = bulk_data_push_struct(&game->entities);
    fps_entity->id   = game->entity_id++;
    fps_entity->type = ENTITY_TYPE_WIDGET;
    fps_entity->p    = (vec2f_t){25, game->window_height - 50};
    fps_entity->size = (vec2f_t){0,0};//does not matter
    fps_entity->z_index = 5;

    widget_t *widget   = bulk_data_push_struct(&game->widgets);
    widget->entity     = fps_entity; 
    fps_entity->data   = widget;
    widget->text_label = bulk_data_push_struct(&game->text_labels);
    widget->text_label->font = &game->font;
    widget->text_label->offset = (vec2f_t){250,-250};
    widget->text_label->text = NULL;
    widget->text_label->duration = 1.0f;
    widget->text_label->timer    = 0.0f;
    widget->type = WIDGET_TYPE_FPS;
#endif

    game->performance_freq = SDL_GetPerformanceFrequency();
    game->previous_counter = SDL_GetPerformanceCounter();

    game->camera.size.x = game->window_width + game->tile_width;
    game->camera.size.y = game->window_height + game->tile_width;
    game->camera.min.x  = game->player_entity->p.x + game->player_entity->size.x / 2 - game->window_width / 2;
    game->camera.min.y  = game->player_entity->p.y + game->player_entity->size.y / 2 - game->window_height / 2;
}

static void update(game_t *game)
{
    //if quit was requested, quit now
    if (game->input.quit)
    {
        game->is_running = false;
        return;
    }

    memory_begin(MEM_TAG_RENDER);

    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t elapsed = now - game->previous_counter;
    game->previous_counter = now;
    
    double sec = (double)elapsed / game->performance_freq;
    game->accumulator += sec;

    while (game->accumulator > 1.0 / 61.0)
    {
        memory_begin(MEM_TAG_SIM);

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
                    case(ENTITY_TYPE_WIDGET):
                    {
                        update_widget(e, 1.0/60.0, sec);
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
    memory_uninit();
}

void game_init(game_t *game)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        LOGE("initializing SDL %s", SDL_GetError());
        return;
    }

#if defined (DEBUG)
    game->window_width = 1920;
    game->window_height = 1080;
    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_BORDERLESS;
#else
    SDL_DisplayMode dm = {};
    SDL_GetCurrentDisplayMode(0, &dm);
    game->window_width = dm.w;
    game->window_height = dm.h;
    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN;
#endif

    game->window = SDL_CreateWindow(NULL,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        game->window_width,
        game->window_height,
        flags
    );

    assert(game->window);

    game->is_running = false;
    game->tile_width = (float)game->window_width / (float)TILE_COUNT_X;

    srand(time(NULL));
    memory_init();
    create_dungeon();
    asset_store_init(&game->asset_store);
    vulkan_renderer_init(&game->renderer, game->window, game->tile_width, game->tile_width);

    game->entity_id = 0;

    bulk_data_init(&game->entities, entity_t);
    bulk_data_init(&game->tiles, tile_t);
    bulk_data_init(&game->weapons, weapon_t);
    bulk_data_init(&game->animation_chunks, animation_chunk_t);
    bulk_data_init(&game->sprites, sprite_t);
    bulk_data_init(&game->widgets, widget_t);
    bulk_data_init(&game->text_labels, text_label_t);

    game->is_running = true;
}

