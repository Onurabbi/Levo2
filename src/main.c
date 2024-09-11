#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include <stb/stb_ds.h>

#include "core/containers/containers.h"
#include "logger/logger.h"
#include "renderer/vulkan_texture.h"
#include "renderer/vulkan_renderer.h"
#include "asset_store/asset_store.h"
#include "game/game.h"
#include "time.h"

int main(int argc, char *argv[])
{
    game_t game = {0};
    game_init(&game);
    game_run(&game);
    return 0;
}
