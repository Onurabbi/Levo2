#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include <stb/stb_ds.h>

#include "containers/containers.h"
#include "logger/logger.h"
#include "renderer/vulkan_texture.h"
#include "renderer/vulkan_renderer.h"
#include "asset_store/asset_store.h"
#include "memory/memory_arena.h"
#include "game/game.h"

int main(int argc, char *argv[])
{
    game_t game = {0};
    game_init(&game);
    game_run(&game);

    return 0;
}
