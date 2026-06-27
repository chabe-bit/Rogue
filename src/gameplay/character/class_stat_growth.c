#include "class_stat_growth.h"

// TODO: Complete the rest of the classes
f32 knight_stat_baseline[5] = {
    15, 15, 15, 15, 15
};

f32 knight_growth_per_level[][20] = {
    // STR
    {4, 4, 4, 4, 4, 8, 8, 8, 8, 8,
    12, 12, 12, 12, 12, 16, 16, 16, 16, 16},

    // AGI
    {9, 9, 9, 9, 9, 9, 4, 4, 4, 4,
    4, 16, 16, 16, 16, 16, 16, 16, 16, 16},

    // VIT
    {9, 9, 9, 9, 9, 9, 4, 4, 4, 4,
    4, 16, 16, 16, 16, 16, 16, 16, 16, 16},

    // WIS
    {9, 9, 9, 9, 9, 9, 4, 4, 4, 4,
    4, 16, 16, 16, 16, 16, 16, 16, 16, 16},

    // LCK
    {9, 9, 9, 9, 9, 9, 4, 4, 4, 4,
    4, 16, 16, 16, 16, 16, 16, 16, 16, 16},
};

void Class_InitStatBaseline(class_stat_growth_per_level_t *class, f32 stat_baseline[])
{
    class->strength.baseline    = stat_baseline[STAT_STR];
    class->agility.baseline     = stat_baseline[STAT_AGI];
    class->stamina.baseline     = stat_baseline[STAT_VIT];
    class->wisdom.baseline      = stat_baseline[STAT_WIS];
    class->luck.baseline        = stat_baseline[STAT_LCK];
}

void Class_InitStatGrowthPerLevel(class_stat_growth_per_level_t *class, f32 stat_growth[][20])
{
    // TODO: Explain level up system
    // TODO: Use an index outside of this loop to increment to the next stat gain on level up
    for (i32 i = 0; i < 20; ++i)
    {
        if (stat_growth[STAT_STR][i] > class->strength.baseline ||
            stat_growth[STAT_AGI][i] > class->agility.baseline  ||
            stat_growth[STAT_VIT][i] > class->stamina.baseline  ||
            stat_growth[STAT_WIS][i] > class->wisdom.baseline   ||
            stat_growth[STAT_LCK][i] > class->luck.baseline)
        {
            stat_growth[STAT_STR][i] = rand() % 2;
            stat_growth[STAT_AGI][i] = rand() % 2;
            stat_growth[STAT_VIT][i] = rand() % 2;
            stat_growth[STAT_WIS][i] = rand() % 2;
            stat_growth[STAT_LCK][i] = rand() % 2;
        }


        class->strength.growth_per_level[i]   = stat_growth[STAT_STR][i]; 
        class->agility.growth_per_level[i]    = stat_growth[STAT_AGI][i]; 
        class->stamina.growth_per_level[i]    = stat_growth[STAT_VIT][i]; 
        class->wisdom.growth_per_level[i]     = stat_growth[STAT_WIS][i]; 
        class->luck.growth_per_level[i]       = stat_growth[STAT_LCK][i]; 
        
        printf("agi stat growth: %.2f\n", class->agility.growth_per_level[i]);
    } 
}


