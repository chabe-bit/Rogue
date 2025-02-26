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

#define DESCRIPTION_COUNT 13

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
    int x_coords[DESCRIPTION_COUNT];
    char *scenario;
    char *name;
    char *description[DESCRIPTION_COUNT];
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
extern test_personality_test_results_t everyman;
extern test_personality_test_results_t free_spirit;
extern test_personality_test_results_t genius;
extern test_personality_test_results_t good_egg;
extern test_personality_test_results_t happy_camper;
extern test_personality_test_results_t idealist;
extern test_personality_test_results_t ironclad;
extern test_personality_test_results_t klutz;

extern test_personality_test_results_t lazybones;
extern test_personality_test_results_t lone_wolf;
extern test_personality_test_results_t lothario;
extern test_personality_test_results_t lout;
extern test_personality_test_results_t lucky_devil;
extern test_personality_test_results_t meathead;
extern test_personality_test_results_t meddler;
extern test_personality_test_results_t mule;
extern test_personality_test_results_t narcissist;
extern test_personality_test_results_t paragon;
extern test_personality_test_results_t plugger;
extern test_personality_test_results_t princess;
extern test_personality_test_results_t scatterbrain;
extern test_personality_test_results_t show_off;
extern test_personality_test_results_t shrinking_violet;
extern test_personality_test_results_t slippery_devil;
extern test_personality_test_results_t socialite;
extern test_personality_test_results_t sore_loser;

extern test_personality_test_results_t spoilt_brat;
extern test_personality_test_results_t straight_arrow;
extern test_personality_test_results_t thug;
extern test_personality_test_results_t tomboy;
extern test_personality_test_results_t tough_cookie;
extern test_personality_test_results_t vamp;
extern test_personality_test_results_t wimp;
extern test_personality_test_results_t wit;


#endif
