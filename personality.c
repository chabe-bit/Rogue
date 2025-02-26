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
         "You are what some call a thoroughly",
         "'good egg'...You are incredibly",
         "considerate, and able to see things",
         "from almost anyone's perspective...",
         "But this is both a blessing and a",
         "curse, that at times can stop you from",
         "quickly reaching a decision...After all",
         "to accept one thing is to reject another.",
         "STR (+)",
         "AGL (-)",
         "VIT (+)",
         "WIS (+)",
         "LCK (-)"
    }
};

test_personality_test_results_t happy_camper  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Happy Camper",
    .description = {
         "You are entirely free from sentiment",
         "or sadness...You are always grinning",
         "from ear to ear, and those around you",
         "often wonder what makes you so happy...",
         "But that's just who you are...",
         "You do have your own share of troubles",
         "too...But somehow everything always works",
         "itself out in the end.",
         "STR (-)",
         "AGL ( )",
         "VIT (-)",
         "WIS ( )",
         "LCK (+)"
    }
};

test_personality_test_results_t idealist  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Idealist",
    .description = {
         "You are something of an idealist...",
         "You are gifted with a strong sense of",
         "justice, and an even stronger sense of",
         "purpose...You are also kinds to others...",
         "You would like to believe those around you",
         "endear these qualities, but that is not",
         "the case, and is quite the other hand.",
         "It is not you, but others at fault.",
         "STR (+)",
         "AGL ( )",
         "VIT (+)",
         "WIS (-)",
         "LCK (-)"
    }
};

test_personality_test_results_t klutz  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Klutz",
    .description = {
         "You are careless...Something of a klutz,",
         "in fact...You did not finish the task at",
         "hand, you left survivors in your wake...",
         "No doubt you will trip over yourself in",
         "your rush to aid a village...Unless",
         "you are able to act calmly and with more",
         "consideration, you will never reach ",
         "anywhere.",
         "STR (-)",
         "AGL (+)",
         "VIT ( )",
         "WIS (-)",
         "LCK (-)"
    }
};

// Male -> Lothario
// Female -> Vamp
test_personality_test_results_t lothario  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Lothario",
    .description = {
         "You are a flirt...It is in your nature",
         "that your romantic urges are stronger",
         "than those of others...You often find",
         "yourself daydreaming about your current",
         "object of affection, doodling their name",
         "in your notebook, seeing their face in",
         "the clouds...You are obsessed with the",
         "art of love, but that is a natural thing.",
         "STR (+)",
         "AGL (+) ",
         "VIT (+)",
         "WIS (+)",
         "LCK (-)"
    }
};

test_personality_test_results_t paragon  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Paragon",
    .description = {
         "It is rare for one so powerful to be so",
         "kind...You are the paragon of virue",
         "itself. Yours is a dominant and forceful",
         "personality, but you are able to keep it",
         "masterfully in check...You are also highly",
         "compassionate, and you always place more",
         "importance of others than yourself. You are",
         "someone those can trust and depend on.",
         "STR (+)",
         "AGL (-)",
         "VIT ( )",
         "WIS (-)",
         "LCK (-)"
    }
};

test_personality_test_results_t plugger  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Plugger",
    .description = {
         "You are what one might call a plugger...",
         "Though times are hard...You are never",
         "discouraged. You simply knuckle down,",
         "and press ownard towards your goal...",
         "Yet...You remain cheerful and light-",
         "hearted...You must find someone who",
         "accepts you as you are, and does not",
         "struggle themselves.",
         "STR (+)",
         "AGL (-)",
         "VIT ( )",
         "WIS (-)",
         "LCK (-)"
    }
};

test_personality_test_results_t show_off  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Show-Off",
    .description = {
         "You are something of a show-off, are you",
         "not...? You have extremely high ideals,",
         "which is not itself bad, but you",
         "struggle when they do not match up to",
         "reality...Your worst feature is your",
         "tendency to compare yourself with others",
         "...You must learn to accept yourself for",
         "who you are, and take things as they come.",
         "STR (+)",
         "AGL (+)",
         "VIT (-)",
         "WIS (+)",
         "LCK (-)"
    }
};

