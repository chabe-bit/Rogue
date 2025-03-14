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

extern const char *village_scenario[];
extern const char *desert_scenario[];
extern const char *monster_scenario[];
extern const char *tower_scenario[];
extern const char *cave_scenario[];
extern const char *theater_scenario[];

#endif 
