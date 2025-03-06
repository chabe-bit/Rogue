#ifndef GLOBAL_STATES_H
#define GLOBAL_STATES_H

typedef enum 
{
    TITLE_NONE,
    TITLE_NEW_GAME,
    TITLE_LOAD_GAME,
    TITLE_SETTINGS,
    TITLE_EXIT,
} title_screen;
title_screen title_screen_state = TITLE_NONE;

typedef enum 
{
    CLASS_SELECT_NONE,
    CLASS_SELECT_KNIGHT,
    CLASS_SELECT_PALADIN,
    CLASS_SELECT_WIZARD,
    CLASS_SELECT_ARCHER,
    CLASS_SELECT_BACK
} class_select_type;
class_select_type class_select_state = CLASS_SELECT_NONE;

typedef enum 
{
    CLASS_ALLOCATION_NONE,
    CLASS_ALLOCATION_PERSONALITY,
    CLASS_ALLOCATION_PRESET,
    CLASS_ALLOCATION_MANUAL,
    CLASS_ALLOCATION_BACK
} class_allocation_type;
class_allocation_type class_allocation_state = CLASS_ALLOCATION_NONE;

// ENTIRELY BASED on DQ3 remaster
// https://game8.co/games/Dragon-Quest-3/archives/464271
typedef enum
{
    CLASS_PERSONALITY_RESULT_NONE,
    CLASS_PERSONALITY_RESULT_VILLAGE,
    CLASS_PERSONALITY_RESULT_MONSTER,
    CLASS_PERSONALITY_RESULT_FOREST,
    CLASS_PERSONALITY_RESULT_CAVE,
    CLASS_PERSONALITY_RESULT_DESERT,
    CLASS_PERSONALITY_RESULT_TOWER,
    CLASS_PERSONALITY_RESULT_THEATER,
    CLASS_PERSONALITY_RESULT_CASTLE,
} character_class_personality_test_result;
character_class_personality_test_result character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_NONE;

typedef enum
{
    CLASS_NAME_NONE,
    CLASS_NAME_ENTER,
    CLASS_NAME_DELETE,
    CLASS_NAME_CONFIRM
} character_class_name_submission;
character_class_name_submission character_class_name_submission_state = CLASS_NAME_NONE;

#endif
