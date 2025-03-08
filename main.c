#include "common.h"
#include "helper_funcs.h"

#include "global_states.h"

#include "sdl_colors.h"
#include "personality.h"
#include "sound.h"
#include "name_entry.h"
#include "gui_text.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

static bool Running;

typedef struct
{
    SDL_Event e;
    SDL_Window *Window;
    SDL_Renderer *Renderer;
    SDL_Texture *Texture;
} window_t;
window_t SDLWindow;

typedef struct
{
    int X, Y, W, H;
    int TargetWidth, TargetHeight;
    SDL_Texture *TargetTexture;
} camera_t;
camera_t SDLCamera;

typedef struct
{
    int hp, atk, def, exp;
    SDL_Rect health_bar;
} stats_t;

typedef struct
{
    bool up, down, left, right;
} asset_direction_t;

typedef struct
{
    int x, y;
    int w, h;
    asset_direction_t direction;
    union 
    {
        bool is_movable;
        bool is_under_attack;
        bool is_attacking;
        bool is_defending;
        bool is_using_item;
        bool is_collidable;
    } conditions;
    SDL_Rect body;
    SDL_Rect adjacent_hitboxes[4];
    SDL_Texture *texture;
} asset_t;

typedef struct
{
    int index;
    bool is_active;
    const char *table[50]; 
} personality_test_t; 

#define PERSONALITY_TEST_QUESTIONS 50

