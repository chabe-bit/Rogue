#include "personality.h"

// https://dragon-quest.org/wiki/List_of_personality_types_in_Dragon_Quest_III#Personalities_and_stat_effects

personality_types personality_types_state = PERSONALITY_UNUSED;

test_personality_test_results_t clown  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Castle Scenario",
    .name = "Clown",
    .description = {
         "You are that classic character...",
         "the clown...When you see someone",
         "working away seriously at",
         "something, your first urge is",
         "to lift their spirits with a joke...",
         "You are always making others laugh,",
         "and for this, you are well-loved.",
         "But is this the real you?",
         "STR ( )",
         "AGL (+)",
         "VIT (-)",
         "WIS (+)",
         "LCK (+)"
    }
};

test_personality_test_results_t daredevil  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Daredevil",
    .description = {
         "You are a risk taker...a daredevil...",
         "You do not think of the future, you live",
         "in the now, and in the face of failure",
         "you do not give up. You simply dust",
         "yourself off and try again...",
         "But because of this, you're more liable",
         "to make the same mistakes over again",
         "and over again.",
         "STR (-)",
         "AGL (+)",
         "VIT (+)",
         "WIS ( )",
         "LCK ( )"
    }
};

test_personality_test_results_t daydreamer  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Daydreamer",
    .description = {
         "You are something of a daydreamer...",
         "You are kind to others, and this kindess",
         "is effortless and heartfelt, making",
         "it easy for others to like you...",
         "Perhaps you are guilty of daydreaming",
         "more often than you should,",
         "making you less active than you",
         "otherwise might be...",
         "STR (-)",
         "AGL (+)",
         "VIT (-)",
         "WIS (+)",
         "LCK ( )"
    }
};

test_personality_test_results_t drudge  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Drudge",
    .description = {
         "You are one of life's strugglers...",
         "You may seem quietly competent to others,",
         "but unbeknown to them, your life is a",
         "constant battle...And though you devote",
         "yourself tirelessly to things you love,",
         "you have no time for others. Learn",
         "to strive at things you do not enjoy,",
         "and perhaps you'll find your strength.",
         "STR (+)",
         "AGL (-)",
         "VIT (+)",
         "WIS ( )",
         "LCK (-)"
    }
};

test_personality_test_results_t egghead  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Egghead",
    .description = {
         "You are what some would describe as an",
         "'egghead'...A person of a well-formed",
         "brain, gifted in thought...But perhaps",
         "not so much in action. If a once-in-a",
         "-lifetime chance were to come along,",
         "you may not act quickly enough to take",
         "it. You are wise enough to recognize",
         "this, but mistakes put a hold on you.",
         "STR (-)",
         "AGL (+)",
         "VIT (-)",
         "WIS (+)",
         "LCK (-)"
    }
};

test_personality_test_results_t good_egg  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Good Egg",
    .description = {
         "You are what some would describe as an",
         "'egghead'...A person of a well-formed",
         "brain, gifted in thought...But perhaps",
         "not so much in action. If a once-in-a",
         "-lifetime chance were to come along,",
         "you may not act quickly enough to take",
         "it. You are wise enough to recognize",
         "this, but mistakes put a hold on you.",
         "STR (-)",
         "AGL (+)",
         "VIT (-)",
         "WIS (+)",
         "LCK (-)"
    }
};



test_personality_test_results_t thug = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Thug",
    .description = {
         "You appear to be a thug...",
         "and though you may not realize it,",
         "your thuggishness is a worry and an",
         "inconvenience to all around you.",
         "Even you had done so, your",
         "lack of empathy would",
         "probably lead you to assume",
         "that they think as you do...",
         "STR (+)",
         "AGL (-)",
         "VIT (-)",
         "WIS (-)",
         "LCK (-)"
    }
};

