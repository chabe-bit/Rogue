#ifndef PERSONALITY_H
#define PERSONALITY_H

#include "common.h"

#define VILLAGE_INDEX   0
#define MONSTER_INDEX   1 
#define FOREST_INDEX    2
#define CAVE_INDEX      3
#define DESERT_INDEX    4
#define TOWER_INDEX     5
#define THEATER_INDEX   6
#define CASTLE_INDEX    7

#define VILLAGE_OPTION_COUNT 3
#define MONSTER_OPTION_COUNT 5
#define CAVE_OPTION_COUNT    5
#define DESERT_OPTION_COUNT  3

#define THUG_DESCRIPTION_COUNT 13

typedef enum
{
    PERSONALITY_UNUSED,
    PERSONALITY_ACROBAT,
    PERSONALITY_AMAZON,
    PERSONALITY_BAT_OUT_OF_HELL,
    PERSONALITY_CLOWN,
    PERSONALITY_CONTRARIAN,
    PERSONALITY_CRY_BABY,
    PERSONALITY_DARE_DEVIL,
    PERSONALITY_DAY_DREAMER,
    PERSONALITY_DRUDGE,
    
    PERSONALITY_EGG_HEAD,
    PERSONALITY_EVERY_MAN,
    PERSONALITY_FREE_SPIRIT,
    PERSONALITY_GENIUS,
    PERSONALITY_GOOD_EGG,
    PERSONALITY_GOURMAND,
    PERSONALITY_HAPPY_CAMPER,
    PERSONALITY_IDEALIST,
    PERSONALITY_IRONCLAD,

    PERSONALITY_KLUTZ,
    PERSONALITY_LAZY_BONES,
    PERSONALITY_LONE_WOLF,
    PERSONALITY_LOTHARIO,
    PERSONALITY_LOUT,
    PERSONALITY_LUCKY_DEVIL,
    PERSONALITY_MEAT_HEAD,
    PERSONALITY_MEDDLER,
    PERSONALITY_MULE,

    PERSONALITY_NARCISSIST,
    PERSONALITY_PARAGON,
    PERSONALITY_PLUGGER,
    PERSONALITY_PRINCESS,
    PERSONALITY_SCATTER_BRAIN,
    PERSONALITY_SHOW_OFF,
    PERSONALITY_SHRINKING_VIOLET,
    PERSONALITY_SLIPPERY_DEVIL,
    PERSONALITY_SOCIALITE,

    PERSONALITY_SORE_LOSER,
    PERSONALITY_SPOILT_BRAT,
    PERSONALITY_STRAIGHT_ARROW,
    PERSONALITY_THUG,
    PERSONALITY_TOM_BOY,
    PERSONALITY_TOUGH_COOKIE,
    PERSONALITY_VAMP,
    PERSONALITY_WIMP,
    PERSONALITY_WIT,
} personality_types;
extern personality_types personality_types_state;

typedef struct 
{
    int x_coords[THUG_DESCRIPTION_COUNT];
    char *scenario;
    char *name;
    char *description[THUG_DESCRIPTION_COUNT];
} test_personality_test_results_t;

extern test_personality_test_results_t acrobat;
extern test_personality_test_results_t amazon;
extern test_personality_test_results_t bat_out_of_hell;
extern test_personality_test_results_t clown;
extern test_personality_test_results_t contrarian;
extern test_personality_test_results_t crybaby;
extern test_personality_test_results_t daredevil;
extern test_personality_test_results_t daydreamer;
extern test_personality_test_results_t drudge;
extern test_personality_test_results_t egghead;
extern test_personality_test_results_t amazon;
extern test_personality_test_results_t amazon;
extern test_personality_test_results_t amazon;
extern test_personality_test_results_t amazon;
extern test_personality_test_results_t amazon;
extern test_personality_test_results_t amazon;
extern test_personality_test_results_t amazon;
extern test_personality_test_results_t amazon;


#endif
