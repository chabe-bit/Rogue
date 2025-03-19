#include "personality_test.h"

personality_scenario_result_type personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_NONE;

const char *personality_questions[PERSONALITY_TEST_QUESTIONS] = {
    // Starting questions 
    "",
    "Do you find life boring?",
    "Do you believe that the sun in the sky above is the king of all nature...?",
    "Is adventuring a hardship?",
    "Does victory come in battle?",
    "Do you feel more confident with strong equipment as opposed to taking allies?",
        
    // Follow up questions
    "Do you enjoy talking with town people?",
    "You see a cave. Do you have an urge to explore it?",
    "Do you spend more on weapons than armor?",
    "Do you help people in trouble?",
    "Rather than an expensive nearby inn, would you go to a cheap inn that's far away?",
    "Do you have trouble sleeping because you are thinking too much?",
    "Do you prefer the mountains to the sea?",
    "Do you find bordem annoying?",
    "Do you dream often?",
    "Do you prefer magic to a sword?",
    "Do you wonder what it's like to fly?",
    "Do you think birds are free?",
    "Do you ever dream of being pursued?",
    "Do you find the company of unfamiliar people tiresome...?",
    "Do you trust in the words of those who tell fortunes...?",
    "If you could be rebord, would it be as a prince or a princess?",
    "Do you find it difficult to turn down others' request?",
    "Are you able to prevent failure from preying upon your mind...?",
    "Do you find yourself unable to argue with others, even if you disagree with them strongly...?",
    "Do you enjoy physical activity?",
    "Are cats cuter than dogs?",
    "Is it wrong to be attracted to a friend's lover?",
    "Do you get embarrassed by other's praise?",
    "Do you worry about what others think of the way you dress?",
    "Do you enjoy physical activity?",
    "Do little things bother you?",
    "Do you put your thoughts into action right away?",
    "Do you have confidence in your beliefs no matter what?",
    "Do you believe that a promise, once made, can under no circumstances be broken...?",
    "Do you believe in Gods?",
    "Do you get busy with one thing and lose sight of other goals?",
    "Do you save your favorite food for last?",
    "Do you daydream to amuse yourself?",
    "If there were one wish in the world you could have come true, could you say that wish right now?",
    "Do you have many friends?",
    "Do you dwell on the past often?",
    "Are you bothered by gossip?",
    "Do you think the world has more sadness than happiness?",
    "If you're conned, do you share some of the responsibility?",
    "Do you want to grow up quickly?", 
    "If something is unattainable, do you only want it more?",
    "If you hold onto a dream long enough, will it come true?",
    "Do you hold anything precious?",
    "Do you trip on a boulder and blame youself?"
};


