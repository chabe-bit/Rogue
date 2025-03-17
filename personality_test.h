#ifndef PERSONALITY_TEST_H
#define PERSONALITY_TEST_H

#include "common.h"

#define PERSONALITY_TEST_QUESTIONS 50

extern const char *personality_questions[PERSONALITY_TEST_QUESTIONS];

typedef struct
{
    i32 index;
    bool is_active;
    const char *table[50]; 
} personality_test_t; 

// ENTIRELY BASED on DQ3 remaster
// https://game8.co/games/Dragon-Quest-3/archives/464271
typedef enum
{
    PERSONALITY_SCENARIO_RESULT_NONE,
    PERSONALITY_SCENARIO_RESULT_VILLAGE,
    PERSONALITY_SCENARIO_RESULT_MONSTER,
    PERSONALITY_SCENARIO_RESULT_FOREST,
    PERSONALITY_SCENARIO_RESULT_CAVE,
    PERSONALITY_SCENARIO_RESULT_DESERT,
    PERSONALITY_SCENARIO_RESULT_TOWER,
    PERSONALITY_SCENARIO_RESULT_THEATER,
    PERSONALITY_SCENARIO_RESULT_CASTLE,
} personality_scenario_result_type;
extern personality_scenario_result_type personality_scenario_result_state;


void PersonalityTest_BranchQuestions(personality_test_t *personality_test, i32 index);

#endif
