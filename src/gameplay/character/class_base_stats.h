#ifndef CLASS_BASE_STATS_H
#define CLASS_BASE_STATS_H

#include "common.h"
    
#define KNIGHT_ID   0
#define PALADIN_ID  1
#define WIZARD_ID   2
#define ARCHER_ID   3
    
// Base stats should be set to a baseline of let's say 5 by default, so that depending on the method the player chooses to allocate their points,
// that is where it's initialized. 
// Ex: Personality will default the selected class's stats of what a wizard would be, then assign your personality.
// But manual allocation you're shown the baseline of "5" per stat, where they can choose what to allocate to the choose a personality.
// Preset would be defaulting the class's stat then choosing a personality.
  
// Base stats of each class from here: https://dragon-quest.org/wiki/List_of_vocations_in_Dragon_Quest_III#The_vocations
typedef struct
{
    i32 index; // Iterate over for player to select

    i32 strength; // Determines physical dmg
    i32 resilience; // Determines damage received
    i32 agility; // Determines who acts first in battle - probably change to evasiveness
    i32 stamina; // Determines HP value and scaling
    i32 wisdom; // Determines magic dmg and MP storage
    i32 luck; // Determines crit chance

    // Maybe this is seperate to just the player and not class?
    i32 max_hp; // Max HP 
    i32 hp; // Current HP
    i32 max_mp; // Max MP
    i32 mp; // Current MP
    i32 attack; // Combination of your strength and weapon damage
    i32 defense; // 
} class_base_stats_t;

extern i32 knight_base_stat_data[];
extern i32 paladin_base_stat_data[];
extern i32 wizard_base_stat_data[];
extern i32 archer_base_stat_data[];


class_base_stats_t Class_InitBaseStats(int class_base_stat_data[]);


#endif
