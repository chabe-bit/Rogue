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

/*
class_base_stats_t class_base_stats[4] = {0};

class_base_stats[KNIGHT_ID].strength = 10;
class_base_stats[KNIGHT_ID].resilience = 10;
class_base_stats[KNIGHT_ID].agility = 5;
class_base_stats[KNIGHT_ID].stamina = 12;
class_base_stats[KNIGHT_ID].wisdom = 2;
class_base_stats[KNIGHT_ID].luck = 3;
class_base_stats[KNIGHT_ID].max_hp = (2 * class_base_stats[KNIGHT_ID].stamina) + 5; // formula from DQ3 -> HP = [3 * VIT / 2] + 5
class_base_stats[KNIGHT_ID].hp = class_base_stats[KNIGHT_ID].max_hp; // 
class_base_stats[KNIGHT_ID].max_mp = class_base_stats[KNIGHT_ID].wisdom; // MP == Wisdom
class_base_stats[KNIGHT_ID].mp = class_base_stats[KNIGHT_ID].max_mp; // 
class_base_stats[KNIGHT_ID].attack = class_base_stats[KNIGHT_ID].strength + 0; // strength + weapon power
class_base_stats[KNIGHT_ID].defense = class_base_stats[KNIGHT_ID].resilience + 0; // resilience + armor 

class_base_stats[PALADIN_ID].strength = 6;
class_base_stats[PALADIN_ID].resilience = 15;
class_base_stats[PALADIN_ID].agility = 4;
class_base_stats[PALADIN_ID].stamina = 16;
class_base_stats[PALADIN_ID].wisdom = 7;
class_base_stats[PALADIN_ID].luck = 0;
class_base_stats[PALADIN_ID].max_hp = (2 * class_base_stats[PALADIN_ID].stamina) + 5; // formula from DQ3 -> HP = [3 * VIT / 2] + 5
class_base_stats[PALADIN_ID].hp = class_base_stats[PALADIN_ID].max_hp; // 
class_base_stats[PALADIN_ID].max_mp = class_base_stats[PALADIN_ID].wisdom; // MP == Wisdom
class_base_stats[PALADIN_ID].hp = class_base_stats[PALADIN_ID].max_mp; // 
class_base_stats[PALADIN_ID].attack = class_base_stats[PALADIN_ID].strength + -1; // strength + weapon power
class_base_stats[PALADIN_ID].defense = class_base_stats[PALADIN_ID].resilience + -1; // resilience + armor 

class_base_stats[WIZARD_ID].strength = 3;
class_base_stats[WIZARD_ID].resilience = 7;
class_base_stats[WIZARD_ID].agility = 6;
class_base_stats[WIZARD_ID].stamina = 7;
class_base_stats[WIZARD_ID].wisdom = 17;
class_base_stats[WIZARD_ID].luck = 7;
class_base_stats[WIZARD_ID].max_hp = (2 * class_base_stats[WIZARD_ID].stamina) + 5; // formula from DQ3 -> HP = [3 * VIT / 2] + 5
class_base_stats[WIZARD_ID].hp = class_base_stats[WIZARD_ID].max_hp; // 
class_base_stats[WIZARD_ID].max_mp = class_base_stats[WIZARD_ID].wisdom; // MP == Wisdom
class_base_stats[WIZARD_ID].hp = class_base_stats[WIZARD_ID].max_mp; // 
class_base_stats[WIZARD_ID].attack = class_base_stats[WIZARD_ID].strength + -1; // strength + weapon power
class_base_stats[WIZARD_ID].defense = class_base_stats[WIZARD_ID].resilience + -1; // resilience + armor 

class_base_stats[ARCHER_ID].strength = 4;
class_base_stats[ARCHER_ID].resilience = 7;
class_base_stats[ARCHER_ID].agility = 14;
class_base_stats[ARCHER_ID].stamina = 8;
class_base_stats[ARCHER_ID].wisdom = 3;
class_base_stats[ARCHER_ID].luck = 8;
class_base_stats[ARCHER_ID].max_hp = (2 * class_base_stats[ARCHER_ID].stamina) + 5; // formula from DQ3 -> HP = [3 * VIT / 2] + 5
class_base_stats[ARCHER_ID].hp = class_base_stats[ARCHER_ID].max_hp; // 
class_base_stats[ARCHER_ID].max_mp = class_base_stats[ARCHER_ID].wisdom; // MP == Wisdom
class_base_stats[ARCHER_ID].hp = class_base_stats[ARCHER_ID].max_mp; // 
class_base_stats[ARCHER_ID].attack = class_base_stats[ARCHER_ID].strength + -1; // strength + weapon power
class_base_stats[ARCHER_ID].defense = class_base_stats[ARCHER_ID].resilience + -1; // resilience + armor 

printf("Max HP: %d\n", class_base_stats[KNIGHT_ID].max_hp);

*/