test_personality_test_results_t shrinking_violet  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Shrinking Violet",
    .description = {
         "You are shy and retiring...A shrinking",
         "violet...You fully push yourself to",
         "the foreground, but always find yourself",
         "in the back of the room...And although",
         "you may dislike how others treat you,",
         "you never quite find the courage to speak",
         "out...This is not the real you...deep",
         "in your heart all you want is to shine.",
         "STR (+)",
         "AGL (-)",
         "VIT (+)",
         "WIS (+)",
         "LCK (-)"
    }
};

test_personality_test_results_t socialite  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Socialite",
    .description = {
         "Others see you as relaxed...As calm",
         "and collected...But this is only",
         "skin-deep...Your heart seethes with",
         "all mannor of conflicting feelings...",
         "You do not develop close relationships",
         "with others, but deep down it's all you",
         "crave...You must focus on opening yourself",
         "up to others, and sharing your thoughts.",
         "STR ( )",
         "AGL (-)",
         "VIT (-)",
         "WIS (+)",
         "LCK (+)"
    }
};

test_personality_test_results_t spoilt_brat  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Spoilt Brat",
    .description = {
         "You are a spoilt brat...You wish",
         "to appear strong and self-reliant,",
         "but without the constant support of",
         "others, you are helpless...In order",
         "to hide this weakness, you overstretch",
         "yourself...But that isn't necessarily",
         "bad, doing so is an effort, and effort",
         "is the resolve for betterment.",
         "STR (-)",
         "AGL ( )",
         "VIT (-)",
         "WIS (+)",
         "LCK ( )"
    }
};


test_personality_test_results_t straight_arrow  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Straight Arrow",
    .description = {
         "You are as the expression says, a",
         "'straight arrow'...You may not realize",
         "it but you are incapable of lying, and",
         "dishonesty shows instantly on your face",
         "...Your honest nature can often lead",
         "you to be swept up in events around you,",
         "and fear may overcome you at times...",
         "But strive to overcome it and be bold.",
         "STR ( )",
         "AGL (-)",
         "VIT ( )",
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

test_personality_test_results_t tough_cookie = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Tough Cookie",
    .description = {
         "You are someone who is not happy unless,",
         "they are giving their all...And you",
         "are capable of enduring great hardship",
         "without faltering or losing heart...",
         "You prefer to solve problems by yourself",
         "rather than burden others with them...",
         "But this independence gives rise to",
         "conceitedness, which can lead to mistakes.",
         "STR (+)",
         "AGL (-)",
         "VIT (+)",
         "WIS (-)",
         "LCK (-)"
    }
};

// Male -> Lothario
// Female -> Vamp
test_personality_test_results_t vamp  = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Vamp",
    .description = {
         "You are a flirt...It is in your nature",
         "that your romantic urges are stronger",
         "than those of others...You often find",
         "yourself daydreaming about your current",
         "object of affection, doodling their name",
         "in your notebook, seeing their face in",
         "the clouds...You are obsessed with the",
         "art of love, but that is a natural thing.",
         "STR (+)",
         "AGL (+) ",
         "VIT (+)",
         "WIS (+)",
         "LCK (-)"
    }
};

test_personality_test_results_t wimp = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Wimp",
    .description = {
         "You are a wimp...You may not think so,",
         "but you are afraid to act decisively",
         "...You are too cautious...You will",
         "only ever take risk is you know there",
         "is a safety net in place...However,",
         "this may not necessarily be a bad thing,",
         "you may not live a remarkable life, but",
         "it is sure to be safe and content.",
         "STR (-)",
         "AGL (-)",
         "VIT (-)",
         "WIS (+)",
         "LCK (+)"
    }
};

test_personality_test_results_t wit = {
    .x_coords = { -64, -56, -48, -40, -32, -24, -16, -8, 16, 24, 32, 40, 48 },
    .scenario = "Desert Scenario",
    .name = "Wit",
    .description = {
         "You are truly gifted...Few are as",
         "clever as you...You are an incredibly",
         "quick thinker whose mind moves from",
         "one subject to the next with ease...",
         "This makes you a gifted",
         "conversationalist, and the life and soul",
         "of the party...Which in turns leads to",
         "a tendency to be liked by all...",
         "STR (-)",
         "AGL ( )",
         "VIT ( )",
         "WIS (+)",
         "LCK (-)"
    }
};

