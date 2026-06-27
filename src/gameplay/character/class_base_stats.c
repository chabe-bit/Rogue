#include "class_base_stats.h"

i32 knight_base_stat_data[] = {
    10, // str
    10, // res
    5,  // agi
    12, // sta
    2,  // wis 
    3,  // lck
};

i32 paladin_base_stat_data[] = {
    6, // str
    15, // res
    4,  // agi
    16, // sta
    7,  // wis
    0,  // lck
};

i32 wizard_base_stat_data[] = {
    3, // str
    7, // res
    6,  // agi
    7, // sta
    17,  // wis
    7,  // lck
};

i32 archer_base_stat_data[] = {
    4, // str
    7, // res
    14,  // agi
    8, // sta
    3,  // wis
    8,  // lck
};

class_base_stats_t Class_InitBaseStats(int class_base_stat_data[])
{
    class_base_stats_t base_stats = {0};

    base_stats.strength        = class_base_stat_data[0]; // str
    base_stats.resilience      = class_base_stat_data[1]; // res 
    base_stats.agility         = class_base_stat_data[2]; // res 
    base_stats.stamina         = class_base_stat_data[3]; // sta
    base_stats.wisdom          = class_base_stat_data[4]; // wis
    base_stats.luck            = class_base_stat_data[5]; // lck

    base_stats.max_hp          = (2 * class_base_stat_data[3]) + 5; // base hp
    base_stats.max_mp          = class_base_stat_data[4]; // base mp == base wis
    
    base_stats.hp              = base_stats.max_hp; // current hp holder
    base_stats.mp              = base_stats.max_mp; // current mp holder

    return base_stats;
}
