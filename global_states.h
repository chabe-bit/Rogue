#ifndef GLOBAL_STATES_H
#define GLOBAL_STATES_H

typedef enum
{
    GAME_STATE_TITLE_SCREEN,
    GAME_STATE_NEW_GAME,
    GAME_STATE_LOAD_GAME,
    GAME_STATE_SETTINGS,
    GAME_STATE_EXIT,
    GAME_STATE_GAMEPLAY,
    GAME_STATE_UNKNOWN
} game_state_e;
game_state_e game_state = GAME_STATE_TITLE_SCREEN;


typedef enum
{
    NEW_GAME_CHARACTER_SELECT,
    NEW_GAME_UNKNOWN
} new_game_state_e;
new_game_state_e new_game_state = NEW_GAME_CHARACTER_SELECT;

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
    VOL_SETTINGS_NONE,
    VOL_SETTINGS_MASTER,
    VOL_SETTINGS_MUSIC,
    VOL_SETTINGS_SFX,
    VOL_SETTINGS_APPLY
} volume_settings;
volume_settings volume_settings_state = VOL_SETTINGS_NONE;

typedef enum
{
    VOL_LEVEL_NONE,
    VOL_LEVEL_GREEN,
    VOL_LEVEL_YELLOW,
    VOL_LEVEL_ORANGE,
    VOL_LEVEL_RED
} volume_level;
volume_level volume_level_state = VOL_LEVEL_NONE;

typedef enum 
{
    CLASS_NONE,
    CLASS_KNIGHT,
    CLASS_PALADIN,
    CLASS_MAGE,
    CLASS_ARCHER
} character_class_selection;
character_class_selection character_class_selection_state = CLASS_NONE;

typedef enum 
{
    CLASS_POINTS_NONE,
    CLASS_POINTS_PERSONALITY,
    CLASS_POINTS_PRESET,
    CLASS_POINTS_MANUAL,
} character_class_point_allocation_method;
character_class_point_allocation_method character_class_point_allocation_method_state = CLASS_POINTS_NONE;

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
    CLASS_NAME_DELETE
} character_class_name_submission;
character_class_name_submission character_class_name_submission_state = CLASS_NAME_NONE;

typedef enum
{
    CONFIRMATION_YES,
    CONFIRMATION_NO,
    CONFIRMATION_NONE,
} yes_or_no_buttons;
yes_or_no_buttons yes_or_no_buttons_state =  CONFIRMATION_NONE;





#endif
