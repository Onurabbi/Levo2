#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include <stb/stb_ds.h>

#include "game.h"

int main(int argc, char *argv[])
{
    game_t game = {0};
    game_init(&game);
    game_run(&game);
    return 0;
}
