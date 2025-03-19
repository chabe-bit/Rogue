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

#define WINDOW_WIDTH    480
#define WINDOW_HEIGHT   464

#define ASPECT_WIDTH    256
#define ASPECT_HEIGHT   244

#define SCREEN_CENTER_X (ASPECT_WIDTH  / 2)
#define SCREEN_CENTER_Y (ASPECT_HEIGHT / 2)

#define GLYPH_WIDTH     6
#define GLYPH_HEIGHT    5
#define NUM_GLYPHS ArraySize(GUIFontData)

#define GET_TEXT_CENTER_W(text) (GLYPH_WIDTH * strlen(text) / 2)
#define GET_TEXT_CENTER_H(text) (GLYPH_HEIGHT * strlen(text) / 2)
#define SET_TEXT_CENTER_X(text, offset) ( (SCREEN_CENTER_X - GET_TEXT_CENTER_W(text)) + offset )

// Only necessecary if rendering text vertically and text needs to be centered, otherwise text will start its render from the literal center of the screen at the Y axis down. 
#define SET_TEXT_CENTER_Y(text, offset) ( (SCREEN_CENTER_Y - GET_TEXT_CENTER_H(text)) + offset ) 

#define TILE_WIDTH          16
#define TILE_HEIGHT         24
#define SUB_TILE_WIDTH      10
#define SUB_TILE_HEIGHT     18

#define SET_TILE_SPACE_POS_Y(x) (TILE_WIDTH  * x)
#define SET_TILE_SPACE_POS_X(y) (TILE_HEIGHT * y)

typedef struct
{
    i32 x, y;
} vec2_t;

typedef struct
{
    u8 r, g, b, a;
} rgba_t;

// temp
typedef struct option_t
{
    char *text;
    i32 x, y;
} option_t;

#endif
