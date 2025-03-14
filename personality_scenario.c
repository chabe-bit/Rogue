#include "personality_scenario.h"

const char *village_scenario[] = {
    "Silver coins drop from the hanging bag of an elderly man's pocket\n as he walks through the market. What do you do?"
    // Steal the coins openly with pride. -> Show-off
    // Steal the coins sneakily. -> Slippery Devil
    // Don't steal the coins and return them. -> Shrinking Violet
};

const char *desert_scenario[] = {
    "You carry with yourself a canteen of water with only a few sips worth left. In the harsh and unforgiving desert terrain, you come across two men stranded, where one is near death from thirst. What do you do?"
    // Finish the canteen and leave -> Thug
    // Give the man the canteen and head to town -> Daredevil
    // Carry the man to town -> Idealist
};

const char *monster_scenario[] = {
    "You are a man by day and a beast by night. You prey off human flesh and blood to survive. You come across a small and quiet village. What do you do?"
    // Kill fewer than three people -> Paragon
    // Kill three or more people, including women and the elderly, but don't kill the children -> Wimpy
    // Kill three or more people, but don't kill the women, the elderly, or children -> Spoilt Brat
    // Kill three or more poeple, including children -> Egghead
    // Kill nine or more people, but don't kill the man by the inn -> Klutz (did you know? the lore is so they accuse the man of missing people, because they're the only ones at night to see people)
};

const char *tower_scenario[] = {
    "You are disoriented and awake at the top of a seemingly endless tower. You see a staircase besides you that leads down to the unknown. What do you do?"
    // Take the stairs -> Socialite
    // Jump -> Daydreamer
};

const char *cave_scenario[] = {
    "You are near the end of your quest in saving the princess, where the entrance to her prison is in front of you. But two doors fork to the left and right, a door to take you deeper in and a door to a room of treasures. What do you do?"
    // Save the princess -> Straight Arrow 
    // Take the door to go deeper -> Mule 
    // Take the door to the room of treasures then go deeper -> Scatterbrain
    // Take the door to the room of treasures then leave -> Narcissist
    // Ignore everything and leave -> Sore Loser
};

const char *theater_scenario[] = {
    "You are a priest, and dressed nicely for the night's stage show. You walk into the theater and a man recognizes you as the town's priest. He immediately begs you to marry the women of life, of which they had only just met and he claims is love at first sight. What do you do?"
    // Ignore the man the leave -> Free spirit 
    // Say yes -> Crybaby
    // Say no -> Lone Wolf
    // Play dumb and say you're not the town's priest -> Lout
};


