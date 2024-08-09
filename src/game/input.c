#include "input.h"
#include <SDL2/SDL.h>

void process_input(input_t *input)
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_QUIT:
                input->quit = true;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    input->quit = true;
                }
                break;
            default:
                break;
        }
    }

    input->keyboard_state = SDL_GetKeyboardState(NULL);
}