// Personality Test inspired by DQ3 Remake
// https://game8.co/games/Dragon-Quest-3/archives/464271
const char *question_table[50] = 
{
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

void PersonalityTest_Init(personality_test_t *personality_test)
{
    personality_test->index = 1; // Starting question begins at 1
    personality_test->is_active = false;

    for (int i = 0; i < PERSONALITY_TEST_QUESTIONS; ++i)
    {
        personality_test->table[i] = question_table[i]; 
    }
}


const char *village_scenario[1] = {
    "Silver coins drop from the hanging bag of an elderly man's pocket\n as he walks through the market. What do you do?"
    // Steal the coins openly with pride. -> Show-off
    // Steal the coins sneakily. -> Slippery Devil
    // Don't steal the coins and return them. -> Shrinking Violet
};

const char *desert_scenario[1] = {
    "You carry with yourself a canteen of water with only a few sips worth left. In the harsh and unforgiving desert terrain, you come across two men stranded, where one is near death from thirst. What do you do?"
    // Finish the canteen and leave -> Thug
    // Give the man the canteen and head to town -> Daredevil
    // Carry the man to town -> Idealist
};

const char *monster_scenario[1] = {
    "You are a man by day and a beast by night. You prey off human flesh and blood to survive. You come across a small and quiet village. What do you do?"
    // Kill fewer than three people -> Paragon
    // Kill three or more people, including women and the elderly, but don't kill the children -> Wimpy
    // Kill three or more people, but don't kill the women, the elderly, or children -> Spoilt Brat
    // Kill three or more poeple, including children -> Egghead
    // Kill nine or more people, but don't kill the man by the inn -> Klutz (did you know? the lore is so they accuse the man of missing people, because they're the only ones at night to see people)
};

const char *tower_scenario[1] = {
    "You are disoriented and awake at the top of a seemingly endless tower. You see a staircase besides you that leads down to the unknown. What do you do?"
    // Take the stairs -> Socialite
    // Jump -> Daydreamer
};

const char *cave_scenario[1] = {
    "You are near the end of your quest in saving the princess, where the entrance to her prison is in front of you. But two doors fork to the left and right, a door to take you deeper in and a door to a room of treasures. What do you do?"
    // Save the princess -> Straight Arrow 
    // Take the door to go deeper -> Mule 
    // Take the door to the room of treasures then go deeper -> Scatterbrain
    // Take the door to the room of treasures then leave -> Narcissist
    // Ignore everything and leave -> Sore Loser
};

const char *forest_scenario[1] = {
    // Idea of doing a repetivie and menial task, something not fun. This is idea similar to pushing a rock, so the player will be given a set of inputs to follow and complete, something simple. Then asked to repeat. Number of times they repeat is their result.
    
};

const char *theater_scenario[1] = {
    "You are a priest, and dressed nicely for the night's stage show. You walk into the theater and a man recognizes you as the town's priest. He immediately begs you to marry the women of life, of which they had only just met and he claims is love at first sight. What do you do?"
    // Ignore the man the leave -> Free spirit 
    // Say yes -> Crybaby
    // Say no -> Lone Wolf
    // Play dumb and say you're not the town's priest -> Lout
};

const char *castle_scenario[] = {
    "You are a soldier, who overhears the newly crowned queen in her room discussing her plan to assasinate the king once he announces to open the lands' borders. Soon later that night you are pulled over by a man, who asks you two simple questions.",
    
    // (1) He asks...
    "The orders of a king must be obeyed, even if they are misguided, correct?",
    
    // (1) if yes 
    "You truly believe the orders of a king are to be set in stone?",
        // (1) if yes 
        "I see...then I know what I must do.", // Ends
        // (1) if no
        "You really think so? So you believe that misguided orders need not to be followed?",
            // (1) if yes
                // Returns to -> "I see...then I know what I must do.", // Ends
            // (1) if no
            "...So you think even the orders of a king, if mistaken, need not to be followed?",
                // (1) if yes
                "Then I suppose what one's idea of what is a mistake or correct is entirely personal...",
                    // (1) if yes
                    "So you believe it's better to do what you see fit than blindly follow orders of the king?",
                        // (1) if yes
                        // (1) if no
                        "So at times such as this, I must have faith in my king, and follow his orders?",
                            // (1) if yes
                                // Returns to the start of (1) -> You truly believe...
                            // (1) if no
                // (1) if no 
                "What silliness is this? Are you even listening to me?",
                    // (1) if yes
                    "...Your answers are wilful...But perhaps those are necessary sometimes, then I know what I must do.", // End
                    // (1) if no
    // (1) if no
    "...So you think even the orders of a king, if mistaken, need not to be followed?",
};

void CursorForItems(menu_item_t *title_screen_options, asset_t *cursor, int x_offset, int y_offset)
{
    cursor->body.x = title_screen_options->x - cursor->w - x_offset;
    cursor->body.y = title_screen_options->y + (GLYPH_HEIGHT / 2) - (cursor->h / 2) - y_offset; // Subtracting by 1 at the end is the center the cursor
}

void Cursor(asset_t *cursor, vec2_t *pos, int x_offset, int y_offset)
{
    cursor->body.x = pos->x - (cursor->w) + x_offset;
    cursor->body.y = pos->y + (GLYPH_HEIGHT / 2) - (cursor->h / 2) + y_offset; 
}

void CursorForAssets(asset_t *asset, asset_t *cursor, int x_offset, int y_offset)
{
    cursor->body.x = asset->x + x_offset;
    cursor->body.y = asset->y + (asset->body.h / 2) + y_offset;
}

void ExitOption()
{
    exit(1);
}

void LoadAsset(asset_t *asset, const char *filename)
{
    int channels;
    unsigned char *data = stbi_load(filename, &asset->w, &asset->h, &channels, STBI_default);
    if (data == NULL)
    {
        fprintf(stderr, "Failed to load files: %s\n", filename);
        return;
    }
    
    int fmt = channels == 2 ? SDL_PIXELFORMAT_RGBA8888 : SDL_PIXELFORMAT_RGBA32;
    int pitch = asset->w * channels;

    // Free later!
    asset->texture = SDL_CreateTexture(SDLWindow.Renderer, fmt, SDL_TEXTUREACCESS_STATIC, asset->w, asset->h);
    if (asset->texture == NULL)
    {
        fprintf(stderr, "Failed to create texture for sprites: %s\n", SDL_GetError());
        return;
    }

    if (SDL_UpdateTexture(asset->texture, NULL, (const void *)data, pitch) < 0)
    {
        fprintf(stderr, "Failed to update texture for sprites: %s\n", SDL_GetError());
        return;
    }

    stbi_image_free(data);  
}

// Probably the ideal way of loading an asset
asset_t IdealLoadAsset(const char *filename)
{
    asset_t asset = {0};

    int channels;
    unsigned char *data = stbi_load(filename, &asset.w, &asset.h, &channels, STBI_default);
    if (data == NULL)
    {
        fprintf(stderr, "Failed to load files: %s\n", filename);
        return asset;
    }
    
    int fmt = channels == 2 ? SDL_PIXELFORMAT_RGBA8888 : SDL_PIXELFORMAT_RGBA32;
    int pitch = asset.w * channels;

    // Free later!
    asset.texture = SDL_CreateTexture(SDLWindow.Renderer, fmt, SDL_TEXTUREACCESS_STATIC, asset.w, asset.h);
    if (asset.texture == NULL)
    {
        fprintf(stderr, "Failed to create texture for sprites: %s\n", SDL_GetError());
        return asset;
    }

    if (SDL_UpdateTexture(asset.texture, NULL, (const void *)data, pitch) < 0)
    {
        fprintf(stderr, "Failed to update texture for sprites: %s\n", SDL_GetError());
        return asset;
    }

    // Initializing asset
    asset.body.w = asset.w;
    asset.body.h = asset.h;

    stbi_image_free(data);  

    return asset;
}

bool AABB_Detection(SDL_Rect *a, SDL_Rect *b)
{
    if(a->y + a->h <= b->y) 
        return false;

    if(a->y >= b->y + b->h) 
        return false;

    if(a->x + a->w <= b->x) 
        return false;

    if(a->x >= b->x + b->w) 
        return false;

    return true;
}

void AABB_Resolution(asset_t *a, asset_t *b)
{
    if (AABB_Detection(&a->body, &b->body) && 
        (a->conditions.is_collidable && b->conditions.is_collidable))
    {
        a->body.x = a->x;
    }

    if (AABB_Detection(&a->body, &b->body) &&
        (a->conditions.is_collidable && b->conditions.is_collidable))
    {
        a->body.y = a->y;
    }
}

void 
CombatCheck(asset_t *player, asset_t* asset)
{
    if (AABB_Detection(&player->body, &asset->body) && asset->conditions.is_collidable)
    {
        player->conditions.is_attacking = true;
        asset->conditions.is_under_attack = true;
    }
}

void
CombatUpdate(asset_t *player, asset_t *asset, sound_wav_t *sound) 
{
    if (asset->conditions.is_under_attack)
    {
       /* if (asset->stats.hp <= 0)
        {
            asset->conditions.is_collidable = false;
            player->stats.exp += asset->stats.exp;
            printf("Player has earned: %d exp\n", player->stats.exp);

            SDL_DestroyTexture(asset->texture);
            asset->texture = NULL;    

        }
        Sound_PlaySFX(sound);
        asset->stats.hp -= player->stats.atk;
        printf("enemy took %d damage \n", player->stats.atk);*/
    }
}

void UpdatePlayer(asset_t *player, sound_wav_t *sound)
{
    player->x = player->body.x; 
    player->y = player->body.y; 

    if (player->conditions.is_movable)
    {
        if (player->direction.up)
        {       
            player->body.y -= player->body.h;
            Sound_PlaySFX(sound);
            player->direction.up = false;
        }

        if (player->direction.down)
        {
            player->body.y += player->body.h; 
            Sound_PlaySFX(sound);
            player->direction.down = false;
        }

        if (player->direction.left)
        {
            player->body.x -= player->body.w; 
            Sound_PlaySFX(sound);
            player->direction.left = false;
        }

        if (player->direction.right)
        {
            player->body.x += player->body.w; 
            Sound_PlaySFX(sound);
            player->direction.right = false;
        }
    }

}

void UpdateAsset(asset_t *asset, asset_t *player)
{
    asset->x = asset->body.x; 
    asset->y = asset->body.y; 

    if (asset->conditions.is_movable)
    {
        if (asset->direction.up)
        {      
            if (AABB_Detection(&asset->body, &player->body) && 
                asset->conditions.is_collidable)
            {
                asset->body.y -= asset->body.h;
            }

            asset->direction.up = false;
        }

        if (asset->direction.down)
        {
            if (AABB_Detection(&asset->body, &player->body) && 
                asset->conditions.is_collidable)
            {
                asset->body.y += asset->body.h; 
            }
                
            asset->direction.down = false;
        }

        if (asset->direction.left)
        {
            if (AABB_Detection(&asset->body, &player->body) && 
                asset->conditions.is_collidable)
            {
                asset->body.x -= asset->body.w; 
            }
           
            asset->direction.left = false;
        }

        if (asset->direction.right)
        {
            if (AABB_Detection(&asset->body, &player->body) && 
                asset->conditions.is_collidable)
            {
                asset->body.x += asset->body.w; 
            }
           
            asset->direction.right = false;
        }
    }
}

void SetAssetPosition(asset_t *asset, int x, int y)
{
    asset->body.x = x;
    asset->body.y = y;
}

void InitializeAssetToRender(asset_t *asset, int x, int y, int w, int h)
{
    asset->body.x = x;
    asset->body.y = y;
    asset->body.w = w;
    asset->body.h = h;
}

void InitializeAssetConditions(asset_t *asset)
{
    asset->conditions.is_collidable = true;
    asset->conditions.is_movable = true;
}

// Render and update any asset that moves in world space
void RenderAndUpdateAsset(asset_t *asset)
{
    asset->x = asset->body.x;
    asset->y = asset->body.y;
    asset->body.x -= SDLCamera.X;
    asset->body.y -= SDLCamera.Y;
   // SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 255, 0, 255);
   // SDL_RenderDrawRect(SDLWindow.Renderer, &asset->body);

    if (asset->texture)
    {
        SDL_SetTextureBlendMode(asset->texture, SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(SDLWindow.Renderer, asset->texture, NULL, &asset->body) < 0)
        {
            fprintf(stderr, "Failed to render asset: %s\n", SDL_GetError());
            return;
        }
    }
  
    asset->body.x = asset->x;
    asset->body.y = asset->y;
}

// Render any asset anywhere in world space
void RenderAsset(asset_t *asset, int x, int y, int w, int h)
{
    asset->body.x = x;
    asset->body.y = y;
    asset->body.x -= SDLCamera.X;
    asset->body.y -= SDLCamera.Y;

    if (asset->texture)
    {
        SDL_SetTextureBlendMode(asset->texture, SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(SDLWindow.Renderer, asset->texture, NULL, &asset->body) < 0)
        {
            fprintf(stderr, "Failed to render asset: %s\n", SDL_GetError());
            return;
        }
    }
}

// Render any asset in camera space e.g UI, HUD
void RenderAssetInCameraSpace(asset_t *asset, int x, int y, int w, int h)
{
    asset->body.x = x;
    asset->body.y = y;
    asset->body.w = w;
    asset->body.h = h;

    if (asset->texture)
    {
        SDL_SetTextureBlendMode(asset->texture, SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(SDLWindow.Renderer, asset->texture, NULL, &asset->body) < 0)
        {
            fprintf(stderr, "Failed to render asset: %s\n", SDL_GetError());
            return;
        }
    }
}

void InitAssetAdjacentHitBoxes(asset_t *asset)
{
    int TOP =   0;
    int DOWN =  1;
    int LEFT =  2;
    int RIGHT = 3;

    // We create an offset of 1px for the adj hitboxes, so the corners of the asset do not
    // touch with incoming assets. 
    // Visual:
    // [][] -> This is what it looks like when two collide, they're overlapping just by a pixel
    // even though it doesn't like it.
    // [] [] -> This is what we're achieving here, a 1px offset for adjacent assets so they aren't
    // touching, and only will touch if you're on that adj hitbox. 

    asset->adjacent_hitboxes[TOP].x = asset->body.x - SDLCamera.X; 
    asset->adjacent_hitboxes[TOP].y = (asset->body.y - 24) - SDLCamera.Y;
    asset->adjacent_hitboxes[TOP].w = 16 - 1;
    asset->adjacent_hitboxes[TOP].h = 24 - 1;

    asset->adjacent_hitboxes[DOWN].x = asset->body.x - SDLCamera.X; 
    asset->adjacent_hitboxes[DOWN].y = (asset->body.y + 24) - SDLCamera.Y;
    asset->adjacent_hitboxes[DOWN].w = 16 - 1;
    asset->adjacent_hitboxes[DOWN].h = 24 - 1;

    asset->adjacent_hitboxes[LEFT].x = (asset->body.x - 16) - SDLCamera.X; 
    asset->adjacent_hitboxes[LEFT].y = asset->body.y - SDLCamera.Y;
    asset->adjacent_hitboxes[LEFT].w = 16 - 1;
    asset->adjacent_hitboxes[LEFT].h = 24 - 1;

    asset->adjacent_hitboxes[RIGHT].x = (asset->body.x + 16) - SDLCamera.X; 
    asset->adjacent_hitboxes[RIGHT].y = asset->body.y - SDLCamera.Y;
    asset->adjacent_hitboxes[RIGHT].w = 16 - 1;
    asset->adjacent_hitboxes[RIGHT].h = 24 - 1;
}

void InitializeCamera()
{
    SDLCamera.X = -1;
    SDLCamera.Y = -1;
    SDLCamera.W = ASPECT_WIDTH;
    SDLCamera.H = ASPECT_HEIGHT;
    SDLCamera.TargetWidth = ASPECT_WIDTH;
    SDLCamera.TargetHeight = ASPECT_HEIGHT;
    
    // TODO: Free later!
    SDLCamera.TargetTexture = SDL_CreateTexture(SDLWindow.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SDLCamera.TargetWidth, SDLCamera.TargetHeight);
    if (SDLCamera.TargetTexture == NULL)
    {
        fprintf(stderr, "Failed to create camera texture: %s\n", SDL_GetError());
        return;
    }
    
    SDL_SetTextureBlendMode(SDLCamera.TargetTexture, SDL_BLENDMODE_BLEND);
}

void AttachCameraToPlayer(asset_t *player, asset_t *room)
{
    // Attach camera to player
    SDLCamera.X = (int)(player->body.x + player->body.w * 0.5f) - (SDLCamera.W * 0.5f);
    SDLCamera.Y = (int)(player->body.y + player->body.h * 0.5f) - (SDLCamera.H * 0.5f);
    
    // Clamp the camera to player
    if (SDLCamera.X < 0) 
        SDLCamera.X = 0;
    if (SDLCamera.X + SDLCamera.W > room->w)
        SDLCamera.X = room->w - SDLCamera.W;

    if (SDLCamera.Y < 0) 
        SDLCamera.Y = 0;
    if (SDLCamera.Y + SDLCamera.H > room->h)
        SDLCamera.Y = room->h - SDLCamera.H;
}

void DestroyCamera()
{
    SDL_DestroyTexture(SDLCamera.TargetTexture);
}

void DestroyAssets(asset_t *assets)
{
    SDL_DestroyTexture(assets->texture);
}



// ----------------------- EXPERIEMENTAL --------------------------
typedef struct
{
    int index; 
    bool is_active;    
    SDL_Rect scenario_box;

    struct
    {
        // Could pack with a set of bools but would might be better and cleaner using each bit as a flag to enable or disable a scenario
        u8 result; 
    } scenario;

    struct
    {
        menu_item_t options[3];
        menu_item_t monster_options[5];
    } info;

} personality_scenario_t; 

typedef enum
{
    SCENARIO_NONE       = -1,
    SCENARIO_VILLAGE    = (1 << 0),
    SCENARIO_MONSTER    = (1 << 1),
    SCENARIO_FOREST     = (1 << 2),
    SCENARIO_CAVE       = (1 << 3),
    SCENARIO_DESERT     = (1 << 4),
    SCENARIO_TOWER      = (1 << 5),
    SCENARIO_THEATER    = (1 << 6),
    SCENARIO_CASTLE     = (1 << 7),
} scenario_type;

typedef struct
{
    int index; 
    bool is_active;
    SDL_Rect box;
    test_personality_test_results_t results;
    menu_item_t options[5];
} scenario_t;

typedef struct
{
    u8 result; 
    char personality[32];
    scenario_t scenario[8];
} test_personality_scenario_t; 

void PersonalityTest_InitResults(test_personality_scenario_t *personality, test_personality_test_results_t *results, int SCENARIO, int description_size)
{
    personality->scenario[SCENARIO].results.name = results->name;
    personality->scenario[SCENARIO].results.scenario = results->scenario;

    for (int i = 0; i < description_size; ++i)
    {
        personality->scenario[SCENARIO].results.x_coords[i] = results->x_coords[i];
        personality->scenario[SCENARIO].results.description[i] = results->description[i];
        printf("description: %s\n", personality->scenario[SCENARIO].results.description[i]);
    }

}

void PersonalityTest_RenderResults(test_personality_scenario_t *personality, SDL_Texture *font_atlas, int SCENARIO, int description_size)
{
    RenderText(SDLWindow.Renderer, font_atlas,
               CENTER_TEXT_X(personality->scenario[SCENARIO].results.name, 0),
               SCREEN_CENTER_Y - 80,
               personality->scenario[SCENARIO].results.name,
               white);

    for (int i = 0; i < description_size; ++i)
    {
        RenderText(SDLWindow.Renderer, font_atlas,
                   CENTER_TEXT_X(personality->scenario[SCENARIO].results.description[i], 0),
                   SCREEN_CENTER_Y + personality->scenario[SCENARIO].results.x_coords[i],
                   personality->scenario[SCENARIO].results.description[i],
                   white);
    }
}

int Personality_GetScenarioOptionCount(test_personality_scenario_t *scenario)
{
    int option_count = 0;
  
    if (scenario->result & SCENARIO_VILLAGE)
        option_count = VILLAGE_OPTION_COUNT;

    if (scenario->result & SCENARIO_MONSTER)
        option_count = MONSTER_OPTION_COUNT;
   
    if (scenario->result & SCENARIO_FOREST)
        option_count = FOREST_INDEX;

    if (scenario->result & SCENARIO_CAVE)
        option_count = CAVE_OPTION_COUNT;
   
    if (scenario->result & SCENARIO_DESERT)
        option_count = DESERT_OPTION_COUNT;

    if (scenario->result & SCENARIO_TOWER)
        option_count = TOWER_INDEX;
   
    if (scenario->result & SCENARIO_THEATER)
        option_count = THEATER_INDEX;

    if (scenario->result & SCENARIO_CASTLE)
        option_count = CASTLE_INDEX;


    return option_count;
}
int Personality_GetScenario(test_personality_scenario_t *scenario)
{
    int result = 0;
  
    if (scenario->result & SCENARIO_VILLAGE)
        result = VILLAGE_INDEX;

    if (scenario->result & SCENARIO_MONSTER)
        result = MONSTER_INDEX;
   
    if (scenario->result & SCENARIO_FOREST)
        result = FOREST_INDEX;

    if (scenario->result & SCENARIO_CAVE)
        result = CAVE_INDEX;
   
    if (scenario->result & SCENARIO_DESERT)
        result = DESERT_INDEX;

    if (scenario->result & SCENARIO_TOWER)
        result = TOWER_INDEX;
   
    if (scenario->result & SCENARIO_THEATER)
        result = THEATER_INDEX;

    if (scenario->result & SCENARIO_CASTLE)
        result = CASTLE_INDEX;


    return result;
}


void Personality_MoveUpScenario(test_personality_scenario_t *personality)
{
    int SCENARIO = Personality_GetScenario(personality);
    int OPTIONS = Personality_GetScenarioOptionCount(personality);

    if (personality->scenario[SCENARIO].is_active)
    {
        personality->scenario[SCENARIO].index--;
        if (personality->scenario[SCENARIO].index < 0)
            personality->scenario[SCENARIO].index = OPTIONS - 1;
    }
}

void Personality_MoveDownScenario(test_personality_scenario_t *personality)
{
    int SCENARIO = Personality_GetScenario(personality);
    int OPTIONS = Personality_GetScenarioOptionCount(personality);

    if (personality->scenario[SCENARIO].is_active)
    {
        personality->scenario[SCENARIO].index++;
        if (personality->scenario[SCENARIO].index >= OPTIONS)
            personality->scenario[SCENARIO].index = 0;
    }
}

typedef struct
{
    vec2_t pos;
    char text[28];
} class_status_overview_t;


void StatOverview_Init(char *buffer, class_status_overview_t *dst, char *src)
{
    size_t total_width = 27;
    size_t append_len = strlen(src);
    size_t insert_index = total_width - append_len;

    strncpy(buffer, dst->text, total_width);
    buffer[total_width] = '\0';

    strncpy(buffer + insert_index, src, append_len);
    buffer[total_width] = '\0';

    strncpy(dst->text, buffer, total_width + 1);

    printf("hey: %s\n", dst->text);
}


// ------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        fprintf(stderr, "Failed to init sdl: %s\n", SDL_GetError());
        return 1;
    }
    
    SDLWindow.Window = SDL_CreateWindow("Game",
                                        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (SDLWindow.Window == NULL)
    {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        return 1;
    }


    SDLWindow.Renderer = SDL_CreateRenderer(SDLWindow.Window, -1, 
                                            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (SDLWindow.Renderer == NULL)
    {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        return 1;
    }

    SDLWindow.Texture = SDL_CreateTexture(SDLWindow.Renderer, SDL_TEXTUREACCESS_STATIC, SDL_PIXELFORMAT_RGBA8888, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (SDLWindow.Texture == NULL)
    {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        return 1;
    }

    // TEMP sound instances
    sound_wav_t title_screen_theme = {0};
    sound_wav_t ambience = {0};
    sound_wav_t sfx_attack = {0};
    sound_wav_t sfx_move = {0};
    sound_wav_t sfx_option = {0};

    Sound_LoadWavFile(&title_screen_theme, "assets/music/6. Simpler Times.wav", AUDIO_TYPE_MUSIC);
    Sound_LoadWavFile(&ambience, "assets/music/8. Temple of Tomb.wav", AUDIO_TYPE_MUSIC);
    Sound_LoadWavFile(&sfx_attack, "assets/sfx/attack.wav", AUDIO_TYPE_SFX);
    Sound_LoadWavFile(&sfx_move, "assets/sfx/move.wav", AUDIO_TYPE_SFX);
    Sound_LoadWavFile(&sfx_option, "assets/sfx/option.wav", AUDIO_TYPE_SFX);


    sound_music_t music_volume[MUSIC_FILE_COUNT] = {0};
    Sound_InitMusic(music_volume, music_files); 

    sound_sfx_t sfx_volume[SFX_FILE_COUNT] = {0};
    Sound_InitSFX(sfx_volume, sfx_files); 

    sound_master_volume_t master_volume = {0};
    Sound_InitMaster(&master_volume, music_volume, sfx_volume);

    personality_test_t personality_test = {0};
    PersonalityTest_Init(&personality_test);



    bool updated_sound = false;

    bool is_title_screen = true;
    bool is_new_game = false;
    bool is_class_select = false;
    bool is_confirmation = false;
    bool is_load_game = false;
    
    bool is_settings = false;
    bool is_settings_master_volume = false;
    bool settings_test = false; 
    
    bool is_exit = false;
    bool is_game_running = false;
    bool class_has_been_selected = false;
    bool is_personality_test = false;
    bool question_confirmation = false;

    bool is_name_submission = false;
    bool is_class_overview_screen = false;


    asset_t blank_screen_asset = {0};
    LoadAsset(&blank_screen_asset, "assets/blank_screen.png");
    InitializeAssetToRender(&blank_screen_asset, 0, 0, ASPECT_WIDTH, ASPECT_HEIGHT);

    asset_t title_screen_asset = {0};
    LoadAsset(&title_screen_asset, "assets/blank_screen.png");
    InitializeAssetToRender(&title_screen_asset, 0, 0, ASPECT_WIDTH, ASPECT_HEIGHT);
    
    asset_t new_game_menu = {0};
    LoadAsset(&new_game_menu, "assets/new_game_screen.png");
    InitializeAssetToRender(&new_game_menu, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    InitializeCamera();
    int CameraX = (int)(-SDLCamera.X);
    int CameraY = (int)(-SDLCamera.Y);


    // TODO(ben): Make an array of rooms for future levels and for the camera switch since it's also tied to the room
    int room_select = 1;
    asset_t room_asset[2] = {0};
    LoadAsset(&room_asset[0], "assets/map1.png");
    LoadAsset(&room_asset[1], "assets/map2.png");
    InitializeAssetToRender(&room_asset[0], CameraX, CameraY, room_asset[0].w, room_asset[0].h);
    InitializeAssetToRender(&room_asset[1], CameraX, CameraY, room_asset[1].w, room_asset[1].h);
    
    asset_t walls[4] = {0};
    walls[0].body.x = 0;
    walls[0].body.y = 0;
    walls[0].body.w = room_asset[0].w;
    walls[0].body.h = 24;
    walls[0].conditions.is_collidable = true;

    walls[1].body.x = 0;
    walls[1].body.y = room_asset[0].h - 24;
    walls[1].body.w = room_asset[0].w;
    walls[1].body.h = 24;
    walls[1].conditions.is_collidable = true;

    walls[2].body.x = 0;
    walls[2].body.y = 0;
    walls[2].body.w = 16;
    walls[2].body.h = room_asset[0].h;
    walls[2].conditions.is_collidable = true;

    walls[3].body.x = room_asset[0].w - 16;
    walls[3].body.y = 0; 
    walls[3].body.w = 16;
    walls[3].body.h = room_asset[0].h;
    walls[3].conditions.is_collidable = true;


    asset_t down_stairs_asset = {0};
    LoadAsset(&down_stairs_asset, "assets/down_stairs.png");
    InitializeAssetToRender(&down_stairs_asset, 14 * 16, 2 * 24, down_stairs_asset.w, down_stairs_asset.h);


    asset_t player_asset = {0};
    stats_t player_stats = {0};
    player_stats.hp = 11;
    player_stats.atk = 8;
    player_stats.def = 9;
    player_stats.exp = 0;
    LoadAsset(&player_asset, "assets/sprites/knight.png");
    InitializeAssetToRender(&player_asset, 10 * 16, 16 * 24, player_asset.w, player_asset.h);
    
    player_asset.conditions.is_collidable = true;
    player_asset.conditions.is_movable = true;


    const char *enemy_filenames[2] = {
        "assets/sprites/enemy1.png",
        "assets/sprites/enemy2.png"
    };
    
    asset_t enemy_arr[3];
    asset_t enemy_arr2[3];
    for (int i = 0; i < ArraySize(enemy_arr); ++i)
    {
        LoadAsset(&enemy_arr[i], enemy_filenames[0]);
        LoadAsset(&enemy_arr2[i], enemy_filenames[1]);
    }

    InitializeAssetToRender(&enemy_arr[0], 7 * 16, 16 * 24, enemy_arr[0].w, enemy_arr[0].h);
    InitializeAssetToRender(&enemy_arr[1], 8 * 16, 15 * 24, enemy_arr[1].w, enemy_arr[1].h);
    InitializeAssetToRender(&enemy_arr[2], 9 * 16, 14 * 24, enemy_arr[2].w, enemy_arr[2].h);

    InitializeAssetToRender(&enemy_arr2[0], 7 * 16, 19 * 24, enemy_arr2[0].w, enemy_arr2[0].h);
    InitializeAssetToRender(&enemy_arr2[1], 8 * 16, 18 * 24, enemy_arr2[1].w, enemy_arr2[1].h);
    InitializeAssetToRender(&enemy_arr2[2], 9 * 16, 17 * 24, enemy_arr2[2].w, enemy_arr2[2].h);


    asset_t forest_scenario_map = {0};
    LoadAsset(&forest_scenario_map, "assets/forest_scenario.png");
    InitializeAssetToRender(&forest_scenario_map, CameraX, CameraY, forest_scenario_map.w, forest_scenario_map.h);
    
    asset_t oldman_asset = {0};
    LoadAsset(&oldman_asset, "assets/sprites/oldman.png");
    InitializeAssetToRender(&oldman_asset, CameraX, CameraY, oldman_asset.w, oldman_asset.h);
    oldman_asset.conditions.is_collidable = true;

    int boulder_count = 0;
    asset_t boulder_asset = {0};
    LoadAsset(&boulder_asset, "assets/sprites/boulder.png");
    InitializeAssetToRender(&boulder_asset, CameraX, CameraY, boulder_asset.w, boulder_asset.h);
    boulder_asset.conditions.is_collidable = true;
    boulder_asset.conditions.is_movable = true;
 
    asset_t boulder_wall_asset[5] = {0};
    for (int i = 0; i < ArraySize(boulder_wall_asset); ++i)
    {
        LoadAsset(&boulder_wall_asset[i], "assets/sprites/boulder.png");
        InitializeAssetToRender(&boulder_wall_asset[i], CameraX, CameraY, boulder_wall_asset[i].w, boulder_wall_asset[i].h);
        boulder_wall_asset[i].conditions.is_collidable = true;

    }

    asset_t forest_scenario_walls[2] = {0};
    forest_scenario_walls[0].body.x = 0;
    forest_scenario_walls[0].body.y = 240 - (48 + 24);
    forest_scenario_walls[0].body.w = forest_scenario_map.w;
    forest_scenario_walls[0].body.h = 24;
    forest_scenario_walls[0].conditions.is_collidable = true;

    forest_scenario_walls[1].body.x = 0;
    forest_scenario_walls[1].body.y = 240 - (48 - 24 - 24 - 24);
    forest_scenario_walls[1].body.w = forest_scenario_map.w;
    forest_scenario_walls[1].body.h = 24;
    forest_scenario_walls[1].conditions.is_collidable = true;
  
    bool player_is_out_of_bounds = false;
    asset_t forest_out_of_bounds[2] = {0};
    forest_out_of_bounds[0].body.x = -16;
    forest_out_of_bounds[0].body.y = 0;
    forest_out_of_bounds[0].body.w = 16;
    forest_out_of_bounds[0].body.h = forest_scenario_map.h;

    forest_out_of_bounds[1].body.x = 400 + (16 * 4);
    forest_out_of_bounds[1].body.y = 0;
    forest_out_of_bounds[1].body.w = 16;
    forest_out_of_bounds[1].body.h = forest_scenario_map.h;

    asset_t forest_scenario_finish_line = {0};
    forest_scenario_finish_line.body.x = 256 + (16 * 9);
    forest_scenario_finish_line.body.y = 240 - (48);
    forest_scenario_finish_line.body.w = 16;
    forest_scenario_finish_line.body.h = 24 * 3;
    forest_scenario_finish_line.conditions.is_collidable = true;

    asset_t dialogue_box = {0};
    dialogue_box.body.x = 16; //256 + (16 * 8);
    dialogue_box.body.y = 150; //240 - (48);
    dialogue_box.body.w = 224;
    dialogue_box.body.h = 64;

    SDL_Texture *font_atlas = CreateFontAtlas(SDLWindow.Renderer);
    if (!font_atlas)
    {
        SDL_DestroyRenderer(SDLWindow.Renderer);
        SDL_DestroyWindow(SDLWindow.Window);
        SDL_Quit();
        return 1;
    }


    int option_index = 0;
    menu_item_t title_screen_options[4] = {
        { "New Game", CENTER_TEXT_X("New Game", 0), SCREEN_CENTER_Y},
        { "Continue Game", CENTER_TEXT_X("Continue Game", 0), SCREEN_CENTER_Y + 16 },
        { "Settings", CENTER_TEXT_X("Settings", 0), SCREEN_CENTER_Y + 32 },
        { "Exit", CENTER_TEXT_X("Exit", 0), SCREEN_CENTER_Y + 48 },
    };

    asset_t up_cursor_asset = {0};
    LoadAsset(&up_cursor_asset, "assets/ui/up_cursor.png");
    InitializeAssetToRender(&up_cursor_asset, 0, 0, up_cursor_asset.w, up_cursor_asset.h);
    
    asset_t down_cursor_asset = {0};
    LoadAsset(&down_cursor_asset, "assets/ui/down_cursor.png");
    InitializeAssetToRender(&down_cursor_asset, 0, 0, down_cursor_asset.w, down_cursor_asset.h);
    
    asset_t right_cursor_asset = {0};
    LoadAsset(&right_cursor_asset, "assets/ui/right_cursor.png");
    InitializeAssetToRender(&right_cursor_asset, 0, 0, right_cursor_asset.w, right_cursor_asset.h);
       
    asset_t left_cursor_asset = {0};
    LoadAsset(&left_cursor_asset, "assets/ui/left_cursor.png");
    InitializeAssetToRender(&left_cursor_asset, 0, 0, left_cursor_asset.w, left_cursor_asset.h);
       
    asset_t new_game_asset = {0};
    LoadAsset(&new_game_asset, "assets/new_game_screen.png");
    InitializeAssetToRender(&new_game_asset, 0, 0, new_game_asset.w, new_game_asset.h); 

    asset_t dialogue_box_asset = {0};
    LoadAsset(&dialogue_box_asset, "assets/ui/dialogue_box.png");
    InitializeAssetToRender(&dialogue_box_asset, 0, 0, dialogue_box_asset.w, dialogue_box_asset.h);
   
    asset_t gold_coin_count_bg_asset = {0};
    LoadAsset(&gold_coin_count_bg_asset, "assets/ui/gold_coin_count_background.png");
    InitializeAssetToRender(&gold_coin_count_bg_asset, 0, 0, gold_coin_count_bg_asset.w, gold_coin_count_bg_asset.h);

    typedef struct class_select_t
    {       
        asset_t asset;
        const char *name;
        const char *description;
    } class_select_t;

    const char *class_files[4] = {
        "assets/sprites/knight.png",
        "assets/sprites/paladin.png",
        "assets/sprites/wizard.png",
        "assets/sprites/archer.png"
    };
 
    const char *class_names[4] = {
        "Knight",
        "Paladin",
        "Wizard",
        "Archer"
    };
    
    const char *class_description[4] = {
        "Wield the power of the sword and shield. Prioritizes in both offensive and defensive combat.",
        "Wield the power of the oath. Prioritizes in defending and buffing allies.",
        "Wield the power of magic. Priortizes in offensive magical spells.",
        "Wield the power of the bow. Prioritizes in speed and accuracy."
    };

    typedef struct
    {
        // overall instance
        int index;
        bool is_active;
        SDL_Rect description_box; 
        asset_t cursor;

        // multiple instances
        struct 
        {
            asset_t asset;
            const char *name;
            const char *description;
        } info[4];

    } character_creation_screen_t;

    character_creation_screen_t character_creation_screen = {0};
    for (int i = 0; i < ArraySize(character_creation_screen.info); ++i)
    {
        LoadAsset(&character_creation_screen.info[i].asset, class_files[i]);
        character_creation_screen.info[i].name = class_names[i];
        character_creation_screen.info[i].description = class_description[i];
        character_creation_screen.info[i].asset.conditions.is_collidable = true;
        character_creation_screen.info[i].asset.conditions.is_movable = true;
    }
    
    InitializeAssetToRender(&character_creation_screen.info[0].asset, SCREEN_CENTER_X - 96, SCREEN_CENTER_Y, 
                            character_creation_screen.info[0].asset.w, character_creation_screen.info[0].asset.h);
    InitializeAssetToRender(&character_creation_screen.info[1].asset, SCREEN_CENTER_X - 36, SCREEN_CENTER_Y, 
                            character_creation_screen.info[1].asset.w, character_creation_screen.info[1].asset.h);  
    InitializeAssetToRender(&character_creation_screen.info[2].asset, SCREEN_CENTER_X + 28, SCREEN_CENTER_Y, 
                            character_creation_screen.info[2].asset.w, character_creation_screen.info[2].asset.h);
    InitializeAssetToRender(&character_creation_screen.info[3].asset, SCREEN_CENTER_X + 80, SCREEN_CENTER_Y, 
                            character_creation_screen.info[3].asset.w, character_creation_screen.info[3].asset.h);

    character_creation_screen.description_box.x = SCREEN_CENTER_X - 112; // 16px padding from start of X
    character_creation_screen.description_box.y = SCREEN_CENTER_Y + 48;
    character_creation_screen.description_box.w = 224; // 16px padding from end of X
    character_creation_screen.description_box.h = 64;

    ////////////////////////////////////////////////////////////////////////////

    const char *stat_options_name[3] = {
        // Have to play with the spacing to center the cursor from below to text
        "Personality\n   Test",
        "  Preset\n  ",
        "  Manual\nAllocation",
    };
    
    const char *stat_options_description[3] = {
        "A personality test to determine your class's point allocation.",
        "A preset allocation of your selected class.",
        "Manually allocate your class's points.",
    };

   
    typedef struct
    {
        // overall instance
        int index;
        bool is_active;
        SDL_Rect description_box; 

        // multiple instances
        struct 
        {
            asset_t asset;
            const char *name;
            const char *description;
        } info[3]; // personality test, manual allocation, presets, randomizer?

    } character_allocation_select_screen_t; 

    character_allocation_select_screen_t character_allocation_select_screen = {0};
    for (int i = 0; i < ArraySize(character_allocation_select_screen.info); ++i)
    {
        // Room for loading an icon per option
        character_allocation_select_screen.info[i].name = stat_options_name[i];
        character_allocation_select_screen.info[i].description = stat_options_description[i];
    }

    InitializeAssetToRender(&character_allocation_select_screen.info[0].asset, SCREEN_CENTER_X - 104, SCREEN_CENTER_Y, 
                            character_allocation_select_screen.info[0].asset.w, character_allocation_select_screen.info[0].asset.h);
    InitializeAssetToRender(&character_allocation_select_screen.info[1].asset, SCREEN_CENTER_X - 28, SCREEN_CENTER_Y, 
                            character_allocation_select_screen.info[1].asset.w, character_allocation_select_screen.info[1].asset.h);
    InitializeAssetToRender(&character_allocation_select_screen.info[2].asset, SCREEN_CENTER_X + 42, SCREEN_CENTER_Y, 
                            character_allocation_select_screen.info[2].asset.w, character_allocation_select_screen.info[2].asset.h);

    character_allocation_select_screen.description_box.x = SCREEN_CENTER_X - 112; // 16px padding from start of X
    character_allocation_select_screen.description_box.y = SCREEN_CENTER_Y + 48;
    character_allocation_select_screen.description_box.w = 224; // 16px padding from end of X
    character_allocation_select_screen.description_box.h = 64;

    //////////////////////////////////////////////////////////////////////////////////////////

    typedef struct
    {
        int index;
        bool is_active;
        SDL_Rect box;
        SDL_Rect box_border;

        struct
        {
            menu_item_t buttons[2];
        } info;

    } confirmation_buttons_t;


    confirmation_buttons_t confirmation = {0};
    confirmation.info.buttons[0].text = "Yes";
    confirmation.info.buttons[0].x = SCREEN_CENTER_X - 4;
    confirmation.info.buttons[0].y = SCREEN_CENTER_Y;

    confirmation.info.buttons[1].text = "No";
    confirmation.info.buttons[1].x = SCREEN_CENTER_X - 2;
    confirmation.info.buttons[1].y = SCREEN_CENTER_Y + 16;

    confirmation.box.x = SCREEN_CENTER_X - 20; // x render pos
    confirmation.box.y = SCREEN_CENTER_Y - 20; // y render pos
    confirmation.box.w = 48;
    confirmation.box.h = 56;

    confirmation.box_border.x = SCREEN_CENTER_X - 21; // shift the x render pos 1 pixel back
    confirmation.box_border.y = SCREEN_CENTER_Y - 21; // shift the y render pos 1 pixel up
    confirmation.box_border.w = 50; // padding of 1px per side for x 
    confirmation.box_border.h = 58; // padding of 1px per side for y


    personality_scenario_t personality_scenario[1] = {0};
    const char *scenario_name = '\0';

    test_personality_scenario_t test_scenarios = {0};
    test_scenarios.scenario[VILLAGE_INDEX].options[0].text = "Steal the coins openly with pride.";
    test_scenarios.scenario[VILLAGE_INDEX].options[0].x = CENTER_TEXT_X(test_scenarios.scenario[VILLAGE_INDEX].options[0].text, 0);
    test_scenarios.scenario[VILLAGE_INDEX].options[0].y = SCREEN_CENTER_Y - 16;
    
    test_scenarios.scenario[VILLAGE_INDEX].options[1].text = "Steal the coins sneakily.";
    test_scenarios.scenario[VILLAGE_INDEX].options[1].x = CENTER_TEXT_X(test_scenarios.scenario[VILLAGE_INDEX].options[1].text, 0);
    test_scenarios.scenario[VILLAGE_INDEX].options[1].y = SCREEN_CENTER_Y + 0;

    test_scenarios.scenario[VILLAGE_INDEX].options[2].text = "Don't steal the coins and return them.";
    test_scenarios.scenario[VILLAGE_INDEX].options[2].x = CENTER_TEXT_X(test_scenarios.scenario[VILLAGE_INDEX].options[2].text, 0);
    test_scenarios.scenario[VILLAGE_INDEX].options[2].y = SCREEN_CENTER_Y + 16;

    test_scenarios.scenario[MONSTER_INDEX].options[0].text = "Kill fewer than three people.";
    test_scenarios.scenario[MONSTER_INDEX].options[0].x = CENTER_TEXT_X(test_scenarios.scenario[MONSTER_INDEX].options[0].text, 0);
    test_scenarios.scenario[MONSTER_INDEX].options[0].y = SCREEN_CENTER_Y - 16;

    test_scenarios.scenario[MONSTER_INDEX].options[1].text = "Kill three or more people, \nincluding women and the elderly, \nbut don't kill the children.";
    test_scenarios.scenario[MONSTER_INDEX].options[1].x = CENTER_TEXT_X(test_scenarios.scenario[MONSTER_INDEX].options[1].text, 176);
    test_scenarios.scenario[MONSTER_INDEX].options[1].y = SCREEN_CENTER_Y + 0;

    test_scenarios.scenario[MONSTER_INDEX].options[2].text = "Kill three or more people, \nbut don't kill the women, \nthe elderly, or children.";
    test_scenarios.scenario[MONSTER_INDEX].options[2].x = CENTER_TEXT_X(test_scenarios.scenario[MONSTER_INDEX].options[2].text, 164);
    test_scenarios.scenario[MONSTER_INDEX].options[2].y = SCREEN_CENTER_Y + 32;

    test_scenarios.scenario[MONSTER_INDEX].options[3].text = "Kill three or more poeple, \nincluding children.";
    test_scenarios.scenario[MONSTER_INDEX].options[3].x = CENTER_TEXT_X(test_scenarios.scenario[MONSTER_INDEX].options[3].text, 64);
    test_scenarios.scenario[MONSTER_INDEX].options[3].y = SCREEN_CENTER_Y + 64;

    test_scenarios.scenario[MONSTER_INDEX].options[4].text = "Kill nine or more people, but \ndon't kill the man by the inn.";
    test_scenarios.scenario[MONSTER_INDEX].options[4].x = CENTER_TEXT_X(test_scenarios.scenario[MONSTER_INDEX].options[4].text, 88);
    test_scenarios.scenario[MONSTER_INDEX].options[4].y = SCREEN_CENTER_Y + 88;

    test_scenarios.scenario[CAVE_INDEX].options[0].text = "Save the princess.";
    test_scenarios.scenario[CAVE_INDEX].options[0].x = CENTER_TEXT_X(test_scenarios.scenario[CAVE_INDEX].options[0].text, 0);
    test_scenarios.scenario[CAVE_INDEX].options[0].y = SCREEN_CENTER_Y - 16;

    test_scenarios.scenario[CAVE_INDEX].options[1].text = "Take the door to go deeper";
    test_scenarios.scenario[CAVE_INDEX].options[1].x = CENTER_TEXT_X(test_scenarios.scenario[CAVE_INDEX].options[1].text, 0);
    test_scenarios.scenario[CAVE_INDEX].options[1].y = SCREEN_CENTER_Y + 0;

    test_scenarios.scenario[CAVE_INDEX].options[2].text = "Take the door to the room\nof treasures then go deeper.";
    test_scenarios.scenario[CAVE_INDEX].options[2].x = CENTER_TEXT_X(test_scenarios.scenario[CAVE_INDEX].options[2].text, 80);
    test_scenarios.scenario[CAVE_INDEX].options[2].y = SCREEN_CENTER_Y + 18;

    test_scenarios.scenario[CAVE_INDEX].options[3].text = "Take the door to the room\nof treasures then leave.";
    test_scenarios.scenario[CAVE_INDEX].options[3].x = CENTER_TEXT_X(test_scenarios.scenario[CAVE_INDEX].options[3].text, 72);
    test_scenarios.scenario[CAVE_INDEX].options[3].y = SCREEN_CENTER_Y + 42;
   
    test_scenarios.scenario[CAVE_INDEX].options[4].text = "Ignore everything and leave.";
    test_scenarios.scenario[CAVE_INDEX].options[4].x = CENTER_TEXT_X(test_scenarios.scenario[CAVE_INDEX].options[4].text, 0);
    test_scenarios.scenario[CAVE_INDEX].options[4].y = SCREEN_CENTER_Y + 64;

    test_scenarios.scenario[DESERT_INDEX].options[0].text = "Finish the canteen and leave.";
    test_scenarios.scenario[DESERT_INDEX].options[0].x = CENTER_TEXT_X(test_scenarios.scenario[DESERT_INDEX].options[0].text, 0);
    test_scenarios.scenario[DESERT_INDEX].options[0].y = SCREEN_CENTER_Y - 16;
    
    test_scenarios.scenario[DESERT_INDEX].options[1].text = "Give the man the canteen\n   and head to town.";
    test_scenarios.scenario[DESERT_INDEX].options[1].x = CENTER_TEXT_X(test_scenarios.scenario[DESERT_INDEX].options[1].text, 56);
    test_scenarios.scenario[DESERT_INDEX].options[1].y = SCREEN_CENTER_Y + 0;

    test_scenarios.scenario[DESERT_INDEX].options[2].text = "Carry the man to town.";
    test_scenarios.scenario[DESERT_INDEX].options[2].x = CENTER_TEXT_X(test_scenarios.scenario[DESERT_INDEX].options[2].text, 0);
    test_scenarios.scenario[DESERT_INDEX].options[2].y = SCREEN_CENTER_Y + 24;

    test_scenarios.scenario[TOWER_INDEX].options[0].text = "Take the stairs.";
    test_scenarios.scenario[TOWER_INDEX].options[0].x = CENTER_TEXT_X(test_scenarios.scenario[TOWER_INDEX].options[0].text, 0);
    test_scenarios.scenario[TOWER_INDEX].options[0].y = SCREEN_CENTER_Y - 16;
    
    test_scenarios.scenario[TOWER_INDEX].options[1].text = "Jump off the tower.";
    test_scenarios.scenario[TOWER_INDEX].options[1].x = CENTER_TEXT_X(test_scenarios.scenario[TOWER_INDEX].options[1].text, 0);
    test_scenarios.scenario[TOWER_INDEX].options[1].y = SCREEN_CENTER_Y + 0;
    
    test_scenarios.scenario[THEATER_INDEX].options[0].text = "Ignore the man and leave.";
    test_scenarios.scenario[THEATER_INDEX].options[0].x = CENTER_TEXT_X(test_scenarios.scenario[THEATER_INDEX].options[0].text, 0);
    test_scenarios.scenario[THEATER_INDEX].options[0].y = SCREEN_CENTER_Y - 16;

    test_scenarios.scenario[THEATER_INDEX].options[1].text = "Say yes";
    test_scenarios.scenario[THEATER_INDEX].options[1].x = CENTER_TEXT_X(test_scenarios.scenario[THEATER_INDEX].options[1].text, 0);
    test_scenarios.scenario[THEATER_INDEX].options[1].y = SCREEN_CENTER_Y + 0;

    test_scenarios.scenario[THEATER_INDEX].options[2].text = "Say no";
    test_scenarios.scenario[THEATER_INDEX].options[2].x = CENTER_TEXT_X(test_scenarios.scenario[THEATER_INDEX].options[2].text, 0);
    test_scenarios.scenario[THEATER_INDEX].options[2].y = SCREEN_CENTER_Y + 18;

    test_scenarios.scenario[THEATER_INDEX].options[3].text = "Play dumb and tell him he\n got the wrong person.";
    test_scenarios.scenario[THEATER_INDEX].options[3].x = CENTER_TEXT_X(test_scenarios.scenario[THEATER_INDEX].options[3].text, 72);
    test_scenarios.scenario[THEATER_INDEX].options[3].y = SCREEN_CENTER_Y + 42;

    bool is_village = false;
    bool is_monster = false;
    bool is_forest = false;
    bool is_cave = false;
    bool is_desert = false;
    bool is_tower = false;
    bool is_theater = false;
    bool is_castle = false;



    //////////////////////////////////////////////////////////////////////////////////////


    int button_select = 0;
    int confirmation_select = 0;
    menu_item_t confirmation_buttons[2] = {
        { "Yes", SCREEN_CENTER_X - 2, SCREEN_CENTER_Y },
        { "No", SCREEN_CENTER_X, SCREEN_CENTER_Y + 16 },
    };
  
    const char *back_or_next_cursor_files[2] = {
        "assets/left_cursor.png",
        "assets/right_cursor.png"
    };

    sound_settings_t sound_settings = {0};
    Sound_InitSettings(&sound_settings); 

    sound_volume_controller_t volume_controller[3] = {0};
    Sound_InitVolumeBar(&sound_settings, volume_controller, VOLUME_CONTROLLER_COUNT); 

    sound_volume_controller_t test_volume_controller = {0};
    Sound_TestInitVolumeBar(&sound_settings, &test_volume_controller, VOLUME_CONTROLLER_COUNT);


    ////////////// Enter name grid 
    name_entry_t name_entry = {0};
    NameEntry_Init(&name_entry);


    /////////////////////////////////////////////////////////////////////////
    // Time to store player data from character creation now that we have the creation process done
   
    /*typedef struct base_abilities_t
    {
        struct {   
            int strength, dexterity, constitution;
        } physical;

        struct {
            int intelligense, wisdom, charisma;
        } mental;
    } base_abilities_t;

    typedef struct stats_t
    {
        int hp, atk, def, exp;
        SDL_Rect health_bar;
    } stats_t;
    */

    typedef struct
    {
        bool is_compatible;
        struct
        {
                // TODO: Load asset
            int id; // do we need?
            int defense;
            char name[10];
            char description[32];
        } helm[2];

        struct
        {
            int id; // do we need?
            int defense;
            char name[10];
            char description[32];
        } chest;

        struct
        {
            int id; // do we need?
            int attack;
            char name[10];
            char description[32];
        } main_hand;

        struct
        {
            int id; // do we need?
            int defense;
            char name[10];
            char description[32];
        } off_hand;

        struct
        {
            int id; // do we need?
            int defense;
            char name[10];
            char description[32];
        } accessories[2];

    } equipment_item_t;

    // Create some equipments
    equipment_item_t equipment_items = {0};
    equipment_items.helm[0].id = 0;
    equipment_items.helm[0].defense = 8;
    PushString(equipment_items.helm[0].name, "Hard cap");
    PushString(equipment_items.helm[0].description, "Hardly knew her");

    printf("helm name: %s\n", equipment_items.helm[0].name);


    // Base stats of each class from here: https://dragon-quest.org/wiki/List_of_vocations_in_Dragon_Quest_III#The_vocations
    typedef struct
    {
        int index; // Iterate over for player to select

        int strength; // Determines physical dmg
        int resilience; // Determines damage received
        int agility; // Determines who acts first in battle - probably change to evasiveness
        int stamina; // Determines HP value and scaling
        int wisdom; // Determines magic dmg and MP storage
        int luck; // Determines crit chance

        // Maybe this is seperate to just the player and not class?
        int max_hp; // Max HP 
        int hp; // Current HP
        int max_mp; // Max MP
        int mp; // Current MP
        int attack; // Combination of your strength and weapon damage
        int defense; // 
    } class_base_stats_t;
    
    // Base stats should be set to a baseline of let's say 5 by default, so that depending on the method the player chooses to allocate their points,
    // that is where it's initialized. 
    // Ex: Personality will default the selected class's stats of what a wizard would be, then assign your personality.
    // But manual allocation you're shown the baseline of "5" per stat, where they can choose what to allocate to the choose a personality.
    // Preset would be defaulting the class's stat then choosing a personality.
#define KNIGHT_ID   0
#define PALADIN_ID  1
#define WIZARD_ID   2
#define ARCHER_ID   3

    class_base_stats_t class_base_stats[4] = {0};
    class_base_stats[KNIGHT_ID].strength = 11;
    class_base_stats[KNIGHT_ID].resilience = 11;
    class_base_stats[KNIGHT_ID].agility = 6;
    class_base_stats[KNIGHT_ID].stamina = 13;
    class_base_stats[KNIGHT_ID].wisdom = 3;
    class_base_stats[KNIGHT_ID].luck = 4;
    class_base_stats[KNIGHT_ID].max_hp = (3 * class_base_stats[KNIGHT_ID].stamina) + 5; // formula from DQ3 -> HP = [3 * VIT / 2] + 5
    class_base_stats[KNIGHT_ID].hp = class_base_stats[KNIGHT_ID].max_hp; // 
    class_base_stats[KNIGHT_ID].max_mp = class_base_stats[KNIGHT_ID].wisdom; // MP == Wisdom
    class_base_stats[KNIGHT_ID].hp = class_base_stats[KNIGHT_ID].max_mp; // 

    class_base_stats[PALADIN_ID].strength = 7;
    class_base_stats[PALADIN_ID].resilience = 14;
    class_base_stats[PALADIN_ID].agility = 5;
    class_base_stats[PALADIN_ID].stamina = 17;
    class_base_stats[PALADIN_ID].wisdom = 8;
    class_base_stats[PALADIN_ID].luck = 1;
    class_base_stats[PALADIN_ID].max_hp = (3 * class_base_stats[PALADIN_ID].stamina) + 5; // formula from DQ3 -> HP = [3 * VIT / 2] + 5
    class_base_stats[PALADIN_ID].hp = class_base_stats[PALADIN_ID].max_hp; // 
    class_base_stats[PALADIN_ID].max_mp = class_base_stats[PALADIN_ID].wisdom; // MP == Wisdom
    class_base_stats[PALADIN_ID].hp = class_base_stats[PALADIN_ID].max_mp; // 

    class_base_stats[WIZARD_ID].strength = 4;
    class_base_stats[WIZARD_ID].resilience = 8;
    class_base_stats[WIZARD_ID].agility = 7;
    class_base_stats[WIZARD_ID].stamina = 8;
    class_base_stats[WIZARD_ID].wisdom = 18;
    class_base_stats[WIZARD_ID].luck = 8;
    class_base_stats[WIZARD_ID].max_hp = (3 * class_base_stats[WIZARD_ID].stamina) + 5; // formula from DQ3 -> HP = [3 * VIT / 2] + 5
    class_base_stats[WIZARD_ID].hp = class_base_stats[WIZARD_ID].max_hp; // 
    class_base_stats[WIZARD_ID].max_mp = class_base_stats[WIZARD_ID].wisdom; // MP == Wisdom
    class_base_stats[WIZARD_ID].hp = class_base_stats[WIZARD_ID].max_mp; // 

    class_base_stats[ARCHER_ID].strength = 5;
    class_base_stats[ARCHER_ID].resilience = 8;
    class_base_stats[ARCHER_ID].agility = 15;
    class_base_stats[ARCHER_ID].stamina = 9;
    class_base_stats[ARCHER_ID].wisdom = 4;
    class_base_stats[ARCHER_ID].luck = 9;
    class_base_stats[ARCHER_ID].max_hp = (3 * class_base_stats[ARCHER_ID].stamina) + 5; // formula from DQ3 -> HP = [3 * VIT / 2] + 5
    class_base_stats[ARCHER_ID].hp = class_base_stats[ARCHER_ID].max_hp; // 
    class_base_stats[ARCHER_ID].max_mp = class_base_stats[ARCHER_ID].wisdom; // MP == Wisdom
    class_base_stats[ARCHER_ID].hp = class_base_stats[ARCHER_ID].max_mp; // 

    printf("Max HP: %d\n", class_base_stats[KNIGHT_ID].max_hp);

       
    // Stat growth:
    // Baseline -> Value that if the gain value exceeds it, the character loses their normal gain rate and instead rolls a 50/50 chance to gain 
    // +1 or +0.
    // Think of it like a bar graph per stat attribute where there's a limit for each for each class, the 50/50 chance roll is simply there to
    // balance in a way that from level 1 to 50, one's HP growth will be consistent in gaining +4 or so per level until 50, but because they excel in
    // magic, their INT will grow at a large rate but cap quickly.

    typedef struct
    {
        struct {
            float baseline;
            float growth_per_level[20];
        } strength;

        struct {
            float baseline;
            float growth_per_level[20];
        } agility;
    
        struct {
            float baseline;
            float growth_per_level[20];
        } stamina;

        struct {
            float baseline;
            float growth_per_level[20];
        } wisdom;
    
        struct {
            float baseline;
            float growth_per_level[20];
        } luck;

    } class_stat_growth_per_level_t;

    class_stat_growth_per_level_t class_stat_growth[1] = {0};
    class_stat_growth[0].strength.baseline = 15;
    class_stat_growth[0].agility.baseline = 15;
    class_stat_growth[0].stamina.baseline = 15;
    class_stat_growth[0].wisdom.baseline = 15;
    class_stat_growth[0].luck.baseline = 15;

    typedef enum
    {
        KNIGHT_STR,
        KNIGHT_AGI,
        KNIGHT_VIT,
        KNIGHT_WIS,
        KNIGHT_LCK
    } knight_stats;

    float knight_growth_per_level[][20] = {
        // STR
        {4, 4, 4, 4, 4, 8, 8, 8, 8, 8,
        12, 12, 12, 12, 12, 16, 16, 16, 16, 16},

        // AGI
        {9, 9, 9, 9, 9, 9, 4, 4, 4, 4,
        4, 16, 16, 16, 16, 16, 16, 16, 16, 16},

        // VIT
        {9, 9, 9, 9, 9, 9, 4, 4, 4, 4,
        4, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    
        // WIS
        {9, 9, 9, 9, 9, 9, 4, 4, 4, 4,
        4, 16, 16, 16, 16, 16, 16, 16, 16, 16},

        // LCK
        {9, 9, 9, 9, 9, 9, 4, 4, 4, 4,
        4, 16, 16, 16, 16, 16, 16, 16, 16, 16},
    };

    // TODO: Use an index outside of this loop to increment to the next stat gain on level up
    for (int i = 0; i < 20; ++i)
    {
        if (knight_growth_per_level[KNIGHT_STR][i] > class_stat_growth[0].strength.baseline ||
            knight_growth_per_level[KNIGHT_AGI][i] > class_stat_growth[0].agility.baseline  ||
            knight_growth_per_level[KNIGHT_VIT][i] > class_stat_growth[0].stamina.baseline  ||
            knight_growth_per_level[KNIGHT_WIS][i] > class_stat_growth[0].wisdom.baseline   ||
            knight_growth_per_level[KNIGHT_LCK][i] > class_stat_growth[0].luck.baseline)
        {
            knight_growth_per_level[KNIGHT_STR][i] = rand() % 2;
            knight_growth_per_level[KNIGHT_AGI][i] = rand() % 2;
            knight_growth_per_level[KNIGHT_VIT][i] = rand() % 2;
            knight_growth_per_level[KNIGHT_WIS][i] = rand() % 2;
            knight_growth_per_level[KNIGHT_LCK][i] = rand() % 2;
        }


        class_stat_growth[0].strength.growth_per_level[i]   = knight_growth_per_level[KNIGHT_STR][i]; 
        class_stat_growth[0].agility.growth_per_level[i]    = knight_growth_per_level[KNIGHT_AGI][i]; 
        class_stat_growth[0].stamina.growth_per_level[i]    = knight_growth_per_level[KNIGHT_VIT][i]; 
        class_stat_growth[0].wisdom.growth_per_level[i]     = knight_growth_per_level[KNIGHT_WIS][i]; 
        class_stat_growth[0].luck.growth_per_level[i]       = knight_growth_per_level[KNIGHT_LCK][i]; 
        
        printf("knight agi stat growth: %.2f\n", class_stat_growth[0].agility.growth_per_level[i]);
    }

    typedef struct
    {
        // TODO: Include name?
        float stat_growth[5];
    } personality_stat_growth_t;

    float personality_stats[][5] = { 
        // STR,   AGL,   VIT,   WIS,   LCK
        {  .90,  1.20,   .90,  1.00,   .80 }, // acrobat
        { 1.30,   .90,  1.00,  1.10,   .80 }, // amazon
        { 1.00,  1.40,  1.00,  1.00,  1.00 }, // bat out of hell
        { 1.00,  1.20,   .95,  1.15,  1.10 }, // clown
        {  .70,  1.20,   .70,  1.10,  1.30 }, // contrarian
        {  .90,   .90,  1.00,  1.10,  1.15 }, // crybaby
        {  .95,  1.20,  1.15,  1.00,  1.00 }, // daredevil
        {  .95,  1.10,   .95,  1.15,  1.00 }, // daydreamer
        { 1.05,   .90,  1.10,  1.00,  1.00 }, // drudge
        {  .80,  1.05,   .90,  1.25,   .80 }, // egghead
        { 1.00,  1.00,  1.00,  1.00,  1.00 }, // everyman
        { 1.00,   .75,  1.10,  1.05,  1.05 }, // free spirit
        { 1.00,  1.20,   .80,  1.40,   .90 }, // genius
        { 1.05,   .95,  1.05,  1.10,   .95 }, // good egg
        { 1.10,   .60,  1.10,   .50,   .80 }, // gourmand
        {  .90,  1.00,   .90,  1.00,  1.30 }, // happy camper
        { 1.15,  1.00,  1.10,   .90,   .60 }, // idealist
        { 1.05,   .80,  1.30,   .90,   .80 }, // ironclad
        {  .80,  1.15,  1.00,   .70,   .70 }, // klutz
        { 1.15,   .60,  1.20,   .65,  1.10 }, // lazybones
        { 1.00,  1.10,  1.20,  1.10,   .70 }, // lone wolf
        { 1.05,   .95,  1.20,  1.05,   .90 }, // lothario
        { 1.00,   .90,   .90,   .70,  1.10 }, // lout
        { 1.00,  1.10,  1.00,  1.00,  1.50 }, // lucky devil
        { 1.30,   .80,  1.00,   .70,   .80 }, // meathead
        { 1.05,   .85,  1.10,   .80,   .70 }, // meddler
        { 1.00,   .60,  1.20,   .60,   .70 }, // mule
        {  .95,  1.05,   .90,   .90,   .90 }, // narcissist
        { 1.40,   .70,  1.00,   .80,   .70 }, // paragon
        { 1.10,   .85,  1.20,   .90,   .70 }, // plugger
        { 1.00,   .80,   .95,  1.10,  1.40 }, // princess
        {  .85,  1.15,   .80,   .80,   .90 }, // scatterbrain
        { 1.05,  1.10,   .95,  1.05,   .95 }, // show-off
        { 1.10,   .60,  1.20,  1.10,   .90 }, // shrinking violet
        {  .90,  1.10,   .90,  1.20,  1.00 }, // slippery devil
        { 1.00,   .90,   .80,  1.10,  1.10 }, // socialite
        {  .95,  1.05,  1.05,   .95,   .95 }, // sore loser
        {  .95,  1.00,   .90,  1.05,  1.00 }, // spoilt brat
        { 1.00,   .90,  1.00,  1.10,   .90 }, // straight arrow
        { 1.20,   .90,   .90,   .60,   .70 }, // thug
        { 1.10,  1.10,   .80,   .90,   .90 }, // tomboy
        { 1.15,   .90,  1.40,   .80,   .70 }, // tough cookie
        { 1.10,  1.20,  1.05,  1.15,  1.20 }, // vamp
        {  .90,   .70,   .90,  1.20,  1.20 }, // wimp
        {  .95,  1.00,  1.00,  1.30,   .90 }, // wit
    };

    // 45 different personalities
    personality_stat_growth_t personality_stat_growth[45] = {0};
    for (int i = 0; i < ArraySize(personality_stat_growth); ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            personality_stat_growth[i].stat_growth[j] = personality_stats[i][j];
        }
    }

    for (int i = 0; i < 5; ++i)
    {
        printf("%.2f\n", personality_stat_growth[44].stat_growth[i]);
    }

    // TODO: Rather than hard rounding it down as a decimal value, we'll keep it simple to round up or down if the decimal is > .5
    // The reasoning is because a stat reduction of 5-10% wouldn't be fair to cut it down that much to a personality where it's 
    // reduction should acutally be that low. But it also conflicts with the stat reduction being pointless because it's effectively
    // treated the same as normal. 
    //
    // Which is why I think we stick to working with floats in the future. What we can do is keep the decimal values but only render the
    // values as whole numbers. If the player's curernt STR was at 134.6 with a gain of 4.5, it'd be 139.1 where the player will just see 139.
    // Decimals have no influence such that .5 will add that much damage or defense.
    typedef struct
    {
         int index;
         u32 experience;
         u32 remaining;
    } character_experience_t;


    typedef struct
    {
        char name[10];
        u32 level; 
        u32 experience;

        // character class model
        asset_t model;
        class_base_stats_t base_stats;
        
        struct 
        {
            char name[32];
        } class;

        struct
        {
            char name[32]; // leave here for now
        } personality;


    } character_data_t;
    
    character_data_t character_data = {0};

    bool init_name = true;
    const char *dst_txt = "Name:                      "; // Just to specify we're working with a str of size 27, for now
    class_status_overview_t class_status_overview[14] = {
        // Box 1 - Info
        { {CENTER_TEXT_X(dst_txt, 0),                 SCREEN_CENTER_Y - 96},    "Name:                      " },
        { {CENTER_TEXT_X(dst_txt, 0),                 SCREEN_CENTER_Y - 86},    "Lv:                       1" },
        { {CENTER_TEXT_X(dst_txt, 0),                 SCREEN_CENTER_Y - 76},    "Class:                     " },
        { {CENTER_TEXT_X(dst_txt, 0),                 SCREEN_CENTER_Y - 66},    "Personality:               " },

        // Box 2 - Stats
        { {CENTER_TEXT_X(dst_txt, 0),   SCREEN_CENTER_Y - 46},                  "Strength:                 " },
        { {CENTER_TEXT_X(dst_txt, 0),   SCREEN_CENTER_Y - 36},                  "Resillience:              " },
        { {CENTER_TEXT_X(dst_txt, 0),   SCREEN_CENTER_Y - 26},                  "Agility:                  " },
        { {CENTER_TEXT_X(dst_txt, 0),   SCREEN_CENTER_Y - 16},                  "Stamina:                  " },
        { {CENTER_TEXT_X(dst_txt, 0),   SCREEN_CENTER_Y - 6},                   "Wisdom:                   " },
        { {CENTER_TEXT_X(dst_txt, 0),   SCREEN_CENTER_Y + 4},                   "Luck:                     " },
        { {CENTER_TEXT_X(dst_txt, 0),   SCREEN_CENTER_Y + 14},                  "Max HP:                   " },
        { {CENTER_TEXT_X(dst_txt, 0),   SCREEN_CENTER_Y + 24},                  "Max MP:                   " },
        { {CENTER_TEXT_X(dst_txt, 0),   SCREEN_CENTER_Y + 34},                  "Attack:                   " },
        { {CENTER_TEXT_X(dst_txt, 0),   SCREEN_CENTER_Y + 44},                  "Defense:                  " },
    };
    

    typedef struct
    {
        int index;
        bool is_active;
        menu_item_t button[1];
    } next_button_t;

    next_button_t next_button = {0};
    next_button.button[0].text = "Next";
    next_button.button[0].x = CENTER_TEXT_X(next_button.button[0].text, 96);
    next_button.button[0].y = SCREEN_CENTER_Y + 96;

    bool boulder_has_reached_end = false;

    // Start event
    is_title_screen = true;
    Running = true;

    SDL_Color hold_my_color = {0};

    bool personality_results_screen = false;
    bool loading_results = false;
    bool player_talking_to_oldman = false;
    bool first_forest_entrance = false;
    bool hold_dialogue_box = false;
    bool hold_dialogue_box_return_boulder = false;
    bool next_text = false;
    bool display_player_gold_count = false;

    int dialogue_index = 0;
    const char *forest_scenario_dialogue_intro[32] = {
        {"Whoa-whoa-whoa!\nI see you're lost. Go"},
        {"west. That's to your left.\nKeep walking"},
        {"in that direction and\nyou'll be clear of"},
        {"this forest. If you\nhappen to see a boulder,"},
        {"could you push it back\nto me? I will be sure"},
        {"to repay you."}
    };

    int dialogue_index_2 = 0;
    const char *forest_scenario_dialogue_return_the_boulder[32] = {
        {"Oh! You brought the\nrock to me. Thank you!"},
        {"Here's 10 gold coins\nfor your troubles."}
    };

    int gold_count_from_old_man = 0;

    printf("%s\n", forest_scenario_dialogue_intro[dialogue_index]);
    printf("%s\n", forest_scenario_dialogue_return_the_boulder[dialogue_index_2]);

    SDL_GameController *controller = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
    {
        if (SDL_IsGameController(i))
        {
            controller = SDL_GameControllerOpen(i);
            if (controller)
            {
                printf("Opened controller: %s\n", SDL_GameControllerName(controller));
                break;
            }
            else
            {
                printf("Could not open game controller %i: %s\n", SDL_GetError());
            }   
        }
    }

    SDL_Color stat_overview_color[10] = {0};
  
    // -------- In-game menus --------
    
    // A menu that opens when the player hits ESC, options for sound settings and to exit,
    // save in the future
    typedef struct
    {
        int index;
        bool is_active;

        // Two options for sound settings and exit
        menu_item_t options[2];

        // Init with our current volume settings
        sound_settings_t *sound_settings;
    } game_settings_t;

    typedef struct
    {
        // Properties of an item:
        // -> ID 
        // -> bool if it can be used or not, some classes don't use MP, so trying to recovering MP should be handled. 
        // -> name
        // -> description
        // -> stat effects (hp, mp ...)i
        // -> buy/sell value
        // -> asset 
   
        // Only implementing health and mana pots for now
        u32 id;
       
        u32 hp_recovery;
        u32 mp_recovery;
        u32 buy_value;
        u32 sell_value;
        
        const char *name;
        const char *description;

        asset_t asset;
    } game_item_t;

    // Example use of item creation below
#define ITEM_COUNT 2
#define ITEM_HEALTH_POTION 0
#define ITEM_MANA_POTION   1 
    game_item_t game_items[ITEM_COUNT] = {0};

    game_items[ITEM_HEALTH_POTION].id = 0;
    game_items[ITEM_HEALTH_POTION].hp_recovery = 20;
    game_items[ITEM_HEALTH_POTION].mp_recovery = 0;
    game_items[ITEM_HEALTH_POTION].buy_value = 5;
    game_items[ITEM_HEALTH_POTION].sell_value = 1;
    game_items[ITEM_HEALTH_POTION].name = "Health Potion";
    game_items[ITEM_HEALTH_POTION].description = "Recovers 20 HP";
    game_items[ITEM_HEALTH_POTION].asset = IdealLoadAsset("assets/sprites/archer.png");

    game_items[ITEM_MANA_POTION].id = 1;
    game_items[ITEM_MANA_POTION].hp_recovery = 0;
    game_items[ITEM_MANA_POTION].mp_recovery = 30;
    game_items[ITEM_MANA_POTION].buy_value = 5;
    game_items[ITEM_MANA_POTION].sell_value = 1;
    game_items[ITEM_MANA_POTION].name = "Mana Potion";
    game_items[ITEM_MANA_POTION].description = "Recovers 30 MP";
    game_items[ITEM_MANA_POTION].asset = IdealLoadAsset("assets/sprites/archer.png");
  
    typedef struct
    {
        // Properties of an equipment:
        // -> ID 
        // -> bool if the equipment can be equiped, wizard cannot be using long swords...or maybe they can
        // -> name
        // -> description
        // -> stat effects (atk, wis, vit, def ...)
        // -> buy/sell value
        // -> asset 

        // Tiers in terms of quality for weapons and armors from weakest to strongest 
        // -> Leather 
        // -> Wood
        // -> Iron
        // -> Steel
        
        u32 id;
    
        u32 attack;
        u32 defense;
        u32 buy_value;
        u32 sell_value;

        u32 range;
        const char *name;
        const char *description; // debatable, 
        
        // white -> common -> 61% 
        // blue -> rare -> 25%
        // purple -> epic -> 8%
        // yellow -> legendary -> 5%
        // red -> mythical -> 1%
        const char *rarity; 
       
        asset_t model;
    } game_equipment_t;
    
    int rarity_roller = rand() % 99;

    // Out of 100 values, there are 5 different regions, each of which will have own threshold/percentage-rate
    int rarity_common_threshold = 60; // ~60% chance
    int rarity_rare_threshold = 85; 
    int rarity_epic_threshold = 93; 
    int rarity_legenary_threshold = 98; 
    int rarity_mythical_threshold = 99; // 0 - 99 == 100
    
    if (rarity_roller <= rarity_common_threshold)
    {
        printf("common! 60 percent \n");
    }
    else if (rarity_roller <= rarity_rare_threshold && 
             rarity_roller > rarity_common_threshold)
    {
        printf("rare! 25 percent \n");
    }
    else if (rarity_roller <= rarity_epic_threshold && 
             rarity_roller > rarity_rare_threshold)
    {
        printf("epic! 8 percent \n");
    }
    else if (rarity_roller <= rarity_legenary_threshold && 
             rarity_roller > rarity_epic_threshold)
    {
        printf("legendary! 5 percent \n");
    }
    else if (rarity_roller <= rarity_mythical_threshold && 
             rarity_roller > rarity_legenary_threshold)
    {
        printf("mythical! 1 percent \n");
    }
    else
    {
        printf("invalid rarity\n");
    }

    // long sword example
    game_equipment_t equipment[1] = {0};
    equipment[0].id = 0;
    equipment[0].attack = 5;
    equipment[0].buy_value = 10;
    equipment[0].sell_value = 5;
    equipment[0].range = 2; // 2 tiles
     
    equipment[0].name = "Long Sword";
    equipment[0].rarity = "Common"; // randomly roll through the rarities and apply name, common default, we may even have it roll as well when the player spawns and not only through chests, just for that extra spice. 
    equipment[0].model = IdealLoadAsset("assets/weapons/long_sword.png");


    typedef struct
    {
        // Properties of each slot:
        //  -> bool to check if it's empty or not, if it's empty a new item can be stored, if it's an existing item
        //  you're picking it then it can be stacked.

        bool occupied;
        asset_t asset;
    } game_command_menu_slots_t;

    typedef struct
    {
        int index;
        bool is_active;

        game_command_menu_slots_t slots[18]; // 18 inventory slots
    } game_command_menu_items_t;

    game_command_menu_items_t gcm_items = {0};
    for (int i = 0; i < 18; ++i)
        gcm_items.slots[i].asset = IdealLoadAsset("assets/sprites/archer.png"); // placeholder for slot, I'd like it squared so we should create it to be 16x16 at least, 24x24 for larger



    typedef struct
    {
        // Rough idea:
        // There will be a helm, chestplate, main-hand, off-hand, and two accessory slots, 
        // a total of 6 slots. There is then for now depending on how large we decide the
        // inventory to be, where both of these will be in the same
        // array, but the equipment slots 'cut off'. This approach I think will be simple
        // work with in that if I wanted to equip or unequip something, it'd swap with
        // whatever item or slot in the inventory, rather than keeping them seperate
        // and writing some middle man to talk between the two.

        // We'll be using a 1D array, not 2D, and traverse through the inventory similarly to
        // how we are with our glyph grid from name entry. And to keep the movement aligned in moving between
        // the equipment slots and item slots, we should instead create a grid that's 6 high  
        // to match with the 6 equipment slots, OTHERWISE we'll have the same issue with our glyph grid 
        // where our confirm button is 'seperate' from the rest of the grid, but only works in this 1D
        // array because we aligned the padding of the 1 confirm button we have an equal width
        // of the glyph grid.
        
        int index;
        bool is_active;


    } game_command_menu_equipment_t;



    // A command menu similar to that from FF games, opens on TAB
    typedef struct
    {
        // Opening this game menu pops to the right side of the screen, with the
        // list of commands. Each command hovered over will render a box for that specific
        // option to the center of the screen, example 'items', this is the first index
        // that is hovered over, so the player can expect to see their inventory of 
        // items upon opening this menu. Hitting confirm for that option will move 
        // the cursor over to that box, in this case of the 'items', the player will
        // enter their inventory to examine or use their items.
        
        //   |        | > | Items |
        //   |        |   | Equipment |
        //   |        |   | Status |
        //   |        |   | ... |
        //   |        |   | ... |


        /* Items -> Overview of entire inventory, player can use items and only examine equipments
         * Attack ?
         * Spells ?
         * Equipment -> Inventory for only equipments that the player can examine and equip
         * Status -> Overview of character's status; name, level, personality, exp, remaining exp to next level, stats and more
         */

        // Cursor package -> index and bool
        int index;
        bool is_active; 

        // Menu package
        bool is_opened;
    
        // Asset for the command menu and respective options (items, equipment ...)
        asset_t command_menu;
        asset_t menu_box[3];
        
        menu_item_t options[3]; // items, equipment and status for now
    } game_command_menu_t;

    game_command_menu_t game_menu = {0};

    while (Running) 
    {
        // Poll events
        while (SDL_PollEvent(&SDLWindow.e))
        {
            switch (SDLWindow.e.type)
            {
                case SDL_QUIT:
                {
        Running = false;
                } break;
                case SDL_CONTROLLERBUTTONDOWN:
                {
                    switch (SDLWindow.e.cbutton.button)
                    {
                        case SDL_CONTROLLER_BUTTON_DPAD_UP:
                        {
                          
                        } break;
                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                        {
                            
                        } break;
                        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                        {
                            
                        } break;
                        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                        {
                            
                        } break;
                        case SDL_CONTROLLER_BUTTON_B:
                        {
                            
                        } break;
                        case SDL_CONTROLLER_BUTTON_A:
                        {
                            
                        } break;
                    }
                } break;
                case SDL_KEYDOWN:
                {  
                    //if (SDLWindow.e.key.repeat == 0)
                    {
                        switch (SDLWindow.e.key.keysym.sym)
                        {
                            case SDLK_w:
                            {
                                character_data.model.direction.up = true;
                                boulder_asset.direction.up = true;

                                if (boulder_has_reached_end)
                                    boulder_has_reached_end = false;

                                if (is_title_screen)
                                {
                                    option_index--;
                                    if (option_index < 0)
                                        option_index = ArraySize(title_screen_options) - 1;
                                    Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }
                                
                                if (is_new_game)
                                {
                                    button_select--;
                                    if (button_select < 0)
                                        button_select = ArraySize(confirmation_buttons) - 1;
                                }
                                
                                if (confirmation.is_active)
                                {
                                    confirmation.index--;
                                    if (confirmation.index < 0)
                                        confirmation.index = ArraySize(confirmation.info.buttons) - 1;
                                    //Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }

                                if (is_settings)
                                {
                                    Sound_MoveUpSettings(&sound_settings); 
                                    Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }
                            
                                if (is_name_submission)
                                {
                                    NameEntry_MoveUp(&name_entry);
                                }

                                Personality_MoveUpScenario(&test_scenarios);
                                
                                if (personality_scenario[0].is_active)
                                {
                                    personality_scenario[0].index--;
                                    if (personality_scenario[0].index < 0)
                                        personality_scenario[0].index = ArraySize(personality_scenario[0].info.monster_options) - 1;
                                }

                            } break;
                            case SDLK_s:
                            {
                                character_data.model.direction.down = true;
                                boulder_asset.direction.down = true;
                                
                                if (boulder_has_reached_end)
                                    boulder_has_reached_end = false;
                                if (player_talking_to_oldman)
                                    player_talking_to_oldman = false;

                                if (is_title_screen)
                                {
                                    option_index++;
                                    if (option_index >= ArraySize(title_screen_options))
                                        option_index = 0;
                                    Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }

                                if (is_new_game)
                                {
                                    button_select++;
                                    if (button_select >= ArraySize(confirmation_buttons))
                                        button_select = 0;
                                }
        
                                if (confirmation.is_active)
                                {
                                    confirmation.index++;
                                    if (confirmation.index >= ArraySize(confirmation.info.buttons))
                                        confirmation.index = 0;
                                    //Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }

                                if (is_settings)
                                {
                                    Sound_MoveDownSettings(&sound_settings); 
                                    Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }

                                if (is_name_submission)
                                {
                                    NameEntry_MoveDown(&name_entry);
                                }  

                                Personality_MoveDownScenario(&test_scenarios);

                                if (personality_scenario[0].is_active)
                                {
                                    personality_scenario[0].index++;
                                    if (personality_scenario[0].index >= ArraySize(personality_scenario[0].info.monster_options))
                                        personality_scenario[0].index = 0;
                                }
                            } break;
                            case SDLK_a:
                            {
                                character_data.model.direction.left = true;
                                boulder_asset.direction.left = true;
                                
                                if (boulder_has_reached_end)
                                    boulder_has_reached_end = false;

                                if (character_creation_screen.is_active && !confirmation.is_active)
                                {
                                    character_creation_screen.index--;
                                    if (character_creation_screen.index < 0)
                                        character_creation_screen.index = ArraySize(character_creation_screen.info) - 1;
                                    Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }

                                if (character_allocation_select_screen.is_active && !confirmation.is_active)
                                {
                                    character_allocation_select_screen.index--;
                                    if (character_allocation_select_screen.index < 0)
                                        character_allocation_select_screen.index = ArraySize(character_allocation_select_screen.info) - 1;
                                    Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }
                          
                                // switch case below
                                if (is_settings && sound_settings.index == 0)
                                {
                                    //Sound_DecreaseVolume(&volume_controller[0]);
                                    Sound_TestDecreaseVolume(&test_volume_controller, MASTER_INDEX);
                                    Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }

                                if (is_settings && sound_settings.index == 1)
                                {
                                    //Sound_DecreaseVolume(&volume_controller[1]);
                                    Sound_TestDecreaseVolume(&test_volume_controller, MUSIC_INDEX);
                                    Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }

                                if (is_settings && sound_settings.index == 2)
                                {
                                    //Sound_DecreaseVolume(&volume_controller[2]);
                                    Sound_TestDecreaseVolume(&test_volume_controller, SFX_INDEX);
                                    Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                } 

                                if (is_settings)
                                {
                                    switch (sound_settings.index)
                                    {
                                        case 0:
                                        {
                                            volume_settings_state = VOL_SETTINGS_MASTER;
                                        } break;
                                        case 1:
                                        {
                                            volume_settings_state = VOL_SETTINGS_MUSIC;
                                        } break;
                                        case 2:
                                        {
                                            volume_settings_state = VOL_SETTINGS_SFX;
                                        } break;
                                        case 3:
                                        {
                                            Sound_MoveLeftSettings(&sound_settings);
                                        } break;
                                        case 4:
                                        {
                                            Sound_MoveLeftSettings(&sound_settings);
                                        } break;
                                    }

                                    for (int vc = 0; vc < VOLUME_CONTROLLER_COUNT; ++vc) 
                                    {
                                        switch (test_volume_controller.info[vc].index)
                                        {
                                            case 0:
                                            {
                                                //test_volume_controller.info[vc].mute = true;
                                                test_volume_controller.info[vc].mute = true;
                                            } break;
                                            case 1:
                                            {
                                                test_volume_controller.info[vc].one = true;
                                            } break;
                                            case 2:
                                            {
                                                test_volume_controller.info[vc].two = true;
                                            } break;
                                            case 3:
                                            {
                                                test_volume_controller.info[vc].three = true;
                                            } break;
                                            case 4:
                                            {
                                                test_volume_controller.info[vc].max = true;
                                            } break;
                                            default:
                                            {
                                                test_volume_controller.info[vc].one = true;
                                            } break;

                                        }
                                    }

                                }

                                if (is_name_submission)
                                {
                                    NameEntry_MoveLeft(&name_entry);
                                }

                                if (personality_results_screen)
                                {
                                    next_button.index--;
                                    if (next_button.index < 0)
                                        next_button.index = ArraySize(next_button.button) - 1;
                                }
                            } break;
                            case SDLK_d:
                            {
                                character_data.model.direction.right = true;
                                boulder_asset.direction.right = true;

                                if (boulder_has_reached_end)
                                    boulder_has_reached_end = false;

                                if (character_creation_screen.is_active && !confirmation.is_active)
                                {
                                    character_creation_screen.index++;
                                    if (character_creation_screen.index >= ArraySize(character_creation_screen.info))
                                        character_creation_screen.index = 0;
                                    Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }

                                if (character_allocation_select_screen.is_active && !confirmation.is_active)
                                {
                                    character_allocation_select_screen.index++;
                                    if (character_allocation_select_screen.index >= ArraySize(character_allocation_select_screen.info))
                                        character_allocation_select_screen.index = 0;
                                    Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }

                                if (is_settings)
                                {
                                    switch (sound_settings.index)
                                    {
                                        case 0:
                                        {
                                            Sound_TestIncreaseVolume(&test_volume_controller, 0);
                                            Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                            
                                            volume_controller[0].touched = true;
                                            printf("master touched\n");
                                            volume_settings_state = VOL_SETTINGS_MASTER;

                                        } break;
                                        case 1:
                                        {
                                            Sound_TestIncreaseVolume(&test_volume_controller, 1);
                                            Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                           
                                            volume_controller[1].touched = true;
                                            printf("music touched\n");
                                            volume_settings_state = VOL_SETTINGS_MUSIC;

                                        } break;
                                        case 2:
                                        {
                                            Sound_TestIncreaseVolume(&test_volume_controller, 2);
                                            Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                            
                                            volume_controller[2].touched = true;
                                            printf("sfx touched\n");
                                            volume_settings_state = VOL_SETTINGS_SFX;
                                        } break;
                                        case 3:
                                        {
                                            Sound_MoveRightSettings(&sound_settings);
                                        } break;
                                        case 4:
                                        {
                                            Sound_MoveRightSettings(&sound_settings);

                                        } break;
                                    }

                                    for (int vc = 0; vc < VOLUME_CONTROLLER_COUNT; ++vc) 
                                    {
                                        switch (test_volume_controller.info[vc].index)
                                        {
                                            case 0:
                                            {
                                                test_volume_controller.info[vc].mute = true;
                                            } break;
                                            case 1:
                                            {
                                                test_volume_controller.info[vc].one = true;
                                            } break;
                                            case 2:
                                            {
                                                test_volume_controller.info[vc].two = true;
                                            } break;
                                            case 3:
                                            {
                                                test_volume_controller.info[vc].three = true;
                                            } break;
                                            case 4:
                                            {
                                                test_volume_controller.info[vc].max = true;
                                            } break;
                                            default:
                                            {
                                                test_volume_controller.info[vc].one = true;
                                            } break;
                                        }
                                    }

                                }
                               
                                
                                if (is_name_submission)
                                {
                                    NameEntry_MoveRight(&name_entry);
                                }

                                if (personality_results_screen)
                                {
                                    next_button.index++;
                                    if (next_button.index >= ArraySize(next_button.button))
                                        next_button.index = 0;
                                }
                            } break;
                            case SDLK_BACKSPACE:
                            {
                                if (is_name_submission)
                                {
                                    name_entry.is_active = true;
                                    character_class_name_submission_state = CLASS_NAME_DELETE;
                                }
                            } break;
                            case SDLK_ESCAPE:
                            {
                                if (character_creation_screen.is_active)
                                {
                                    class_select_state = CLASS_SELECT_BACK;
                                }

                                if (character_allocation_select_screen.is_active && !personality_test.is_active)
                                {
                                    class_allocation_state = CLASS_ALLOCATION_BACK;
                                }

                                if (is_settings)
                                {
                                    volume_settings_state = VOL_SETTINGS_BACK;
                                }
                            } break;
                            case SDLK_TAB:
                            {
                                if (is_game_running && !game_menu.is_opened)
                                {
                                    game_menu.is_opened = true;
                                    printf("menu opened\n");
                                }
                                else
                                {
                                    game_menu.is_opened = false;
                                    printf("menu closed\n");
                                }


                            } break;
                            case SDLK_RETURN:
                            {
                                // TEST
                                if (is_title_screen)
                                {
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                    switch (option_index)
                                    {
                                        case 0:
                                        {
                                            title_screen_state = TITLE_NEW_GAME;
                                        } break;
                                        case 1:
                                        {
                                            title_screen_state = TITLE_LOAD_GAME;
                                        } break;
                                        case 2:
                                        {
                                            title_screen_state = TITLE_SETTINGS;
                                        } break;
                                        case 3:
                                        {
                                            title_screen_state = TITLE_EXIT;
                                        } break;
                                        default:
                                        {
                                            title_screen_state = TITLE_NONE;
                                        } break;
                                    }
                                }
                                
                                if (character_creation_screen.is_active)
                                {
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                    switch (character_creation_screen.index)
                                    {
                                        case 0:
                                        {
                                            class_select_state = CLASS_SELECT_KNIGHT;
                                        } break;
                                        case 1:
                                        {
                                            class_select_state = CLASS_SELECT_PALADIN;
                                        } break;
                                        case 2:
                                        {
                                            class_select_state = CLASS_SELECT_WIZARD;
                                        } break; 
                                        case 3:
                                        {
                                            class_select_state = CLASS_SELECT_ARCHER;
                                        } break;
                                    }
                                }
                               
                                if (character_allocation_select_screen.is_active)
                                {
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                    switch (character_allocation_select_screen.index)
                                    {
                                        case 0:
                                        {
                                            class_allocation_state = CLASS_ALLOCATION_PERSONALITY; 
                                        } break;
                                        case 1:
                                        {
                                            class_allocation_state = CLASS_ALLOCATION_PRESET; 
                                        } break;
                                        case 2:
                                        {
                                            class_allocation_state = CLASS_ALLOCATION_MANUAL; 
                                        } break;
                                    }
                                }

                                if (character_creation_screen.is_active && confirmation.is_active)
                                {
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                    switch (confirmation.index)
                                    {
                                        case 0:
                                        {
                                            character_creation_screen.is_active = false;
                                            character_allocation_select_screen.is_active = true;
                                            confirmation.is_active = false; 
                                        } break;
                                        case 1:
                                        {
                                            confirmation.is_active = false; 
                                            confirmation.index = 0;
                                            class_select_state = CLASS_SELECT_NONE;
                                        } break;
                                    }
                                 
                                }
                              
                                if (character_allocation_select_screen.is_active && confirmation.is_active)
                                {
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                    switch (confirmation.index)
                                    {
                                        case 0:
                                        {
                                            if (character_allocation_select_screen.index == 0)
                                            {
                                                character_allocation_select_screen.is_active = false;
                                                is_personality_test = true;
                                                personality_test.is_active = true;
                                            }
                                            
                                            confirmation.is_active = false; 
                                        } break;
                                        case 1:
                                        {
                                            confirmation.is_active = false;
                                            confirmation.index = 0;
                                            class_allocation_state = CLASS_ALLOCATION_NONE;
                                        } break;
                                    }
                                }

                                if (is_personality_test && personality_test.is_active && confirmation.is_active)
                                {
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                    switch (confirmation.index)
                                    {
                                        case 0: // yes
                                        {
                                            // turn into a function
                                            switch (personality_test.index)
                                            {
                                                case 1:
                                                {
                                                    personality_test.index = 7;
                                                } break;
                                                case 2:
                                                {
                                                    personality_test.index = 14;
                                                } break;
                                                case 3:
                                                {
                                                    personality_test.index = 6;
                                                } break;
                                                case 4:
                                                {
                                                    personality_test.index = 15;
                                                } break;
                                                case 5:
                                                {
                                                    personality_test.index = 8;
                                                } break;
                                                case 6:
                                                {
                                                    personality_test.index = 7;
                                                } break;
                                                case 7:
                                                {
                                                    personality_test.index = 10;
                                                } break;
                                                case 8:
                                                {
                                                    personality_test.index = 10;
                                                } break;
                                                case 9:
                                                {
                                                    personality_test.index = 11;
                                                } break;
                                                case 10:
                                                {
                                                    personality_test.index = 14;
                                                } break;
                                                case 11:
                                                {
                                                    personality_test.index = 14;
                                                } break;
                                                case 12:
                                                {
                                                    personality_test.index = 31;
                                                } break;
                                                case 13:
                                                {
                                                    personality_test.index = 25;
                                                } break;
                                                case 14:
                                                {
                                                    personality_test.index = 18;
                                                } break;
                                                case 15:
                                                {
                                                    personality_test.index = 16;
                                                } break;
                                                case 16:
                                                {
                                                    personality_test.index = 17;
                                                } break;
                                                case 17:
                                                {
                                                    personality_test.index = 21;
                                                } break;
                                                case 18:
                                                {
                                                    personality_test.index = 19;
                                                } break;
                                                case 19:
                                                {
                                                    personality_test.index = 20;
                                                } break;
                                                case 20:
                                                {
                                                    personality_test.index = 21;
                                                } break; 
                                                case 21:
                                                {
                                                    personality_test.index = 23;
                                                } break;
                                                case 22:
                                                {
                                                    personality_test.index = 38;
                                                } break;
                                                case 23:
                                                {
                                                    personality_test.index = 24;
                                                } break;
                                                case 24:
                                                {
                                                    personality_test.index = 34;
                                                } break;
                                                case 25:
                                                {
                                                    personality_test.index = 31;
                                                } break;
                                                case 26:
                                                {
                                                    personality_test.index = 27;
                                                } break;
                                                case 27:
                                                {
                                                    personality_test.index = 28;
                                                } break;
                                                case 28:
                                                {
                                                    personality_test.index = 29;
                                                } break;
                                                case 29:
                                                {
                                                    personality_test.index = 30;
                                                } break;
                                                case 30:
                                                {
                                                    character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_VILLAGE; // final question
                                                } break;
                                                case 31:
                                                {
                                                    personality_test.index = 32;
                                                } break;
                                                case 32:
                                                {
                                                    personality_test.index = 33;
                                                } break;
                                                case 33:
                                                {
                                                    character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_DESERT; // final question
                                                } break;
                                                case 34:
                                                {
                                                    character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_FOREST; //CLASS_PERSONALITY_RESULT_MONSTER; // final question
                                                } break;
                                                case 35:
                                                {
                                                    personality_test.index = 0; // final question
                                                } break;
                                                case 36:
                                                {
                                                    personality_test.index = 37;
                                                } break;
                                                case 37:
                                                {
                                                    personality_test.index = 43;
                                                } break;
                                                case 38:
                                                {
                                                    personality_test.index = 39;
                                                } break;
                                                case 39:
                                                {
                                                    character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_TOWER;
                                                } break;
                                                case 40:
                                                {
                                                    personality_test.index = 42;
                                                } break; 
                                                case 41:
                                                {
                                                    personality_test.index = 43;
                                                } break;
                                                case 42:
                                                {
                                                    personality_test.index = 43;
                                                } break;
                                                case 43:
                                                {
                                                    character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_FOREST; 
                                                } break;
                                                case 44:
                                                {
                                                    character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_THEATER; 
                                                } break;
                                                case 45:
                                                {
                                                    personality_test.index = 47;
                                                } break;
                                                case 46:
                                                {
                                                    character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_CAVE; 
                                                } break;
                                                case 47:
                                                {
                                                    character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_CAVE; 
                                                } break;
                                                case 48:
                                                {
                                                    character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_FOREST;//CLASS_PERSONALITY_RESULT_CASTLE;
                                                } break;
                                                case 49:
                                                {
                                                   character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_FOREST;//CLASS_PERSONALITY_RESULT_CASTLE; 
                                                } break;
                                            }
                                        } break;
                                        case 1: // no
                                        {
                                            switch (personality_test.index)
                                            {
                                                case 1:
                                                {
                                                    personality_test.index = 9;
                                                } break;
                                                case 2:
                                                {
                                                    personality_test.index = 8;
                                                } break;
                                                case 3:
                                                {
                                                    personality_test.index = 8;
                                                } break;
                                                case 4:
                                                {
                                                    personality_test.index = 6;
                                                } break;
                                                case 5:
                                                {
                                                    personality_test.index = 16;
                                                } break;
                                                case 6:
                                                {
                                                    personality_test.index = 8;
                                                } break;
                                                case 7:
                                                {
                                                    personality_test.index = 5;
                                                } break;
                                                case 8:
                                                {
                                                    personality_test.index = 9;
                                                } break;
                                                case 9:
                                                {
                                                    personality_test.index = 12;
                                                } break;
                                                case 10:
                                                {
                                                    personality_test.index = 13;
                                                } break;
                                                case 11:
                                                {
                                                    personality_test.index = 13;
                                                } break;
                                                case 12:
                                                {
                                                    personality_test.index = 14;
                                                } break;
                                                case 13:
                                                {
                                                    personality_test.index = 15;
                                                } break;
                                                case 14:
                                                {
                                                    personality_test.index = 19;
                                                } break;
                                                case 15:
                                                {
                                                    personality_test.index = 20;
                                                } break;
                                                case 16:
                                                {
                                                    personality_test.index = 22;
                                                } break;
                                                case 17:
                                                {
                                                    personality_test.index = 25;
                                                } break;
                                                case 18:
                                                {
                                                    personality_test.index = 23;
                                                } break;
                                                case 19:
                                                {
                                                    personality_test.index = 25;
                                                } break;
                                                case 20:
                                                {
                                                    personality_test.index = 22;
                                                } break; 
                                                case 21:
                                                {
                                                    personality_test.index = 23;
                                                } break;
                                                case 22:
                                                {
                                                    personality_test.index = 38;
                                                } break;
                                                case 23:
                                                { 
                                                    personality_test.index = 40;
                                                } break;
                                                case 24:
                                                {
                                                    personality_test.index = 25;
                                                } break;
                                                case 25:
                                                {
                                                    personality_test.index = 26;
                                                } break;
                                                case 26:
                                                {
                                                    personality_test.index = 28;
                                                } break;
                                                case 27:
                                                {
                                                    personality_test.index = 29;
                                                } break;
                                                case 28:
                                                {
                                                    personality_test.index = 30;
                                                } break;
                                                case 29:
                                                {
                                                    personality_test.index = 30;
                                                } break;
                                                case 30:
                                                {
                                                    personality_test.index = 40; 
                                                } break;
                                                case 31:
                                                {
                                                    personality_test.index = 34;
                                                } break;
                                                case 32:
                                                {
                                                    personality_test.index = 36;
                                                } break;
                                                case 33:
                                                {
                                                    personality_test.index = 36;
                                                } break;
                                                case 34:
                                                {
                                                    personality_test.index = 36;
                                                } break;
                                                case 35:
                                                {
                                                    personality_test.index = 36; 
                                                } break;
                                                case 36:
                                                {
                                                    personality_test.index = 48;
                                                } break;
                                                case 37:
                                                {
                                                    personality_test.index = 49;
                                                } break;
                                                case 38:
                                                {
                                                    personality_test.index = 40;
                                                } break;
                                                case 39:
                                                {
                                                    personality_test.index = 41; 
                                                } break;
                                                case 40:
                                                {
                                                    personality_test.index = 41;
                                                } break; 
                                                case 41:
                                                {
                                                    personality_test.index = 42;
                                                } break;
                                                case 42:
                                                {
                                                    personality_test.index = 44;
                                                } break;
                                                case 43:
                                                {
                                                    personality_test.index = 45; 
                                                } break;
                                                case 44:
                                                {
                                                    personality_test.index = 45;
                                                } break;
                                                case 45:
                                                {
                                                    personality_test.index = 46;
                                                } break;
                                                case 46:
                                                {
                                                    personality_test.index = 47; 
                                                } break;
                                                case 47:
                                                {
                                                    character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_THEATER; 
                                                } break;
                                                case 48:
                                                {
                                                    personality_test.index = 49;
                                                } break;
                                                case 49:
                                                {
                                                    character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_CAVE;
                                                } break;
                                            }
                                        } break;
                                    }
                                }
                                                           

                                if (is_settings)
                                {
                                    switch (sound_settings.index)
                                    {
                                        case 3: // apply
                                        {
                                            Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                            test_volume_controller.apply = true;
                                            volume_settings_state = VOL_SETTINGS_APPLY;
                                        } break;
                                        case 4:
                                        {
                                            Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                            volume_settings_state = VOL_SETTINGS_BACK;
                                        } break;
                                        default:
                                        { 
                                             
                                        } break;
                                    }
                                }

                                if (is_name_submission)
                                {
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                    
                                    printf("name_entry.index: %d\n", name_entry.index);
                                    name_entry.index = name_entry.current_row * NAME_ENTRY_GRID_COLS + name_entry.current_col;

                                    // Glyph grid
                                    if (name_entry.index >= 0 && name_entry.index < 50)
                                    {
                                        name_entry.is_active = true;
                                        character_class_name_submission_state = CLASS_NAME_ENTER;
                                    }

                                    // Button grid 
                                    if (name_entry.index >= 50 && name_entry.index < 60)
                                    {
                                        name_entry.is_active = true;
                                        character_class_name_submission_state = CLASS_NAME_CONFIRM;
                                    }
                                }

                                if (is_personality_test)
                                {
                                    if (test_scenarios.result & SCENARIO_VILLAGE)
                                    {
                                        switch (test_scenarios.scenario[VILLAGE_INDEX].index)
                                        {
                                            case 0:
                                            {
                                                printf("Show-off\n");
                                                personality_types_state = PERSONALITY_SHOW_OFF;
                                                PushString(test_scenarios.personality, "Show-Off");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[VILLAGE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                            case 1:
                                            {
                                                printf("Slippery Devil\n");
                                                personality_types_state = PERSONALITY_SLIPPERY_DEVIL;
                                                PushString(test_scenarios.personality, "Slippery Devil");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[VILLAGE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                            case 2:
                                            {
                                                printf("Shrinking Violet\n");
                                                personality_types_state = PERSONALITY_SHRINKING_VIOLET;
                                                PushString(test_scenarios.personality, "Shrinking Violet");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[VILLAGE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                        }

                                    }

                                    if (test_scenarios.result & SCENARIO_MONSTER)
                                    {
                                        switch (test_scenarios.scenario[MONSTER_INDEX].index)
                                        {
                                            case 0: 
                                            {
                                                printf("Paragon\n");
                                                personality_types_state = PERSONALITY_PARAGON;
                                                PushString(test_scenarios.personality, "Paragon");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[MONSTER_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                            case 1: 
                                            {
                                                printf("Wimp\n");
                                                personality_types_state = PERSONALITY_WIMP;
                                                PushString(test_scenarios.personality, "Wimp");
                                                loading_results = true;
                                                test_scenarios.scenario[MONSTER_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                            case 2: 
                                            {
                                                printf("Spoilt Brat\n");
                                                personality_types_state = PERSONALITY_SPOILT_BRAT;
                                                PushString(test_scenarios.personality, "Spoilt Brat");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[MONSTER_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                            case 3: 
                                            {
                                                printf("Egghead\n");
                                                personality_types_state = PERSONALITY_EGGHEAD;
                                                PushString(test_scenarios.personality, "Egghead");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[MONSTER_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                            case 4: 
                                            {
                                                printf("Klutz\n");
                                                personality_types_state = PERSONALITY_KLUTZ;
                                                PushString(test_scenarios.personality, "Klutz");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[MONSTER_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                        } 
                                    }

                                    if (test_scenarios.result & SCENARIO_CAVE)
                                    {
                                        switch (test_scenarios.scenario[CAVE_INDEX].index)
                                        {
                                            case 0: 
                                            {
                                                printf("Straight Arrow\n");
                                                personality_types_state = PERSONALITY_STRAIGHT_ARROW;
                                                PushString(test_scenarios.personality, "Straight Arrow");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[CAVE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                            case 1: 
                                            {
                                                printf("Mule\n");
                                                personality_types_state = PERSONALITY_MULE;
                                                PushString(test_scenarios.personality, "Mule");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[CAVE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                            case 2: 
                                            {
                                                printf("Scatterbrain\n");
                                                personality_types_state = PERSONALITY_SCATTER_BRAIN;
                                                PushString(test_scenarios.personality, "Scatterbrain");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[CAVE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                            case 3: 
                                            {
                                                printf("Narcissist\n");
                                                personality_types_state = PERSONALITY_NARCISSIST;
                                                PushString(test_scenarios.personality, "Narcissist");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[CAVE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                            case 4: 
                                            {
                                                printf("Sore Loser\n");
                                                personality_types_state = PERSONALITY_SORE_LOSER;
                                                PushString(test_scenarios.personality, "Sore Loser");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[CAVE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                        } 
                                    }

                                    if (test_scenarios.result & SCENARIO_DESERT)
                                    {
                                        switch (test_scenarios.scenario[DESERT_INDEX].index)
                                        {
                                            case 0: 
                                            {
                                                personality_types_state = PERSONALITY_THUG;
                                                PushString(test_scenarios.personality, "Thug");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[DESERT_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                            case 1: 
                                            {
                                                printf("Daredevil\n");
                                                personality_types_state = PERSONALITY_DAREDEVIL;
                                                PushString(test_scenarios.personality, "Daredevil");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[DESERT_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                            case 2: 
                                            {
                                                printf("Idealist\n");
                                                personality_types_state = PERSONALITY_IDEALIST;
                                                PushString(test_scenarios.personality, "Idealist");
                                                loading_results = true;
                                                
                                                test_scenarios.scenario[DESERT_INDEX].is_active = false;
                                                is_personality_test = false;
                                                personality_results_screen = true;
                                            } break;
                                        } 
                                    }
                                }
                                    
                                if (test_scenarios.result & SCENARIO_TOWER)
                                {
                                    switch (test_scenarios.scenario[TOWER_INDEX].index)
                                    {
                                        case 0: 
                                        {
                                            printf("Daydreamer\n");
                                            personality_types_state = PERSONALITY_DAYDREAMER;
                                            PushString(test_scenarios.personality, "Daydreamer");
                                            loading_results = true;
                                            
                                            test_scenarios.scenario[TOWER_INDEX].is_active = false;
                                            is_personality_test = false;
                                            personality_results_screen = true;
                                        } break;
                                        case 1: 
                                        {
                                            printf("Socialite\n");
                                            personality_types_state = PERSONALITY_SOCIALITE;
                                            PushString(test_scenarios.personality, "Socialite");
                                            loading_results = true;
                                            
                                            test_scenarios.scenario[TOWER_INDEX].is_active = false;
                                            is_personality_test = false;
                                            personality_results_screen = true;
                                        } break;
                                    }
                                }
                               
                                if (test_scenarios.result & SCENARIO_THEATER)
                                {
                                    switch (test_scenarios.scenario[THEATER_INDEX].index)
                                    {
                                        case 0: 
                                        {
                                            personality_types_state = PERSONALITY_FREE_SPIRIT;
                                            PushString(test_scenarios.personality, "Free Spirit");
                                            loading_results = true;
                                            
                                            test_scenarios.scenario[THEATER_INDEX].is_active = false;
                                            is_personality_test = false;
                                            personality_results_screen = true;
                                        } break;
                                        case 1: 
                                        {
                                            personality_types_state = PERSONALITY_CRYBABY;
                                            PushString(test_scenarios.personality, "Crybaby");
                                            loading_results = true;
                                            
                                            test_scenarios.scenario[THEATER_INDEX].is_active = false;
                                            is_personality_test = false;
                                            personality_results_screen = true;
                                        } break;
                                        case 2: 
                                        {
                                            personality_types_state = PERSONALITY_LONE_WOLF;
                                            PushString(test_scenarios.personality, "Lone Wolf");
                                            loading_results = true;
                                            
                                            test_scenarios.scenario[THEATER_INDEX].is_active = false;
                                            is_personality_test = false;
                                            personality_results_screen = true;
                                        } break;
                                        case 3: 
                                        {
                                            personality_types_state = PERSONALITY_LOUT;
                                            PushString(test_scenarios.personality, "Lout");
                                            loading_results = true;
                                            
                                            test_scenarios.scenario[THEATER_INDEX].is_active = false;
                                            is_personality_test = false;
                                            personality_results_screen = true;
                                        } break;
                                    }
                                }

                                if (test_scenarios.scenario[FOREST_INDEX].is_active)
                                {

                                    if (player_talking_to_oldman)
                                        hold_dialogue_box = true;

                                    if (hold_dialogue_box)
                                    {
                                        if (next_text && dialogue_index < 6)
                                        {
                                            dialogue_index++;
                                            next_text = false;
                                        }
      
                                        if (dialogue_index > 5)
                                        {
                                            dialogue_index = 0;
                                            character_data.model.conditions.is_movable = true;
                                            player_talking_to_oldman = false;
                                            hold_dialogue_box = false;
                                        }
                                    }

                                    if (hold_dialogue_box_return_boulder)
                                    {
                                        dialogue_index_2++;
                                        if (dialogue_index_2 > 1)
                                        {
                                            dialogue_index_2 = 0;
                                            character_data.model.conditions.is_movable = true; 
                                            hold_dialogue_box_return_boulder = false;
                                        }
                                    }
                                }

                                if (personality_results_screen)
                                {
                                    if (next_button.is_active)
                                    {
                                        switch (next_button.index)
                                        {
                                            case 0: // next
                                            {
                                                printf("next\n");
                                                Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                                personality_results_screen = false;
                                                is_name_submission = true;
                                                next_button.is_active = false;
                                            } break;
                                        }
                                    }
                                }
                                  
                                if (is_class_overview_screen)
                                {
                                    if (next_button.is_active)
                                    {
                                        switch (next_button.index)
                                        {
                                            case 0: // next
                                            {
                                                printf("next\n");
                                                Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                                is_class_overview_screen = false;
                                                is_game_running = true;
                                                next_button.is_active = false;
                                            } break;
                                        }
                                    }
                                }
                            } break;
                            default:
                            {

                            } break;
                        }
                    }
                } break;
                default:
                {
                   // Nothing 
                } break;
            }
        }
       
        if (is_title_screen)
        {
            Sound_PlayMusic(&master_volume.music[0]->wav);

            switch (title_screen_state)
            {
                case TITLE_NONE:
                {

                } break;
                case TITLE_NEW_GAME:
                {   
                    is_title_screen = false;
                    is_new_game = true;
                    character_creation_screen.is_active = true;
                } break;
                case TITLE_LOAD_GAME:
                {
                    is_title_screen = false;
                    is_game_running = true; 
                } break;
                case TITLE_SETTINGS:
                {
                    is_title_screen = false;
                    is_settings = true;
                } break;
                case TITLE_EXIT:
                {
                    is_title_screen = false;
                    ExitOption();
                } break;
                default:
                {
                    title_screen_state = TITLE_NONE;
                } break;
            }      

   
            SDL_RenderCopy(SDLWindow.Renderer, title_screen_asset.texture, NULL, &title_screen_asset.body);
            CursorForItems(&title_screen_options[option_index], &right_cursor_asset, 4, 1);
            RenderAndUpdateAsset(&right_cursor_asset);
            for (int i = 0; i < ArraySize(title_screen_options); ++i)
            {
                RenderText(SDLWindow.Renderer, font_atlas, 
                           title_screen_options[i].x, 
                           title_screen_options[i].y,
                           title_screen_options[i].text, white);
            }
        }
    
        if (is_settings) 
        {
            SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
            CursorForItems(&sound_settings.options[sound_settings.index], &right_cursor_asset, 6, 1);
            RenderAndUpdateAsset(&right_cursor_asset);
           
            RenderText(SDLWindow.Renderer, font_atlas,
                           CENTER_TEXT_X("Sound Settings", 0), 
                           SCREEN_CENTER_Y - 64,
                           "Sound Settings", 
                           white);


            for (int i = 0; i < ArraySize(sound_settings.options); ++i) // Ignore rendering the apply button 
            {
                RenderText(SDLWindow.Renderer, font_atlas,
                           sound_settings.options[i].x, 
                           sound_settings.options[i].y,
                           sound_settings.options[i].text, 
                           white);
            }

            SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(SDLWindow.Renderer, &sound_settings.volume_body[0]);
            SDL_RenderDrawRect(SDLWindow.Renderer, &sound_settings.volume_body[1]);
            SDL_RenderDrawRect(SDLWindow.Renderer, &sound_settings.volume_body[2]);

            switch (volume_settings_state)
            {
                case VOL_SETTINGS_NONE:
                {
                    Sound_TestRenderVolumeBars(SDLWindow.Renderer, &test_volume_controller, VOLUME_CONTROLLER_COUNT);
                } break;
                case VOL_SETTINGS_MASTER:
                {
                    Sound_TestUpdateVolumeBars(&test_volume_controller, MASTER_INDEX);
                    Sound_TestRenderVolumeBars(SDLWindow.Renderer, &test_volume_controller, VOLUME_CONTROLLER_COUNT);
                } break;
                case VOL_SETTINGS_MUSIC:
                {
                    Sound_TestUpdateVolumeBars(&test_volume_controller, MUSIC_INDEX);
                    Sound_TestRenderVolumeBars(SDLWindow.Renderer, &test_volume_controller, VOLUME_CONTROLLER_COUNT);
                } break;
                case VOL_SETTINGS_SFX:
                {
                    Sound_TestUpdateVolumeBars(&test_volume_controller, SFX_INDEX);
                    Sound_TestRenderVolumeBars(SDLWindow.Renderer, &test_volume_controller, VOLUME_CONTROLLER_COUNT);
                } break;
                case VOL_SETTINGS_APPLY:
                {
                    printf("master_volume: %d\n", test_volume_controller.info[MASTER_INDEX].volume);
                    
                    if (test_volume_controller.apply)
                    {
                        for (int i = 0; i < MUSIC_INDEX; ++i)
                        {
                            master_volume.music[MASTER_INDEX]->wav.volume = test_volume_controller.info[MASTER_INDEX].volume;
                        }
                          
                        for (int i = 0; i < SFX_INDEX; ++i)
                        {
                            master_volume.sfx[MASTER_INDEX]->wav.volume = test_volume_controller.info[MASTER_INDEX].volume;
                        }                      
                        test_volume_controller.apply = false;
                    }
                  
                    // Updates 
                    //music_volume[MASTER_INDEX].wav.volume = test_volume_controller.info[MASTER_INDEX].volume;
                    
                    /*switch (sound_settings.index)
                    {
                        case 3:
                        {           
                            // Music and SFX should not be any louder than the Master volume if set to be,
                            // so they can only be as loud as Master. The simple attempt was to set the 
                            // music and sfx volume to master's if that was the case and it works, but it 
                            // doesn't FEEL right that when I have the master volume at bar 1, it feels like
                            // nothing's happening messing with the music volume, I would 
                            // like to feel it where if I increased the volume of the music, it would go
                            // up, but relative to where the volume master's set at. The next attempt is
                            // to average the volume of music or sfx according to the master's.

                            if (test_volume_controller.apply)
                            {
                                // If master volume is touched, set every other volume level as master's
                                for (int i = 0; i < MUSIC_FILE_COUNT; ++i)
                                {
                                    master_volume.music[i].wav.volume = volume_controller[0].volume;
                                    printf("master_music: %d\n", music_volume[i].wav.volume);

                                    if (music_volume[i].wav.volume > master_volume.music[i].wav.volume)
                                    {
                                        master_volume.music[i].wav.volume = master_volume.music[i].wav.volume;
                                    }
                                }

                                for (int i = 0; i < SFX_FILE_COUNT; ++i)
                                {
                                    master_volume.sfx[i].wav.volume = volume_controller[0].volume;
                                    printf("master_sfx: %d\n", sfx_volume[i].wav.volume);

                                }

                                master_volume.music[MASTER_INDEX].wav.volume = test_volume_controller.info[MASTER_INDEX].volume;

                                test_volume_controller.apply = false;
                            }

                        }
                    }*/

                    Sound_TestRenderVolumeBars(SDLWindow.Renderer, &test_volume_controller, VOLUME_CONTROLLER_COUNT);
                } break;
                case VOL_SETTINGS_BACK:
                {
                    is_settings = false;
                    is_title_screen = true;
                    volume_settings_state = VOL_SETTINGS_NONE;
                    title_screen_state = TITLE_NONE;
                } break;
                default:
                {
                    volume_settings_state = VOL_SETTINGS_NONE;
                } break;
            }
        }

        if (is_new_game)
        {
            if (character_creation_screen.is_active)
            {
                SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);

                RenderText(SDLWindow.Renderer, font_atlas, 
                           CENTER_TEXT_X("Back", -94),
                           SCREEN_CENTER_Y - 96,
                           "Back",
                           white);

                RenderText(SDLWindow.Renderer, font_atlas, 
                           CENTER_TEXT_X("[ESC]", -64),
                           SCREEN_CENTER_Y - 96,
                           "[ESC]",
                           orange);

                for (int i = 0; i < ArraySize(character_creation_screen.info); ++i)
                {
                    RenderText(SDLWindow.Renderer, font_atlas, character_creation_screen.info[i].asset.x - 8, character_creation_screen.info[i].asset.y - 16, 
                               character_creation_screen.info[i].name, white);
                    RenderWrappedText(SDLWindow.Renderer, font_atlas, 
                                              character_creation_screen.info[character_creation_screen.index].description, white, 
                                              character_creation_screen.description_box.x + 2,
                                              character_creation_screen.description_box.y + 8, // containerX, containerY
                                              character_creation_screen.description_box.w, 
                                              character_creation_screen.description_box.h,    // containerW, containerH
                                              4);          // lineSpacing

                    RenderAndUpdateAsset(&character_creation_screen.info[i].asset);
                }
                
                CursorForAssets(&character_creation_screen.info[character_creation_screen.index].asset, &up_cursor_asset, 4, 16);
                RenderAndUpdateAsset(&up_cursor_asset);

                SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(SDLWindow.Renderer, &character_creation_screen.description_box);
            
                if (!confirmation.is_active)
                {
                    switch (class_select_state)
                    {
                        case CLASS_SELECT_KNIGHT:
                        {
                            PushString(character_data.class.name, "Knight");
                            character_data.model = character_creation_screen.info[KNIGHT_ID].asset;
                            character_data.model.conditions.is_collidable = true;
                            character_data.model.conditions.is_movable = true;
                            character_data.model.direction.up = false;
                            character_data.model.direction.down = false;
                            character_data.model.direction.left = false;
                            character_data.model.direction.right = false;

                            character_data.base_stats = class_base_stats[KNIGHT_ID];
                            confirmation.is_active = true; 
                        } break;
                        case CLASS_SELECT_PALADIN:
                        {
                            PushString(character_data.class.name, "Paladin");
                            character_data.model = character_creation_screen.info[PALADIN_ID].asset;
                            character_data.base_stats = class_base_stats[PALADIN_ID];
                            confirmation.is_active = true; 
                        } break;
                        case CLASS_SELECT_WIZARD:
                        {
                            PushString(character_data.class.name, "Wizard");
                            character_data.model = character_creation_screen.info[WIZARD_ID].asset;
                            character_data.base_stats = class_base_stats[WIZARD_ID];
                            confirmation.is_active = true; 
                        } break;
                        case CLASS_SELECT_ARCHER:
                        {
                            PushString(character_data.class.name, "Archer");
                            character_data.model = character_creation_screen.info[ARCHER_ID].asset;
                            character_data.base_stats = class_base_stats[ARCHER_ID];
                            confirmation.is_active = true; 
                        } break;
                        case CLASS_SELECT_BACK:
                        {
                            character_creation_screen.is_active = false;
                            is_title_screen = true;
                            class_select_state = CLASS_SELECT_NONE;
                            title_screen_state = TITLE_NONE;
                        } break;
                    }
                }
        
                if (confirmation.is_active)
                {
                    // Box border
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(SDLWindow.Renderer, &confirmation.box_border);
                    // Border
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
                    SDL_RenderFillRect(SDLWindow.Renderer, &confirmation.box);

                    CursorForItems(&confirmation.info.buttons[confirmation.index], &right_cursor_asset, 4, 1);
                    RenderAndUpdateAsset(&right_cursor_asset);
                    for (int i = 0; i < ArraySize(confirmation.info.buttons); ++i) 
                    {
                        RenderText(SDLWindow.Renderer, font_atlas,
                                   confirmation.info.buttons[i].x, 
                                   confirmation.info.buttons[i].y,
                                   confirmation.info.buttons[i].text, 
                                   white);
                    }
                }

            }

            if (character_allocation_select_screen.is_active)
            {
                SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                RenderText(SDLWindow.Renderer, font_atlas, 
                           CENTER_TEXT_X("Back", -94),
                           SCREEN_CENTER_Y - 96,
                           "Back",
                           white);
                RenderText(SDLWindow.Renderer, font_atlas, 
                           CENTER_TEXT_X("[ESC]", -64),
                           SCREEN_CENTER_Y - 96,
                           "[ESC]",
                           orange);

                RenderText(SDLWindow.Renderer, font_atlas,
                           CENTER_TEXT_X("How will you allocate your", 0),
                           SCREEN_CENTER_Y - 48,
                           "How will you allocate your",
                           white);
                RenderText(SDLWindow.Renderer, font_atlas,
                           CENTER_TEXT_X("points for your character?", 0),
                           SCREEN_CENTER_Y - 40,
                           "points for your character?",
                           white);
    
                for (int i = 0; i < ArraySize(character_allocation_select_screen.info); ++i)
                {
                    RenderAndUpdateAsset(&character_allocation_select_screen.info[i].asset);
                    RenderTextWithNewlines(SDLWindow.Renderer, font_atlas, 
                               character_allocation_select_screen.info[i].asset.x, 
                               character_allocation_select_screen.info[i].asset.y, 
                               character_allocation_select_screen.info[i].name, 
                               white, 
                               2);
    
                    RenderWrappedText(SDLWindow.Renderer, font_atlas, 
                                      character_allocation_select_screen.info[character_allocation_select_screen.index].description, white, 
                                      character_allocation_select_screen.description_box.x + 2,
                                      character_allocation_select_screen.description_box.y + 8, // containerX, containerY
                                      character_allocation_select_screen.description_box.w, 
                                      character_allocation_select_screen.description_box.h,    // containerW, containerH
                                      4);          // lineSpacing

                }
                  
                CursorForAssets(&character_allocation_select_screen.info[character_allocation_select_screen.index].asset, &up_cursor_asset, 24, 20);
                RenderAndUpdateAsset(&up_cursor_asset);
                
                SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(SDLWindow.Renderer, &character_allocation_select_screen.description_box);
           
                if (!confirmation.is_active)
                {
                    switch (class_allocation_state)
                    {
                        case CLASS_ALLOCATION_PERSONALITY:
                        {
                            confirmation.is_active = true;
                        } break;
                        case CLASS_ALLOCATION_PRESET:
                        {
                            confirmation.is_active = true;
                        } break;
                        case CLASS_ALLOCATION_MANUAL:
                        {
                            confirmation.is_active = true;
                        } break;
                        case CLASS_ALLOCATION_BACK:
                        {
                            character_allocation_select_screen.is_active = false;
                            character_creation_screen.is_active = true;
                            class_allocation_state = CLASS_ALLOCATION_NONE;
                            class_select_state = CLASS_SELECT_NONE;
                        } break;
                    };
                    
                }

                if (confirmation.is_active)
                {
                    // TODO: Above the confirmation modal, prompt what the player is confirming for, ex: "You selected X, are you sure?"

                    // Box border
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(SDLWindow.Renderer, &confirmation.box_border);
                    // Border
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
                    SDL_RenderFillRect(SDLWindow.Renderer, &confirmation.box);
                    
                    CursorForItems(&confirmation.info.buttons[confirmation.index], &right_cursor_asset, 4, 1);
                    RenderAndUpdateAsset(&right_cursor_asset);
                    for (int i = 0; i < ArraySize(confirmation.info.buttons); ++i) 
                    {if (is_settings)
                            {
                                switch (sound_settings.index)
                                {
                                    case 3: // apply
                                    {
                                        Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                        test_volume_controller.apply = true;
                                        volume_settings_state = VOL_SETTINGS_APPLY;
                                    } break;
                                    case 4:
                                    {
                                        Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                        volume_settings_state = VOL_SETTINGS_BACK;
                                    } break;
                                    default:
                                    { 
                                         
                                    } break;
                                }
                            }
                        RenderText(SDLWindow.Renderer, font_atlas,
                                   confirmation.info.buttons[i].x, 
                                   confirmation.info.buttons[i].y,
                                   confirmation.info.buttons[i].text, 
                                   white);
                    }
                }
            }

        
            // Character name is the last screen where input will be entering it in letter by letter like FF8 or pokemon   
            if (is_personality_test)
            {
                static int rand_index = 0;

    
                if (question_confirmation)
                {
                    question_confirmation = false;
                    ++personality_test.index;
                    
                    static bool follow_up_question_selected = false;
                    if (!follow_up_question_selected)
                    {
                        int skip_count = 5;
                        int total = ArraySize(personality_test.table) - skip_count;
                        rand_index = skip_count + (rand() % total);
                        follow_up_question_selected = true;
                    }

                }
                else
                {
                    // Only the starting questions
                    static bool first_question_selected = false;
                    if (!first_question_selected)
                    {
                        int total = ArraySize(personality_test.table) ;
                        rand_index = rand() % 5;
                        first_question_selected = true;
                    }
                }  

                SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                RenderWrappedTextCentered(SDLWindow.Renderer, font_atlas, 
                                      personality_test.table[personality_test.index], white, 
                                      0, -32,        // containerX, containerY
                                      256, 240,    // containerW, containerH
                                      2);          // lineSpacing


                if (personality_test.is_active)
                {
                    switch (character_class_personality_test_result_state)
                    {
                        case CLASS_PERSONALITY_RESULT_NONE:
                        {
                            confirmation.is_active = true;
                        } break;
                        case CLASS_PERSONALITY_RESULT_VILLAGE:
                        {
                            scenario_name = "Village Scenario";
                            test_scenarios.result |= SCENARIO_VILLAGE;
                            test_scenarios.scenario[VILLAGE_INDEX].is_active = true;
                            personality_test.is_active = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_MONSTER:
                        {
                            scenario_name = "Monster Scenario";
                            test_scenarios.result |= SCENARIO_MONSTER;
                            test_scenarios.scenario[MONSTER_INDEX].is_active = true;
                            personality_test.is_active = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_FOREST:
                        {
                            scenario_name = "Forest Scenario";
                           
                            SetAssetPosition(&character_data.model, 128 + (16 * 16), 240 - 24);
                            SetAssetPosition(&oldman_asset, 128 + (16 * 18), 240 - 24);
                            SetAssetPosition(&boulder_asset, 128 - (16 * 4), 240 - 24);
                            SetAssetPosition(&dialogue_box_asset, SCREEN_CENTER_X, SCREEN_CENTER_Y);
                         
                            SetAssetPosition(&boulder_wall_asset[0], 128 + (16 * 20), 240);
                            SetAssetPosition(&boulder_wall_asset[1], 128 + (16 * 20), 240 - 24);
                            SetAssetPosition(&boulder_wall_asset[2], 128 + (16 * 20), 240 - 48);
                            SetAssetPosition(&boulder_wall_asset[3], 128 + (16 * 19), 240 - 12);
                            SetAssetPosition(&boulder_wall_asset[4], 128 + (16 * 19), 240 - 36);

                            InitAssetAdjacentHitBoxes(&oldman_asset);
                            
                            test_scenarios.scenario[FOREST_INDEX].is_active = true;
                            personality_test.is_active = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_CAVE:
                        {
                            scenario_name = "Cave Scenario";
                            test_scenarios.result |= SCENARIO_CAVE;
                            test_scenarios.scenario[CAVE_INDEX].is_active = true;
                            personality_test.is_active = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_DESERT:
                        {
                            scenario_name = "Desert Scenario";
                            test_scenarios.result |= SCENARIO_DESERT;
                            test_scenarios.scenario[DESERT_INDEX].is_active = true;
                            personality_test.is_active = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_TOWER:
                        {
                            scenario_name = "Tower Scenario";
                            test_scenarios.result |= SCENARIO_TOWER;
                            test_scenarios.scenario[TOWER_INDEX].is_active = true;
                            personality_test.is_active = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_THEATER:
                        {
                            scenario_name = "Theater Scenario";
                            test_scenarios.result |= SCENARIO_THEATER;
                            test_scenarios.scenario[THEATER_INDEX].is_active = true;
                            personality_test.is_active = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_CASTLE:
                        {
                            scenario_name = "Castle Scenario";
                            test_scenarios.result |= SCENARIO_CASTLE;
                            test_scenarios.scenario[CASTLE_INDEX].is_active = true;
                            personality_test.is_active = false;
                        } break;
                        default:
                        {
                            character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_NONE;
                        } break;
                    }
                }

                if (confirmation.is_active)
                {
                    CursorForItems(&confirmation.info.buttons[confirmation.index], &right_cursor_asset, 4, 1);
                    RenderAndUpdateAsset(&right_cursor_asset);
                    for (int i = 0; i < ArraySize(confirmation.info.buttons); ++i) 
                    {
                        RenderText(SDLWindow.Renderer, font_atlas,
                                   confirmation.info.buttons[i].x, 
                                   confirmation.info.buttons[i].y,
                                   confirmation.info.buttons[i].text, 
                                   white);

                    }
                }

                if (test_scenarios.scenario[VILLAGE_INDEX].is_active)
                {
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   white);

                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("Silver coins drop from the hanging bag", 0), 
                               SCREEN_CENTER_Y - 88,
                               "Silver coins drop from the hanging bag", 
                               white); 
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X(" of an elderly man's pocket as he walks", 0), 
                               SCREEN_CENTER_Y - 80,
                               " of an elderly man's pocket as he walks", 
                               white);

                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X(" through the market. What do you do?", 0), 
                               SCREEN_CENTER_Y - 72,
                               " through the market. What do you do?", 
                               white);

                    CursorForItems(&test_scenarios.scenario[VILLAGE_INDEX].options[test_scenarios.scenario[VILLAGE_INDEX].index], &right_cursor_asset, 4, 1);
                    RenderAndUpdateAsset(&right_cursor_asset);

                    for (int i = 0; i < VILLAGE_OPTION_COUNT; ++i)
                    {
                        RenderTextWithNewlines(SDLWindow.Renderer, font_atlas,
                                               test_scenarios.scenario[VILLAGE_INDEX].options[i].x,
                                               test_scenarios.scenario[VILLAGE_INDEX].options[i].y,
                                               test_scenarios.scenario[VILLAGE_INDEX].options[i].text,
                                               white,
                                               2);
                    }
                     

                }

                if (test_scenarios.scenario[MONSTER_INDEX].is_active)
                {
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   white);

                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X("You are a man by day and a beast by", 0), 
                                   SCREEN_CENTER_Y - 88,
                                   "You are a man by day and a beast by", 
                                   white);                   

                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X("night. You prey off human flesh and blood", 0), 
                                   SCREEN_CENTER_Y - 80,
                                   "night. You prey off human flesh and blood", 
                                   white);  

                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X(" to survive. You come across a small and", 0), 
                                   SCREEN_CENTER_Y - 72,
                                   " to survive. You come across a small and", 
                                   white);

                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X(" quiet village. What do you do?", 0), 
                                   SCREEN_CENTER_Y - 64,
                                   " quiet village. What do you do?", 
                                   white);

                    personality_scenario[MONSTER_INDEX].scenario_box.x = SCREEN_CENTER_X - 124; 
                    personality_scenario[MONSTER_INDEX].scenario_box.y = SCREEN_CENTER_Y - 96; 
                    personality_scenario[MONSTER_INDEX].scenario_box.w = 240; 
                    personality_scenario[MONSTER_INDEX].scenario_box.h = 64; 

                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(SDLWindow.Renderer, &personality_scenario[MONSTER_INDEX].scenario_box);
                    
                    CursorForItems(&test_scenarios.scenario[MONSTER_INDEX].options[test_scenarios.scenario[MONSTER_INDEX].index], &right_cursor_asset, 4, 1);
                    RenderAndUpdateAsset(&right_cursor_asset);

                    for (int i = 0; i < MONSTER_OPTION_COUNT; ++i)
                    {
                        RenderTextWithNewlines(SDLWindow.Renderer, font_atlas,
                                               test_scenarios.scenario[MONSTER_INDEX].options[i].x,
                                               test_scenarios.scenario[MONSTER_INDEX].options[i].y,
                                               test_scenarios.scenario[MONSTER_INDEX].options[i].text,
                                               white,
                                               2);
                    }

                }

                if (test_scenarios.scenario[FOREST_INDEX].is_active)
                {
                    if (!first_forest_entrance)
                    {
                        hold_dialogue_box = true;
                        first_forest_entrance = true;
                    }

                    UpdatePlayer(&character_data.model, &sfx_move);
                    UpdateAsset(&boulder_asset, &character_data.model); 

                    for (int i = 0; i < ArraySize(forest_scenario_walls); ++i)
                    {
                        AABB_Resolution(&character_data.model, &forest_scenario_walls[i]);
                        AABB_Resolution(&boulder_asset, &forest_scenario_walls[i]);
                    }
                    
                    for (int i = 0; i < ArraySize(boulder_wall_asset); ++i)
                    {
                        AABB_Resolution(&character_data.model, &boulder_wall_asset[i]);
                        AABB_Resolution(&boulder_asset, &boulder_wall_asset[i]);
                    }

                    AABB_Resolution(&character_data.model, &boulder_asset);
                    AABB_Resolution(&character_data.model, &oldman_asset);

                    // Boulder going out of bounds counts as ending the scene too
                    if (!player_is_out_of_bounds)
                    {
                        for (int i = 0; i < ArraySize(forest_out_of_bounds); ++i)
                        {
                            if (AABB_Detection(&character_data.model.body, &forest_out_of_bounds[i].body) ||
                                AABB_Detection(&boulder_asset.body, &forest_out_of_bounds[i].body))
                            {
                                printf("out of bounds!\n");
                                
                                if (boulder_count <= 1)
                                {
                                    personality_types_state = PERSONALITY_LAZYBONES;
                                    PushString(test_scenarios.personality, "Lazybones");
                                    loading_results = true;
                                    
                                    // Set character's x,y coords for when the game loop starts, player
                                    // is left at the position when leaving the forest scenario
                                    character_data.model.body.x = 10 * 16;
                                    character_data.model.body.y = 16 * 24;

                                    test_scenarios.scenario[FOREST_INDEX].is_active = false;
                                    is_personality_test = false;
                                    personality_results_screen = true;
                                }

                                if (boulder_count <= 5 && boulder_count >= 2)
                                {
                                    personality_types_state = PERSONALITY_SHOW_OFF;
                                    PushString(test_scenarios.personality, "Show-Off");
                                    loading_results = true;
                                    
                                    test_scenarios.scenario[FOREST_INDEX].is_active = false;
                                    is_personality_test = false;
                                    personality_results_screen = true;
                                }
                                
                                if (boulder_count <= 19 && boulder_count >= 6)
                                {
                                    personality_types_state = PERSONALITY_PLUGGER;
                                    PushString(test_scenarios.personality, "Plugger");
                                    loading_results = true;
                                    
                                    character_data.model.body.x = 10 * 16;
                                    character_data.model.body.y = 16 * 24;

                                    test_scenarios.scenario[FOREST_INDEX].is_active = false;
                                    is_personality_test = false;
                                    personality_results_screen = true;
                                }

                                if (boulder_count <= 39 && boulder_count >= 20)
                                {
                                    personality_types_state = PERSONALITY_DRUDGE;
                                    PushString(test_scenarios.personality, "Drudge");
                                    loading_results = true;
                                    
                                    character_data.model.body.x = 10 * 16;
                                    character_data.model.body.y = 16 * 24;

                                    test_scenarios.scenario[FOREST_INDEX].is_active = false;
                                    is_personality_test = false;
                                    personality_results_screen = true;
                                }

                                if (boulder_count >= 40)
                                {
                                    personality_types_state = PERSONALITY_TOUGH_COOKIE;
                                    PushString(test_scenarios.personality, "Tough-Cookie");
                                    loading_results = true;
                                    
                                    character_data.model.body.x = 10 * 16;
                                    character_data.model.body.y = 16 * 24;

                                    test_scenarios.scenario[FOREST_INDEX].is_active = false;
                                    is_personality_test = false;
                                    personality_results_screen = true;
                                }

                                player_is_out_of_bounds = true;
                            }
                        }
                    }

                    if (!player_talking_to_oldman)
                    {
                        for (int i = 0; i < ArraySize(oldman_asset.adjacent_hitboxes); ++i)
                        {
                            if (AABB_Detection(&character_data.model.body, &oldman_asset.adjacent_hitboxes[i]))
                            {
                                printf("touching\n");
                                player_talking_to_oldman = true;
                            }
                        }
                    }

                    if (!AABB_Detection(&character_data.model.body, &oldman_asset.adjacent_hitboxes[0]) &&
                        !AABB_Detection(&character_data.model.body, &oldman_asset.adjacent_hitboxes[1]) &&
                        !AABB_Detection(&character_data.model.body, &oldman_asset.adjacent_hitboxes[2]) &&
                        !AABB_Detection(&character_data.model.body, &oldman_asset.adjacent_hitboxes[3]) )
                    {
                        printf("not touching\n");
                        player_talking_to_oldman = false;
                    }

                    AttachCameraToPlayer(&character_data.model, &forest_scenario_map);
                    
                    for (int i = 0; i < ArraySize(forest_scenario_walls); ++i)
                        RenderAndUpdateAsset(&forest_scenario_walls[i]);
                    
                    for (int i = 0; i < ArraySize(forest_out_of_bounds); ++i)
                        RenderAndUpdateAsset(&forest_out_of_bounds[i]);


                    RenderAndUpdateAsset(&forest_scenario_map); 
                    RenderAndUpdateAsset(&forest_scenario_finish_line);

  
                    static char gold_coins[3];
                    if (!boulder_has_reached_end)
                    {
                        if (AABB_Detection(&boulder_asset.body, &forest_scenario_finish_line.body))
                        {
                            boulder_count++;
                            gold_count_from_old_man += 10;
                            sprintf(gold_coins, "%d", gold_count_from_old_man);
                            display_player_gold_count = true; 
    
                            Sound_PlaySFX(&sfx_attack);
                            printf("boulder count: %d\n", boulder_count);
                    
                            // "respawn" the boulder
                            SetAssetPosition(&boulder_asset, 128 - (16 * 4), 240 - 24);
                            hold_dialogue_box_return_boulder = true;
                            boulder_has_reached_end = true;
                        }
                    }
                 
                    if (display_player_gold_count)
                    {
                        RenderAssetInCameraSpace(&gold_coin_count_bg_asset, 
                                             SCREEN_CENTER_X + 88, SCREEN_CENTER_Y - 105, 
                                             gold_coin_count_bg_asset.w, gold_coin_count_bg_asset.h); 

                        RenderText(SDLWindow.Renderer, font_atlas, 
                               CENTER_TEXT_X(gold_coins, 104), SCREEN_CENTER_Y - 95,
                               gold_coins,
                               green);
                    }
                    
                   

                    if (hold_dialogue_box)
                    {
                        character_data.model.conditions.is_movable = false; 
                        RenderAssetInCameraSpace(&dialogue_box_asset, 
                                             SCREEN_CENTER_X - (128 - 32), SCREEN_CENTER_Y + 32, 
                                             dialogue_box_asset.w, dialogue_box_asset.h); 
                        RenderTextWithNewlines(SDLWindow.Renderer, font_atlas,
                                   SCREEN_CENTER_X - (128 - 56), 
                                   SCREEN_CENTER_Y + 56,
                                   forest_scenario_dialogue_intro[dialogue_index], 
                                   green, 
                                   2);
                       
                        // Ensures we're rendering the dialogue from index 0, then increment
                        // The dialogue event for returning the boulder doesn't use this because this
                        // dialogue event requires the user to press enter to start it, the other does not.
                        // Could use some more work.
                        if (!next_text)
                            next_text = true;


                    }

                    if (hold_dialogue_box_return_boulder)
                    {
                        character_data.model.conditions.is_movable = false; 
                        RenderAssetInCameraSpace(&dialogue_box_asset, 
                                             SCREEN_CENTER_X - (128 - 32), SCREEN_CENTER_Y + 32, 
                                             dialogue_box_asset.w, dialogue_box_asset.h); 
                            
                        RenderTextWithNewlines(SDLWindow.Renderer, font_atlas,
                                   SCREEN_CENTER_X - (128 - 56), 
                                   SCREEN_CENTER_Y + 56,
                                   forest_scenario_dialogue_return_the_boulder[dialogue_index_2], 
                                   green, 
                                   2);
                    }

                    RenderAndUpdateAsset(&character_data.model);
                    RenderAndUpdateAsset(&oldman_asset);
                    RenderAndUpdateAsset(&boulder_asset);

                    for (int i = 0; i < ArraySize(boulder_wall_asset); ++i)
                        RenderAndUpdateAsset(&boulder_wall_asset[i]);

                }


                if (test_scenarios.scenario[CAVE_INDEX].is_active)
                {
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   white);

                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X("You are near the end of your quest in", 0), 
                                   SCREEN_CENTER_Y - 88,
                                   "You are near the end of your quest in ", 
                                   white); 
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X("saving the princess, where the entrance", 0), 
                                   SCREEN_CENTER_Y - 80,
                                   "saving the princess, where the entrance ", 
                                   white); 
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X("to her prison is in front of you. But two", 0), 
                                   SCREEN_CENTER_Y - 72,
                                   "to her prison is in front of you. But two doors", 
                                   white); 
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X("doors fork to the left and right, a", 0), 
                                   SCREEN_CENTER_Y - 64,
                                   "doors fork to the left and right, a", 
                                   white); 
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X("door to take you deeper in and a door to", 0), 
                                   SCREEN_CENTER_Y - 56,
                                   "door to take you deeper in and a door to", 
                                   white); 
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X("a room of treasures. What do you do?", 0), 
                                   SCREEN_CENTER_Y - 48,
                                   "a room of treasures. What do you do?", 
                                   white);

   
                    test_scenarios.scenario[CAVE_INDEX].box.x = SCREEN_CENTER_X - 126;
                    test_scenarios.scenario[CAVE_INDEX].box.y = SCREEN_CENTER_Y - 98;
                    test_scenarios.scenario[CAVE_INDEX].box.w = 250;
                    test_scenarios.scenario[CAVE_INDEX].box.h = 64;

                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(SDLWindow.Renderer, &test_scenarios.scenario[CAVE_INDEX].box);


                    CursorForItems(&test_scenarios.scenario[CAVE_INDEX].options[test_scenarios.scenario[CAVE_INDEX].index], &right_cursor_asset, 4, 1);
                    RenderAndUpdateAsset(&right_cursor_asset);

                    for (int i = 0; i < CAVE_OPTION_COUNT; ++i)
                    {
                        RenderTextWithNewlines(SDLWindow.Renderer, font_atlas,
                                               test_scenarios.scenario[CAVE_INDEX].options[i].x,
                                               test_scenarios.scenario[CAVE_INDEX].options[i].y,
                                               test_scenarios.scenario[CAVE_INDEX].options[i].text,
                                               white,
                                               2);
                    }
                }

                if (test_scenarios.scenario[DESERT_INDEX].is_active)
                {
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   white);

                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("You carry with yourself a canteen of", 0), 
                               SCREEN_CENTER_Y - 88,
                               "You carry with yourself a canteen of", 
                               white);           
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("water with only a few sips worth left.", 0), 
                               SCREEN_CENTER_Y - 80,
                               "water with only a few sips worth left.", 
                               white);
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("In the harsh and unforgiving desert,", 0), 
                               SCREEN_CENTER_Y - 72,
                               "In the harsh and unforgiving desert", 
                               white);           
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("you come across two men stranded, where", 0), 
                               SCREEN_CENTER_Y - 64,
                               "you come across two men stranded, where", 
                               white);
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("one is near death from thirst.", 0), 
                               SCREEN_CENTER_Y - 56,
                               "one is near death from thirst.", 
                               white);
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("What do you do?", 0), 
                               SCREEN_CENTER_Y - 48,
                               "What do you do?", 
                               white);

                    test_scenarios.scenario[DESERT_INDEX].box.x = SCREEN_CENTER_X - 126;
                    test_scenarios.scenario[DESERT_INDEX].box.y = SCREEN_CENTER_Y - 98;
                    test_scenarios.scenario[DESERT_INDEX].box.w = 250;
                    test_scenarios.scenario[DESERT_INDEX].box.h = 64;
                    
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(SDLWindow.Renderer, &test_scenarios.scenario[DESERT_INDEX].box);

                    CursorForItems(&test_scenarios.scenario[DESERT_INDEX].options[test_scenarios.scenario[DESERT_INDEX].index], &right_cursor_asset, 4, 1);
                    RenderAndUpdateAsset(&right_cursor_asset);

                    for (int i = 0; i < DESERT_OPTION_COUNT; ++i)
                    {
                        RenderTextWithNewlines(SDLWindow.Renderer, font_atlas,
                                               test_scenarios.scenario[DESERT_INDEX].options[i].x,
                                               test_scenarios.scenario[DESERT_INDEX].options[i].y,
                                               test_scenarios.scenario[DESERT_INDEX].options[i].text,
                                               white,
                                               2);
                    }
                }

                if (test_scenarios.scenario[TOWER_INDEX].is_active)
                {
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   white);

                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("You are disoriented and awake at", 0), 
                               SCREEN_CENTER_Y - 88,
                               "You are disoriented and awake at", 
                               white);           
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("the top of a seemingly endless tower.", 0), 
                               SCREEN_CENTER_Y - 80,
                               "the top of a seemingly endless tower.", 
                               white);  
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("You see a staircase besides you that leads", 0), 
                               SCREEN_CENTER_Y - 72,
                               "You see a staircase besides you that leads", 
                               white);
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("down to the unknown. What do you do?", 0), 
                               SCREEN_CENTER_Y - 64,
                               "down to the unknown. What do you do?", 
                               white);
                                        
                    test_scenarios.scenario[TOWER_INDEX].box.x = SCREEN_CENTER_X - 126;
                    test_scenarios.scenario[TOWER_INDEX].box.y = SCREEN_CENTER_Y - 98;
                    test_scenarios.scenario[TOWER_INDEX].box.w = 250;
                    test_scenarios.scenario[TOWER_INDEX].box.h = 64;
                    
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(SDLWindow.Renderer, &test_scenarios.scenario[TOWER_INDEX].box);
                    
                    CursorForItems(&test_scenarios.scenario[TOWER_INDEX].options[test_scenarios.scenario[TOWER_INDEX].index], &right_cursor_asset, 4, 1);
                    RenderAndUpdateAsset(&right_cursor_asset);

                    for (int i = 0; i < TOWER_OPTION_COUNT; ++i)
                    {
                        RenderTextWithNewlines(SDLWindow.Renderer, font_atlas,
                                               test_scenarios.scenario[TOWER_INDEX].options[i].x,
                                               test_scenarios.scenario[TOWER_INDEX].options[i].y,
                                               test_scenarios.scenario[TOWER_INDEX].options[i].text,
                                               white,
                                               2);
                    }
                }

                if (test_scenarios.scenario[THEATER_INDEX].is_active)
                {
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   white);
             

                    // TODO: Corny scenario I think, edit later
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("You are a priest, and dressed for", 0), 
                               SCREEN_CENTER_Y - 88,
                               "You are a priest, and dressed for", 
                               white); 
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("night's stage show. You walk into", 0), 
                               SCREEN_CENTER_Y - 80,
                               "night's stage show. You walk into", 
                               white);
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("the theater and a man recognizes you", 0), 
                               SCREEN_CENTER_Y - 72,
                               "the theater and a man recognizes you", 
                               white);
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("as the town's priest. He immediately", 0), 
                               SCREEN_CENTER_Y - 64,
                               "as the town's priest. He immediately", 
                               white);
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("begs you to marry the women of life, of", 0), 
                               SCREEN_CENTER_Y - 56,
                               "begs you to marry the women of life, of", 
                               white);
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("life, of which they had only just met", 0), 
                               SCREEN_CENTER_Y - 48,
                               "life, of which they had only just met", 
                               white);
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("and he claims is love at first sight.", 0), 
                               SCREEN_CENTER_Y - 40,
                               "and he claims is love at first sight.", 
                               white);
                    RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X(" What do you do?", 0), 
                               SCREEN_CENTER_Y - 32,
                               "What do you do?", 
                               white);

                    test_scenarios.scenario[THEATER_INDEX].box.x = SCREEN_CENTER_X - 126;
                    test_scenarios.scenario[THEATER_INDEX].box.y = SCREEN_CENTER_Y - 98;
                    test_scenarios.scenario[THEATER_INDEX].box.w = 250;
                    test_scenarios.scenario[THEATER_INDEX].box.h = 80;
                    
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(SDLWindow.Renderer, &test_scenarios.scenario[THEATER_INDEX].box);
                    
                    CursorForItems(&test_scenarios.scenario[THEATER_INDEX].options[test_scenarios.scenario[THEATER_INDEX].index], &right_cursor_asset, 4, 1);
                    RenderAndUpdateAsset(&right_cursor_asset);

                    for (int i = 0; i < THEATER_OPTION_COUNT; ++i)
                    {
                        RenderTextWithNewlines(SDLWindow.Renderer, font_atlas,
                                               test_scenarios.scenario[THEATER_INDEX].options[i].x,
                                               test_scenarios.scenario[THEATER_INDEX].options[i].y,
                                               test_scenarios.scenario[THEATER_INDEX].options[i].text,
                                               white,
                                               2);
                    }

                }

                if (test_scenarios.scenario[CASTLE_INDEX].is_active)
                {
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CENTER_TEXT_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   white);
                }  
            }
    
            if (personality_results_screen)
            {
                SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                RenderText(SDLWindow.Renderer, font_atlas,
                               CENTER_TEXT_X("Personality:", 0), 
                               SCREEN_CENTER_Y - 88,
                               "Personality:", 
                               white);


                static test_personality_test_results_t PERSONALITY_RESULT = {0};
                static int SCENARIO_INDEX = -1;
                static int SCENARIO_DIALOGUE_SIZE = -1;

    
                // TODO: Use this as an example to polish the rest of opening and closing a pipeline to actiave/deactivate something
                if (loading_results)
                {
                    switch (personality_types_state)
                    {
                        // VILLAGE
                        case PERSONALITY_SHOW_OFF:
                        {
                            PERSONALITY_RESULT = show_off;
                            SCENARIO_INDEX = VILLAGE_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;

                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = show_off.colors[i];

                            
                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_SLIPPERY_DEVIL:
                        {
                            PERSONALITY_RESULT = slippery_devil;
                            SCENARIO_INDEX = VILLAGE_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = slippery_devil.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_SHRINKING_VIOLET:
                        {
                            PERSONALITY_RESULT = shrinking_violet;
                            SCENARIO_INDEX = VILLAGE_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = shrinking_violet.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                
                        // MONSTER
                        case PERSONALITY_PARAGON:
                        {
                            PERSONALITY_RESULT = paragon;
                            SCENARIO_INDEX = MONSTER_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = paragon.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_WIMP:
                        {
                            PERSONALITY_RESULT = wimp;
                            SCENARIO_INDEX = MONSTER_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = wimp.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_SPOILT_BRAT:
                        {
                            PERSONALITY_RESULT = spoilt_brat;
                            SCENARIO_INDEX = MONSTER_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = spoilt_brat.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_EGGHEAD:
                        {
                            PERSONALITY_RESULT = egghead;
                            SCENARIO_INDEX = MONSTER_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = egghead.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_KLUTZ:
                        {
                            PERSONALITY_RESULT = klutz;
                            SCENARIO_INDEX = MONSTER_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                             
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = klutz.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;

                        // DESERT 
                        case PERSONALITY_DAREDEVIL:
                        {
                            PERSONALITY_RESULT = daredevil;
                            SCENARIO_INDEX = DESERT_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = daredevil.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_IDEALIST:
                        {
                            PERSONALITY_RESULT = idealist;
                            SCENARIO_INDEX = DESERT_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                              
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = idealist.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break; 
                        case PERSONALITY_THUG:
                        {
                            PERSONALITY_RESULT = thug;
                            SCENARIO_INDEX = DESERT_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = thug.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;

                        // CAVE
                        case PERSONALITY_STRAIGHT_ARROW:
                        {
                            PERSONALITY_RESULT = straight_arrow;
                            SCENARIO_INDEX = CAVE_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                             
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = straight_arrow.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_MULE:
                        {
                            PERSONALITY_RESULT = mule;
                            SCENARIO_INDEX = CAVE_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = mule.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_NARCISSIST:
                        {
                            PERSONALITY_RESULT = narcissist;
                            SCENARIO_INDEX = CAVE_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                                                        
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = narcissist.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_SORE_LOSER:
                        {
                            PERSONALITY_RESULT = sore_loser;
                            SCENARIO_INDEX = CAVE_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = sore_loser.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;

                        // FOREST 
                        case PERSONALITY_LAZYBONES:
                        {
                            PERSONALITY_RESULT = lazybones;
                            SCENARIO_INDEX = FOREST_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                                                
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = lazybones.colors[i];
        
                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_PLUGGER:
                        {
                            PERSONALITY_RESULT = plugger;
                            SCENARIO_INDEX = FOREST_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = plugger.colors[i];

                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_DRUDGE:
                        {
                            PERSONALITY_RESULT = drudge;
                            SCENARIO_INDEX = FOREST_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                                                    
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = drudge.colors[i];

    
                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_TOUGH_COOKIE:
                        {
                            PERSONALITY_RESULT = tough_cookie;
                            SCENARIO_INDEX = FOREST_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = tough_cookie.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;

                        // TOWER
                        case PERSONALITY_DAYDREAMER:
                        {
                            PERSONALITY_RESULT = daydreamer;
                            SCENARIO_INDEX = TOWER_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                                                        
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = daydreamer.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_SOCIALITE:
                        {
                            PERSONALITY_RESULT = socialite;
                            SCENARIO_INDEX = TOWER_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = socialite.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;

                        // THEATER
                        case PERSONALITY_FREE_SPIRIT:
                        {
                            PERSONALITY_RESULT = free_spirit;
                            SCENARIO_INDEX = THEATER_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                                                      
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = free_spirit.colors[i];

  
                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_CRYBABY:
                        {
                            PERSONALITY_RESULT = crybaby;
                            SCENARIO_INDEX = THEATER_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = crybaby.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_LONE_WOLF:
                        {
                            PERSONALITY_RESULT = lone_wolf;
                            SCENARIO_INDEX = THEATER_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = lone_wolf.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_LOUT:
                        {
                            PERSONALITY_RESULT = lout;
                            SCENARIO_INDEX = THEATER_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = lout.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;

                        // CASTLE
                        case PERSONALITY_VAMP:
                        {
                            PERSONALITY_RESULT = vamp;
                            SCENARIO_INDEX = CASTLE_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                                                   
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = vamp.colors[i];

     
                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_WIT:
                        {
                            PERSONALITY_RESULT = wit;
                            SCENARIO_INDEX = CASTLE_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = wit.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_CLOWN:
                        {
                            PERSONALITY_RESULT = clown;
                            SCENARIO_INDEX = CASTLE_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = clown.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_GOOD_EGG:
                        {
                            PERSONALITY_RESULT = good_egg;
                            SCENARIO_INDEX = CASTLE_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = good_egg.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        case PERSONALITY_HAPPY_CAMPER:
                        {
                            PERSONALITY_RESULT = happy_camper;
                            SCENARIO_INDEX = CASTLE_INDEX;
                            SCENARIO_DIALOGUE_SIZE = 13;
                            
                            for (int i = 0; i < 10; ++i)
                                stat_overview_color[i] = happy_camper.colors[i];


                            PersonalityTest_InitResults(&test_scenarios, &PERSONALITY_RESULT, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                            loading_results = false;
                        } break;
                        default:
                        {
                            personality_types_state = PERSONALITY_UNUSED;
                        } break;
                    }
                }

                // Resets the camera so it doesn't affect assets to render AFTER it's been used in a condition where it's then 
                // turned off relative to it still.
                SDLCamera.X = 0;
                SDLCamera.Y = 0;

                PersonalityTest_RenderResults(&test_scenarios, font_atlas, SCENARIO_INDEX, SCENARIO_DIALOGUE_SIZE);
                
                CursorForItems(&next_button.button[next_button.index], &right_cursor_asset, 4, 1);
                RenderAndUpdateAsset(&right_cursor_asset);
                for (int i = 0; i < ArraySize(next_button.button); ++i)
                {
                    RenderText(SDLWindow.Renderer, font_atlas,
                               next_button.button[i].x,
                               next_button.button[i].y,
                               next_button.button[i].text,
                               white);
                }

                if (!next_button.is_active)
                {
                    switch (next_button.index)
                    {
                        case 0:
                        {
                            next_button.is_active = true;
                        } break;
                    }
                }
            }


            if (is_name_submission) //enter name screen
            {
                if (name_entry.is_active)
                {
                    switch (character_class_name_submission_state)
                    {
                        case CLASS_NAME_ENTER:
                        { 
                            name_entry.glyph_entered = true;
                            name_entry.is_active = false;
                        } break;
                        case CLASS_NAME_DELETE:
                        {
                            name_entry.glyph_deleted = true;
                            name_entry.is_active = false;
                        } break;
                        case CLASS_NAME_CONFIRM:
                        {
                            name_entry.name_confirmed = true;
                            name_entry.is_active = false;
                            is_class_overview_screen = true;
                        } break;
                    }
                }

                NameEntry_EnterGlyph(&name_entry, name_entry_bar, ascii_to_glyph_grid);
                NameEntry_DeleteGlyph(&name_entry, name_entry_bar);
                NameEntry_ConfirmName(&name_entry, name_entry_bar);

                SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                
                NameEntry_RenderNameUnderline(name_entry_bar, SDLWindow.Renderer, font_atlas, white);
                NameEntry_RenderName(name_entry_bar, SDLWindow.Renderer, font_atlas, white);
            
                for (int i = 0; i < ArraySize(glyph_grid); ++i)
                {
                    RenderText(SDLWindow.Renderer, font_atlas,
                            glyph_grid[i].pos.x, 
                            glyph_grid[i].pos.y,
                            glyph_grid[i].glyph, 
                            white);

                }    
                
                RenderText(SDLWindow.Renderer, font_atlas,
                           CENTER_TEXT_X("Enter your name", 0), 
                           SCREEN_CENTER_Y - 96,
                           "Enter your name", 
                           white);
                
                
                // Render after the glyph grid so it renders over rather than behind, looks decent but still considering a change
                Cursor(&right_cursor_asset, &glyph_grid[name_entry.index].pos, -4, -1);
                RenderAndUpdateAsset(&right_cursor_asset);
                RenderAsset(&character_creation_screen.info[character_creation_screen.index].asset, 
                            SCREEN_CENTER_X - (16/2), 52,
                            16, 24);
            }
        }

        if (is_class_overview_screen)
        {
          
            static char full_text[10][28];
            static int full_len;
            static int color_count;
            static int non_colored_len;
            static int base_x_arr[10];
            static int base_y_arr[10];
            static int base_x;
            static int base_y;
            static int offset_x;
            static int offset_x_arr[10];

            static char intBuffer[10][28]; // Enough to hold any 32-bit integer string representation.
            if (init_name)
            {
                char buffer[28];
                char *name = NameEntry_GetName(&name_entry);
                char *class_name = character_data.class.name;
                char *class_personality = test_scenarios.personality;
                StatOverview_Init(buffer, &class_status_overview[0], name);
                StatOverview_Init(buffer, &class_status_overview[2], class_name);
                StatOverview_Init(buffer, &class_status_overview[3], class_personality);

                sprintf(intBuffer[0], "%d", character_data.base_stats.strength);
                sprintf(intBuffer[1], "%d", character_data.base_stats.resilience);
                sprintf(intBuffer[2], "%d", character_data.base_stats.agility);
                sprintf(intBuffer[3], "%d", character_data.base_stats.stamina);
                sprintf(intBuffer[4], "%d", character_data.base_stats.wisdom);
                sprintf(intBuffer[5], "%d", character_data.base_stats.luck);
                sprintf(intBuffer[6], "%d", character_data.base_stats.max_hp);
                sprintf(intBuffer[7], "%d", character_data.base_stats.max_mp);
                sprintf(intBuffer[8], "%d", character_data.base_stats.attack);
                sprintf(intBuffer[9], "%d", character_data.base_stats.defense);
                
                StatOverview_Init(buffer, &class_status_overview[4], intBuffer[0]);
                StatOverview_Init(buffer, &class_status_overview[5], intBuffer[1]);
                StatOverview_Init(buffer, &class_status_overview[6], intBuffer[2]);
                StatOverview_Init(buffer, &class_status_overview[7], intBuffer[3]);
                StatOverview_Init(buffer, &class_status_overview[8], intBuffer[4]);
                StatOverview_Init(buffer, &class_status_overview[9], intBuffer[5]);
                StatOverview_Init(buffer, &class_status_overview[10], intBuffer[6]);
                StatOverview_Init(buffer, &class_status_overview[11], intBuffer[7]);
                StatOverview_Init(buffer, &class_status_overview[12], intBuffer[8]);
                StatOverview_Init(buffer, &class_status_overview[13], intBuffer[9]);
     
                full_len = strlen(class_status_overview[4].text);
                color_count = 3;
                non_colored_len = full_len - color_count;
                if (non_colored_len < 0)
                    non_colored_len = 0;     

                for (int i = 0, j = 4; i < 10 && j < 14; ++i, ++j)
                {

                    PushString(full_text[i], class_status_overview[j].text);

                    base_x_arr[i] = class_status_overview[j].pos.x;
                    base_y_arr[i] = class_status_overview[j].pos.y;

                    offset_x_arr[i] = base_x_arr[i] + non_colored_len * GLYPH_WIDTH;
                }

                init_name = false;
            }

            SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                            
            CursorForItems(&next_button.button[next_button.index], &right_cursor_asset, 4, 1);
            RenderAndUpdateAsset(&right_cursor_asset);
            for (int i = 0; i < ArraySize(next_button.button); ++i)
            {
                RenderText(SDLWindow.Renderer, font_atlas,
                           next_button.button[i].x,
                           next_button.button[i].y,
                           next_button.button[i].text,
                           white);
            }

                
            if (!next_button.is_active)
            {
                switch (next_button.index)
                {
                    case 0:
                    {
                        next_button.is_active = true;
                    } break;
                }
            }
            

            for (int i = 0; i < ArraySize(class_status_overview); ++i)
            {
                RenderText(SDLWindow.Renderer, font_atlas,
                           class_status_overview[i].pos.x,
                           class_status_overview[i].pos.y,
                           class_status_overview[i].text,
                           white);
            }

            for (int i = 0; i < 10; ++i)
            {
                RenderText(SDLWindow.Renderer, font_atlas,
                       offset_x_arr[i],
                       base_y_arr[i],
                       full_text[i] + non_colored_len,
                       stat_overview_color[i]);
            }
            

            RenderAsset(&character_creation_screen.info[character_creation_screen.index].asset, 
                         (SCREEN_CENTER_X - (32/2)) + 32, SCREEN_CENTER_Y, 
                         32, 48);

        }
        
        if (is_game_running)
        {
            UpdatePlayer(&character_data.model, &sfx_move);
            for (int i = 0; i < ArraySize(walls); ++i)
                AABB_Resolution(&character_data.model, &walls[i]);

            AttachCameraToPlayer(&character_data.model, &room_asset[0]);

            SDL_SetRenderTarget(SDLWindow.Renderer, SDLCamera.TargetTexture);
            SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
            SDL_RenderClear(SDLWindow.Renderer);

            RenderAndUpdateAsset(&room_asset[0]);
            RenderAndUpdateAsset(&down_stairs_asset);
            RenderAndUpdateAsset(&character_data.model);

            for (int i = 0; i < ArraySize(walls); ++i)
                RenderAndUpdateAsset(&walls[i]);

            // Set render target to camera
            SDL_SetRenderTarget(SDLWindow.Renderer, NULL);
            SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
            SDL_RenderClear(SDLWindow.Renderer);
        
            // Render to camera
            SDL_RenderCopy(SDLWindow.Renderer, SDLCamera.TargetTexture, NULL, NULL);
        }

        SDL_RenderSetLogicalSize(SDLWindow.Renderer, ASPECT_WIDTH, ASPECT_HEIGHT);    
        SDL_RenderPresent(SDLWindow.Renderer);
    }

    DestroyCamera(); 
    for (int i = 0; i < 3; ++i)
    {
        DestroyAssets(&enemy_arr[i]);
        DestroyAssets(&enemy_arr2[i]);
    }
    DestroyAssets(&room_asset[0]);
    DestroyAssets(&player_asset);
    DestroyAssets(&right_cursor_asset);
    SDL_DestroyTexture(font_atlas);


    SDL_CloseAudioDevice(title_screen_theme.device_id);
    SDL_CloseAudioDevice(ambience.device_id);
    SDL_FreeWAV(title_screen_theme.buffer);
    SDL_FreeWAV(ambience.buffer);
    SDL_FreeWAV(sfx_attack.buffer);
    SDL_DestroyRenderer(SDLWindow.Renderer);
    SDL_DestroyTexture(SDLWindow.Texture);
    SDL_DestroyWindow(SDLWindow.Window);

    SDL_Quit();
    return 0;
}


