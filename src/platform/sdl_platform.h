#ifndef SDL_PLATFORM_H
#define SDL_PLATFORM_H

#include "common.h"

typedef struct
{
    SDL_Window *Window;
    SDL_Renderer *Renderer;
    SDL_Texture *Texture;
    SDL_Event e;
} window_t;

extern window_t SDLWindow;

#endif /* SDL_PLATFORM */