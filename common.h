#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <SDL2/SDL.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#define ArraySize(array) (sizeof(array) / sizeof((array)[0]))

#define WINDOW_WIDTH  480
#define WINDOW_HEIGHT 464

#define ASPECT_WIDTH  256
#define ASPECT_HEIGHT 240

#define SCREEN_CENTER_X (ASPECT_WIDTH  / 2)
#define SCREEN_CENTER_Y (ASPECT_HEIGHT / 2)

#define GLYPH_WIDTH  6
#define GLYPH_HEIGHT 5
#define NUM_GLYPHS ArraySize(GUIFontData)

#define CENTER_TEXT_X(text, offset) ( SCREEN_CENTER_X - ((GLYPH_WIDTH * strlen(text)) / 2) + offset )


typedef struct
{
    i32 x, y;
} vec2_t;

typedef struct
{
    u8 r, g, b, a;
} rgba_t;

// temp
typedef struct menu_item_t
{
    char *text;
    i32 x, y;
} menu_item_t;

#endif
