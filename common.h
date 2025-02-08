#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <SDL2/SDL.h>

typedef unsigned char   u1;
typedef unsigned short  u2;
typedef unsigned int    u4;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define ArraySize(array) (sizeof(array) / sizeof((array)[0]))



#endif
