#ifndef COLORS_H
#define COLORS_H

#include "common.h"

#define MAX_COLOR       6

#define COLOR_RED       0
#define COLOR_GREEN     1
#define COLOR_BLUE      2
#define COLOR_WHITE     3
#define COLOR_BLACK     4
#define COLOR_ORANGE    5

typedef struct
{
    SDL_Color red;
    SDL_Color green;
    SDL_Color blue;
    SDL_Color white;
    SDL_Color black;
    SDL_Color orange;
} color_t;

extern color_t color;

#endif 
