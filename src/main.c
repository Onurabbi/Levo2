#include <stdio.h>

#include "game.c"

int main(int argc, char *argv[])
{
    game_t game = {0};
    game_init(&game);
    game_run(&game);
    return 0;
}
