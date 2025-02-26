#include "personality.h"

personality_types personality_types_state = PERSONALITY_UNUSED;

test_personality_test_results_t thug_personality = {
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
         "+ STR",
         "- AGL",
         "- VIT",
         "- WIS",
         "- LCK"
    }
};