void PersonalityTest_BranchQuestions(personality_test_t *personality_test, i32 index)
{
    printf("confirm index: %d\n", index);
    switch (index)
    {
        case 0: // yes
        {
            // turn into a function
            switch (personality_test->index)
            {
                case 1:
                {
                    personality_test->index = 7;
                } break;
                case 2:
                {
                    personality_test->index = 14;
                } break;
                case 3:
                {
                    personality_test->index = 6;
                } break;
                case 4:
                {
                    personality_test->index = 15;
                } break;
                case 5:
                {
                    personality_test->index = 8;
                } break;
                case 6:
                {
                    personality_test->index = 7;
                } break;
                case 7:
                {
                    personality_test->index = 10;
                } break;
                case 8:
                {
                    personality_test->index = 10;
                } break;
                case 9:
                {
                    personality_test->index = 11;
                } break;
                case 10:
                {
                    personality_test->index = 14;
                } break;
                case 11:
                {
                    personality_test->index = 14;
                } break;
                case 12:
                {
                    personality_test->index = 31;
                } break;
                case 13:
                {
                    personality_test->index = 25;
                } break;
                case 14:
                {
                    personality_test->index = 18;
                } break;
                case 15:
                {
                    personality_test->index = 16;
                } break;
                case 16:
                {
                    personality_test->index = 17;
                } break;
                case 17:
                {
                    personality_test->index = 21;
                } break;
                case 18:
                {
                    personality_test->index = 19;
                } break;
                case 19:
                {
                    personality_test->index = 20;
                } break;
                case 20:
                {
                    personality_test->index = 21;
                } break; 
                case 21:
                {
                    personality_test->index = 23;
                } break;
                case 22:
                {
                    personality_test->index = 38;
                } break;
                case 23:
                {
                    personality_test->index = 24;
                } break;
                case 24:
                {
                    personality_test->index = 34;
                } break;
                case 25:
                {
                    personality_test->index = 31;
                } break;
                case 26:
                {
                    personality_test->index = 27;
                } break;
                case 27:
                {
                    personality_test->index = 28;
                } break;
                case 28:
                {
                    personality_test->index = 29;
                } break;
                case 29:
                {
                    personality_test->index = 30;
                } break;
                case 30:
                {
                    personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_VILLAGE; // final question
                } break;
                case 31:
                {
                    personality_test->index = 32;
                } break;
                case 32:
                {
                    personality_test->index = 33;
                } break;
                case 33:
                {
                    personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_DESERT; // final question
                } break;
                case 34:
                {
                    personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_FOREST; //CLASS_PERSONALITY_RESULT_MONSTER; // final question
                } break;
                case 35:
                {
                    personality_test->index = 0; // final question
                } break;
                case 36:
                {
                    personality_test->index = 37;
                } break;
                case 37:
                {
                    personality_test->index = 43;
                } break;
                case 38:
                {
                    personality_test->index = 39;
                } break;
                case 39:
                {
                    personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_TOWER;
                } break;
                case 40:
                {
                    personality_test->index = 42;
                } break; 
                case 41:
                {
                    personality_test->index = 43;
                } break;
                case 42:
                {
                    personality_test->index = 43;
                } break;
                case 43:
                {
                    personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_FOREST; 
                } break;
                case 44:
                {
                    personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_THEATER; 
                } break;
                case 45:
                {
                    personality_test->index = 47;
                } break;
                case 46:
                {
                    personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_CAVE; 
                } break;
                case 47:
                {
                    personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_CAVE; 
                } break;
                case 48:
                {
                    personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_FOREST;//CLASS_PERSONALITY_RESULT_CASTLE;
                } break;
                case 49:
                {
                    personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_FOREST;//CLASS_PERSONALITY_RESULT_CASTLE; 
                } break;
            }
        } break;
        case 1: // no
        {
            switch (personality_test->index)
            {
                case 1:
                {
                    personality_test->index = 9;
                } break;
                case 2:
                {
                    personality_test->index = 8;
                } break;
                case 3:
                {
                    personality_test->index = 8;
                } break;
                case 4:
                {
                    personality_test->index = 6;
                } break;
                case 5:
                {
                    personality_test->index = 16;
                } break;
                case 6:
                {
                    personality_test->index = 8;
                } break;
                case 7:
                {
                    personality_test->index = 5;
                } break;
                case 8:
                {
                    personality_test->index = 9;
                } break;
                case 9:
                {
                    personality_test->index = 12;
                } break;
                case 10:
                {
                    personality_test->index = 13;
                } break;
                case 11:
                {
                    personality_test->index = 13;
                } break;
                case 12:
                {
                    personality_test->index = 14;
                } break;
                case 13:
                {
                    personality_test->index = 15;
                } break;
                case 14:
                {
                    personality_test->index = 19;
                } break;
                case 15:
                {
                    personality_test->index = 20;
                } break;
                case 16:
                {
                    personality_test->index = 22;
                } break;
                case 17:
                {
                    personality_test->index = 25;
                } break;
                case 18:
                {
                    personality_test->index = 23;
                } break;
                case 19:
                {
                    personality_test->index = 25;
                } break;
                case 20:
                {
                    personality_test->index = 22;
                } break; 
                case 21:
                {
                    personality_test->index = 23;
                } break;
                case 22:
                {
                    personality_test->index = 38;
                } break;
                case 23:
                { 
                    personality_test->index = 40;
                } break;
                case 24:
                {
                    personality_test->index = 25;
                } break;
                case 25:
                {
                    personality_test->index = 26;
                } break;
                case 26:
                {
                    personality_test->index = 28;
                } break;
                case 27:
                {
                    personality_test->index = 29;
                } break;
                case 28:
                {
                    personality_test->index = 30;
                } break;
                case 29:
                {
                    personality_test->index = 30;
                } break;
                case 30:
                {
                    personality_test->index = 40; 
                } break;
                case 31:
                {
                    personality_test->index = 34;
                } break;
                case 32:
                {
                    personality_test->index = 36;
                } break;
                case 33:
                {
                    personality_test->index = 36;
                } break;
                case 34:
                {
                    personality_test->index = 36;
                } break;
                case 35:
                {
                    personality_test->index = 36; 
                } break;
                case 36:
                {
                    personality_test->index = 48;
                } break;
                case 37:
                {
                    personality_test->index = 49;
                } break;
                case 38:
                {
                    personality_test->index = 40;
                } break;
                case 39:
                {
                    personality_test->index = 41; 
                } break;
                case 40:
                {
                    personality_test->index = 41;
                } break; 
                case 41:
                {
                    personality_test->index = 42;
                } break;
                case 42:
                {
                    personality_test->index = 44;
                } break;
                case 43:
                {
                    personality_test->index = 45; 
                } break;
                case 44:
                {
                    personality_test->index = 45;
                } break;
                case 45:
                {
                    personality_test->index = 46;
                } break;
                case 46:
                {
                    personality_test->index = 47; 
                } break;
                case 47:
                {
                    personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_THEATER; 
                } break;
                case 48:
                {
                    personality_test->index = 49;
                } break;
                case 49:
                {
                    personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_CAVE;
                } break;
            }
        } break;
    }

}
