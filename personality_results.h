#ifndef PERSONALITY_RESULT_H
#define PERSONALITY_RESULT_H

#include "common.h"

#define PERSONALITY_DESCRIPTION_SIZE 13

typedef enum
{
    PERSONALITY_UNUSED,
    PERSONALITY_ACROBAT,
    PERSONALITY_AMAZON,
    PERSONALITY_BAT_OUT_OF_HELL,
    PERSONALITY_CLOWN,
    PERSONALITY_CONTRARIAN,
    PERSONALITY_CRYBABY,
    PERSONALITY_DAREDEVIL,
    PERSONALITY_DAYDREAMER,
    PERSONALITY_DRUDGE,
    
    PERSONALITY_EGGHEAD,
    PERSONALITY_EVERYMAN,
    PERSONALITY_FREE_SPIRIT,
    PERSONALITY_GENIUS,
    PERSONALITY_GOOD_EGG,
    PERSONALITY_GOURMAND,
    PERSONALITY_HAPPY_CAMPER,
    PERSONALITY_IDEALIST,
    PERSONALITY_IRONCLAD,

    PERSONALITY_KLUTZ,
    PERSONALITY_LAZYBONES,
    PERSONALITY_LONE_WOLF,
    PERSONALITY_LOTHARIO,
    PERSONALITY_LOUT,
    PERSONALITY_LUCKY_DEVIL,
    PERSONALITY_MEATHEAD,
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
    PERSONALITY_TOMBOY,
    PERSONALITY_TOUGH_COOKIE,
    PERSONALITY_VAMP,
    PERSONALITY_WIMP,
    PERSONALITY_WIT,
} personality_types;
extern personality_types personality_types_state;

typedef struct 
{
    int x_coords[PERSONALITY_DESCRIPTION_SIZE];
    char *scenario;
    char *name;
    SDL_Color colors[10];
    char *description[PERSONALITY_DESCRIPTION_SIZE];
} personality_results_t;

extern personality_results_t acrobat;
extern personality_results_t amazon;
extern personality_results_t bat_out_of_hell;
extern personality_results_t clown;
extern personality_results_t contrarian;
extern personality_results_t crybaby;
extern personality_results_t daredevil;
extern personality_results_t daydreamer;
extern personality_results_t drudge;
extern personality_results_t egghead;
extern personality_results_t everyman;
extern personality_results_t free_spirit;
extern personality_results_t genius;
extern personality_results_t good_egg;
extern personality_results_t happy_camper;
extern personality_results_t idealist;
extern personality_results_t ironclad;
extern personality_results_t klutz;

extern personality_results_t lazybones;
extern personality_results_t lone_wolf;
extern personality_results_t lothario;
extern personality_results_t lout;
extern personality_results_t lucky_devil;
extern personality_results_t meathead;
extern personality_results_t meddler;
extern personality_results_t mule;
extern personality_results_t narcissist;
extern personality_results_t paragon;
extern personality_results_t plugger;
extern personality_results_t princess;
extern personality_results_t scatterbrain;
extern personality_results_t show_off;
extern personality_results_t shrinking_violet;
extern personality_results_t slippery_devil;
extern personality_results_t socialite;
extern personality_results_t sore_loser;

extern personality_results_t spoilt_brat;
extern personality_results_t straight_arrow;
extern personality_results_t thug;
extern personality_results_t tomboy;
extern personality_results_t tough_cookie;
extern personality_results_t vamp;
extern personality_results_t wimp;
extern personality_results_t wit;




#endif
