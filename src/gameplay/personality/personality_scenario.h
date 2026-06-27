#ifndef PERSONALITY_SCENARIO_H
#define PERSONALITY_SCENARIO_H

#include "common.h"

#define VILLAGE_INDEX   0
#define MONSTER_INDEX   1 
#define FOREST_INDEX    2
#define CAVE_INDEX      3
#define DESERT_INDEX    4
#define TOWER_INDEX     5
#define THEATER_INDEX   6
#define CASTLE_INDEX    7

#define VILLAGE_OPTION_COUNT    3
#define MONSTER_OPTION_COUNT    5
#define CAVE_OPTION_COUNT       5
#define DESERT_OPTION_COUNT     3
#define TOWER_OPTION_COUNT      2
#define THEATER_OPTION_COUNT    4

#define VILLAGE_OPTION_1 "Steal the coins openly with pride."
#define VILLAGE_OPTION_2 "Steal the coins sneakily."
#define VILLAGE_OPTION_3 "Don't steal the coins and return them."

#define MONSTER_OPTION_1 "Kill fewer than three people.\nincluding women and the elderly,\nbut don't kill the children."
#define MONSTER_OPTION_2 "Kill three or more people,\nbut don't kill the women,\nthe elderly, or children."
#define MONSTER_OPTION_3 "Kill three or more poeple,\nincluding children."
#define MONSTER_OPTION_4 "Kill nine or more people, but\ndon't kill the man by the inn."

#define CAVE_OPTION_1 "Save the princess."
#define CAVE_OPTION_2 "Take the door to go deeper"
#define CAVE_OPTION_3 "Take the door to the room\nof treasures then go deeper."
#define CAVE_OPTION_4 "Take the door to the room\nof treasures then leave."
#define CAVE_OPTION_5 "Ignore everything and leave."

#define DESERT_OPTION_1 "Finish the canteen and leave."
#define DESERT_OPTION_2 "Give the man the canteen\nand head to town."
#define DESERT_OPTION_3 "Carry the man to town."

#define TOWER_OPTION_1 "Take the stairs."
#define TOWER_OPTION_2 "Jump off the tower."

#define THEATER_OPTION_1 "Ignore the man and leave." 
#define THEATER_OPTION_2 "Say yes"
#define THEATER_OPTION_3 "Say no" 
#define THEATER_OPTION_4 "Play dumb and tell him\nhe got the wrong person." 

typedef enum
{
    SCENARIO_NONE       = -1,
    SCENARIO_VILLAGE    = (1 << 0),
    SCENARIO_MONSTER    = (1 << 1),
    SCENARIO_FOREST     = (1 << 2),
    SCENARIO_CAVE       = (1 << 3),
    SCENARIO_DESERT     = (1 << 4),
    SCENARIO_TOWER      = (1 << 5),
    SCENARIO_THEATER    = (1 << 6),
    SCENARIO_CASTLE     = (1 << 7),
} scenario_type;

typedef struct
{
    i32 index; 
    bool is_active;
    SDL_Rect box;
    option_t options[5];
} scenario_t;

typedef struct
{
    bool load_results;
    u8 result; 
    char personality[32];
    scenario_t scenario[8];
} personality_scenario_t; 

extern const char village_scenario[];
extern const char desert_scenario[];
extern const char monster_scenario[];
extern const char tower_scenario[];
extern const char cave_scenario[];
extern const char theater_scenario[];

extern option_t village_scenario_options[];
extern option_t monster_scenario_options[];
extern option_t cave_scenario_options[];
extern option_t desert_scenario_options[];
extern option_t tower_scenario_options[];
extern option_t theater_scenario_options[];

#endif 
