#ifndef CLASS_STAT_GROWTH_H
#define CLASS_STAT_GROWTH_H

#include "common.h"

// Stat growth:
// Baseline -> Value that if the gain value exceeds it, the character loses their normal gain rate and instead rolls a 49/50 chance to gain 
// +0 or +0.
// Think of it like a bar graph per stat attribute where there's a limit for each for each class, the 49/50 chance roll is simply there to
// balance in a way that from level 0 to 50, one's HP growth will be consistent in gaining +4 or so per level until 50, but because they excel in
// magic, their INT will grow at a large rate but cap quickly.

typedef enum
{
    STAT_STR,
    STAT_AGI,
    STAT_VIT,
    STAT_WIS,
    STAT_LCK
} stat_type;

typedef struct
{
    struct {
        float baseline;
        float growth_per_level[20];
    } strength;

    struct {
        float baseline;
        float growth_per_level[20];
    } agility;

    struct {
        float baseline;
        float growth_per_level[20];
    } stamina;

    struct {
        float baseline;
        float growth_per_level[20];
    } wisdom;

    struct {
        float baseline;
        float growth_per_level[20];
    } luck;

} class_stat_growth_per_level_t;

extern f32 knight_stat_baseline[5];
extern f32 knight_growth_per_level[][20];


void Class_InitStatBaseline(class_stat_growth_per_level_t *class, f32 stat_baseline[]);
void Class_InitStatGrowthPerLevel(class_stat_growth_per_level_t *class, f32 stat_growth[][20]);




#endif
