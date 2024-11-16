#ifndef PLATFORM_H_
#define PLATFORM_H_

struct SDL_Window;

typedef struct 
{
    struct SDL_Window *window;
    int width, height;
} window_t;

#endif


