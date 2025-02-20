#include "common.h"
#include "gui.h"
#include "global_states.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <SDL2/SDL.h>

#define WINDOW_WIDTH  480
#define WINDOW_HEIGHT 464

#define ASPECT_WIDTH  256
#define ASPECT_HEIGHT 240

#define GLYPH_WIDTH  6
#define GLYPH_HEIGHT 5
#define NUM_GLYPHS ArraySize(GUIFontData)

static bool Running;
#define ArraySize(array) ( sizeof(array) / sizeof((array)[0]) )
#define ArraySize2D(array) ( sizeof((array)[0]) / sizeof((array)[0][0]) )
#define CenterRenderedText(x_screen_center, text, offset) ( x_screen_center - ((GLYPH_WIDTH * strlen(text)) / 2) + offset )

struct
{
    bool up, down, left, right;
} Orientation;

typedef struct window_t 
{
    SDL_Event e;
    SDL_Window *Window;
    SDL_Renderer *Renderer;
    SDL_Texture *Texture;
} window_t;
window_t SDLWindow;

typedef struct camera_t
{
    int X, Y, W, H;
    int TargetWidth, TargetHeight;
    SDL_Texture *TargetTexture;
} camera_t;
camera_t SDLCamera;

typedef struct base_abilities_t
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

#define NUM_ENEMIES 2
typedef struct asset_t
{
    //bool collidable;
    int x, y, w, h;
    stats_t stats;
    union 
    { 
        bool is_moving;
        bool is_under_attack;
        bool is_attacking;
        bool is_defending;
        bool is_using_item;
        bool is_collidable;
    } conditions;
    SDL_Rect body;
    SDL_Texture *texture;
} asset_t;

typedef struct wav_t
{
    bool reset_audio;
    int volume;
    u32 offset;
    u32 length;
    u8 *buffer;
    SDL_AudioSpec spec;
    SDL_AudioDeviceID device_id;
} wav_t;

typedef struct music_t
{
    int volume;
    wav_t music; 
} music_t;

typedef struct sfx_t
{
    int volume;
    wav_t sfx; 
} sfx_t;

typedef struct master_volume_t
{
    // any audio output will only go as high as the master volume,
    // if the sfx audio was maxed but master volume was set to 10%, it'll
    // only be 10% as strong though it's maxed 
    
    int volume;

    // array size will stay to be hardcoded in, why tf malloc?
    music_t music[2]; 
    sfx_t sfx[2];
} master_volume_t;


static void
LoadWavFile(wav_t *wav, const char *file_name)
{
    SDL_LoadWAV(file_name, &wav->spec, &wav->buffer, &wav->length);
    wav->device_id = SDL_OpenAudioDevice(NULL, 0, &wav->spec, NULL, 0);   
}

static void
PlayMusic(wav_t *sound)
{
    if (SDL_GetQueuedAudioSize(sound->device_id) == 0)
    {
        SDL_QueueAudio(sound->device_id, sound->buffer, sound->length);            
    }
    SDL_PauseAudioDevice(sound->device_id, 0);
}

typedef struct mix_audio_t
{
    SDL_AudioFormat format;
    u8 *audio;
} mix_audio_t;

static void
PlaySFX(wav_t *sound)
{
    SDL_PauseAudioDevice(sound->device_id, 0);

    // Lock to update then unlock
    SDL_LockAudioDevice(sound->device_id);
    sound->reset_audio = true;
    SDL_UnlockAudioDevice(sound->device_id);
}

static void 
PauseAudio(wav_t *sound)
{
    SDL_PauseAudioDevice(sound->device_id, 1);
}


typedef struct text_t
{   
    int font_height;
    int font_width;
    SDL_Texture *font_atlas;
} text_t;

text_t CreateText(SDL_Texture *font_atlas)
{
    text_t text_data = {0};
    text_data.font_atlas = font_atlas;
    return text_data;
}

SDL_Texture* CreateFontAtlas(SDL_Renderer *renderer) {
    const int atlasCols = 16;
    const int atlasRows = (NUM_GLYPHS + atlasCols - 1) / atlasCols;
    const int atlasWidth  = atlasCols * GLYPH_WIDTH;
    const int atlasHeight = atlasRows * GLYPH_HEIGHT;

    // Create an RGBA surface with transparency.
    SDL_Surface *surface = SDL_CreateRGBSurface(0, atlasWidth, atlasHeight,
                                                32,
                                                0x00FF0000,
                                                0x0000FF00,
                                                0x000000FF,
                                                0xFF000000);
    if (!surface) {
        fprintf(stderr, "SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
        return NULL;
    }
    // Fill with transparent (0 alpha)
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

    // For each glyph in our font data, draw it into the surface.
    for (int glyphIndex = 0; glyphIndex < NUM_GLYPHS; glyphIndex++) {
        // Compute glyph’s position in atlas:
        int col = glyphIndex % atlasCols;
        int row = glyphIndex / atlasCols;
        int dstX = col * GLYPH_WIDTH;
        int dstY = row * GLYPH_HEIGHT;

        // Draw the glyph pixel by pixel.
        // For our font, each glyph is GLYPH_HEIGHT rows and GLYPH_WIDTH columns.
        // We assume the most significant 6 bits in each byte represent the 6 columns.
        for (int y = 0; y < GLYPH_HEIGHT; y++) {
            u1 rowData = GUIFontData[glyphIndex][y];
            for (int x = 0; x < GLYPH_WIDTH; x++) {
                // Check the bit corresponding to this column.
                if (rowData & (0x80 >> x)) {
                    // Set pixel to white (fully opaque).
                    Uint32 *pixel = (Uint32 *)((Uint8 *)surface->pixels +
                                        (dstY + y) * surface->pitch +
                                        (dstX + x) * 4);
                    *pixel = SDL_MapRGBA(surface->format, 255, 255, 255, 255);
                }
            }
        }
    }

    // Create a texture from the surface.
    SDL_Texture *fontAtlas = SDL_CreateTextureFromSurface(renderer, surface);
    if (!fontAtlas) {
        fprintf(stderr, "SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
    }
    SDL_FreeSurface(surface);
    return fontAtlas;
}

void RenderText(SDL_Renderer *renderer, SDL_Texture *fontAtlas, int x, int y,
                const char *text, SDL_Color color)
{
    // Since our atlas is white, we can tint it by setting the texture color mod.
    SDL_SetTextureColorMod(fontAtlas, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(fontAtlas, color.a);

    const int atlasCols = 16;

    // For each character in the text:
    for (const char *p = text; *p != '\0'; p++) {
        u1 ascii = (u1)*p;
        // Look up the corresponding glyph index.
        // (Assuming ASCII2Font is defined for your ASCII range.)
        u1 glyphIndex = ASCII2Font[ascii];

        // Compute source rectangle in the atlas.
        SDL_Rect src;
        src.x = (glyphIndex % atlasCols) * GLYPH_WIDTH;
        src.y = (glyphIndex / atlasCols) * GLYPH_HEIGHT;
        src.w = GLYPH_WIDTH;
        src.h = GLYPH_HEIGHT;

        // Destination rectangle on screen.
        SDL_Rect dst;
        dst.x = x;
        dst.y = y;
        dst.w = GLYPH_WIDTH;
        dst.h = GLYPH_HEIGHT;

        // Render this glyph.
        SDL_RenderCopy(renderer, fontAtlas, &src, &dst);

        // Advance the x position.
        x += GLYPH_WIDTH;
    }
}

char** WrapText(const char *text, int maxWidth, int *outLineCount)
{
    // Maximum number of characters per line for a fixed-width font:
    int maxCharsPerLine = maxWidth / GLYPH_WIDTH;

    // Copy the input string because strtok_r modifies it.
    char *copy = strdup(text);
    if (!copy) return NULL;

    // Prepare dynamic array for lines.
    int capacity = 32;
    char **lines = malloc(capacity * sizeof(*lines));
    if (!lines) {
        free(copy);
        return NULL;
    }
    int lineCount = 0;

    // Temporary line buffer and pointer to the current end.
    char lineBuffer[512];
    int bufferPos = 0;  // current length in lineBuffer
    lineBuffer[0] = '\0';

    // Use strtok_r for reentrancy.
    char *saveptr = NULL;
    char *token = strtok_r(copy, " \t\n\r", &saveptr);
    while (token) {
        int wordLen = strlen(token);
        // If lineBuffer is not empty, you need one extra character for the space.
        int needed = (bufferPos == 0) ? wordLen : (bufferPos + 1 + wordLen);

        // If the word itself is longer than maxCharsPerLine, you may want to handle that
        // separately (e.g., break the word). For now, we assume words are short enough.
        if (needed > maxCharsPerLine) {
            // Finalize the current line if not empty.
            if (bufferPos > 0) {
                lines[lineCount] = strdup(lineBuffer);
                if (!lines[lineCount]) {
                    // On allocation failure, free everything.
                    for (int i = 0; i < lineCount; i++) free(lines[i]);
                    free(lines);
                    free(copy);
                    return NULL;
                }
                lineCount++;
                if (lineCount >= capacity) {
                    capacity *= 2;
                    char **tmp = realloc(lines, capacity * sizeof(*lines));
                    if (!tmp) {
                        for (int i = 0; i < lineCount; i++) free(lines[i]);
                        free(lines);
                        free(copy);
                        return NULL;
                    }
                    lines = tmp;
                }
                // Start a new line.
                bufferPos = 0;
                lineBuffer[0] = '\0';
            }
            // Place the long word on a new line (or split it if desired)
            strncpy(lineBuffer, token, sizeof(lineBuffer) - 1);
            lineBuffer[sizeof(lineBuffer) - 1] = '\0';
            bufferPos = strlen(lineBuffer);
        } else {
            // Append the word to the current line.
            if (bufferPos == 0) {
                // First word in the line.
                snprintf(lineBuffer, sizeof(lineBuffer), "%s", token);
                bufferPos = wordLen;
            } else {
                // Append a space and then the word.
                int written = snprintf(lineBuffer + bufferPos, sizeof(lineBuffer) - bufferPos, " %s", token);
                if (written < 0 || written >= (int)(sizeof(lineBuffer) - bufferPos)) {
                    // Handle error or overflow.
                    break;
                }
                bufferPos += written;
            }
        }
        token = strtok_r(NULL, " \t\n\r", &saveptr);
    }

    // If there's any remaining text in the buffer, push it as the last line.
    if (bufferPos > 0) {
        lines[lineCount] = strdup(lineBuffer);
        if (!lines[lineCount]) {
            for (int i = 0; i < lineCount; i++) free(lines[i]);
            free(lines);
            free(copy);
            return NULL;
        }
        lineCount++;
    }

    // Null-terminate the array.
    lines[lineCount] = NULL;
    free(copy);
    *outLineCount = lineCount;
    return lines;
}

void RenderWrappedTextCentered(SDL_Renderer *renderer, SDL_Texture *fontAtlas,
                               const char *text, SDL_Color color,
                               int containerX, int containerY,
                               int containerW, int containerH,
                               int lineSpacing /* extra spacing between lines */)
{
    // 1) Wrap the text
    int lineCount = 0;
    char **lines = WrapText(text, containerW, &lineCount);
    if (!lines) return;

    // 2) Calculate total text block height
    //    Each line is GLYPH_HEIGHT tall + lineSpacing (except maybe the last)
    int totalHeight = lineCount * GLYPH_HEIGHT + (lineCount - 1) * lineSpacing;

    // 3) Compute the top coordinate so the block is vertically centered
    int startY = containerY + (containerH - totalHeight) / 2;

    // 4) For each line, center it horizontally and render
    int currentY = startY;
    for (int i = 0; i < lineCount; i++) {
        char *line = lines[i];
        int lineLen = strlen(line);
        int lineWidth = lineLen * GLYPH_WIDTH;

        // Horizontal center
        int x = containerX + (containerW - lineWidth) / 2;
        int y = currentY;

        // Render the line
        RenderText(renderer, fontAtlas, x, y, line, color);

        currentY += GLYPH_HEIGHT + lineSpacing;
        free(line); // free each line after rendering
    }

    free(lines); // free the array of line pointers
}

void RenderWrappedText(SDL_Renderer *renderer, SDL_Texture *fontAtlas,
                       const char *text, SDL_Color color,
                       int containerX, int containerY,
                       int containerW, int containerH,
                       int lineSpacing)
{
    // Wrap the text into lines that fit within containerW.
    int lineCount = 0;
    char **lines = WrapText(text, containerW, &lineCount);
    if (!lines) return;
    
    // Start rendering at containerY.
    int currentY = containerY;
    
    // For each wrapped line, render it left-aligned at containerX.
    for (int i = 0; i < lineCount; i++) {
        char *line = lines[i];
        // RenderText is assumed to render a single line of text at (x,y).
        RenderText(renderer, fontAtlas, containerX, currentY, line, color);
        currentY += GLYPH_HEIGHT + lineSpacing;
        free(line); // free each allocated line
    }
    free(lines);
}


void RenderTextWithNewlines(SDL_Renderer *renderer, SDL_Texture *fontAtlas,
                            int startX, int startY,
                            const char *text, SDL_Color color, int lineSpacing)
{
    // Make a copy of the text because strtok modifies the string.
    char *copy = strdup(text);
    if (!copy) return;

    int x = startX;
    int y = startY;
    
    // Use strtok to split the text by newline.
    char *line = strtok(copy, "\n");
    while (line) {
        // Render the current line at (x, y).
        RenderText(renderer, fontAtlas, x, y, line, color);
        // Move y down for the next line (add line spacing if desired).
        y += GLYPH_HEIGHT + lineSpacing;
        // Get the next line.
        line = strtok(NULL, "\n");
    }
    
    free(copy);
}

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

typedef struct menu_item_t
{
    const char *text;
    int x, y;
} menu_item_t;

void CursorForItems(menu_item_t *menu_items, asset_t *cursor, int x_offset, int y_offset)
{
    cursor->body.x = menu_items->x - cursor->w - x_offset;
    cursor->body.y = menu_items->y + (GLYPH_HEIGHT / 2) - (cursor->h / 2) - y_offset; // Subtracting by 1 at the end is the center the cursor
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

static void LoadAsset(asset_t *asset, const char *filename)
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

static bool AABB(SDL_Rect *a, SDL_Rect *b)
{
    if(a->y + a->h <= b->y) return false;

    if(a->y >= b->y + b->h) return false;

    if(a->x + a->w <= b->x) return false;

    if(a->x >= b->x + b->w) return false;

    return true;
}

static inline void 
CollisionXCheck(asset_t *player, asset_t *assets)
{
    if (AABB(&player->body, &assets->body) && assets->conditions.is_collidable)
    {
        player->body.x = player->x;
    }
}  

static inline void 
CollisionYCheck(asset_t *player, asset_t *assets)
{
    if (AABB(&player->body, &assets->body) && assets->conditions.is_collidable)
    {
        player->body.y = player->y;
    }
}

static void 
CombatCheck(asset_t *player, asset_t* asset)
{
    if (AABB(&player->body, &asset->body) && asset->conditions.is_collidable)
    {
        player->conditions.is_attacking = true;
        asset->conditions.is_under_attack = true;
    }
}

static void
CombatUpdate(asset_t *player, asset_t *asset, wav_t *sound) 
{
    if (asset->conditions.is_under_attack)
    {
        if (asset->stats.hp <= 0)
        {
            asset->conditions.is_collidable = false;
            player->stats.exp += asset->stats.exp;
            printf("Player has earned: %d exp\n", player->stats.exp);

            SDL_DestroyTexture(asset->texture);
            asset->texture = NULL;    

        }
        PlaySFX(sound);
        asset->stats.hp -= player->stats.atk;
        printf("enemy took %d damage \n", player->stats.atk);
    }
}

void UpdatePlayer(asset_t *player, asset_t *enemy, wav_t *sound)
{
    player->x = player->body.x; 
    player->y = player->body.y; 

    if (Orientation.up)
    {       
        player->body.y -= player->body.h;
        PlaySFX(sound);
        Orientation.up = false;
    }
    if (Orientation.left)
    {
        player->body.x -= player->body.w; 
        PlaySFX(sound);
        Orientation.left = false;
    }
    if (Orientation.down)
    {
        player->body.y += player->body.h; 
        PlaySFX(sound);
        Orientation.down = false;
    }
    if (Orientation.right)
    {
        player->body.x += player->body.w; 
        PlaySFX(sound);
        Orientation.right = false;
    }
}

void InitializeAssetStats(asset_t *assets, stats_t *stats)
{
    assets->stats.hp = stats->hp;
    assets->stats.atk = stats->atk;
    assets->stats.def = stats->def;
    assets->stats.exp = stats->exp;

    assets->conditions.is_collidable = true;
}

void InitializeAssetToRender(asset_t *assets, int x, int y, int w, int h)
{
    assets->body.x = x;
    assets->body.y = y;
    assets->body.w = w;
    assets->body.h = h;
}

void RenderAssetHealthBar(asset_t *assets)
{
    assets->stats.health_bar;
    assets->stats.health_bar.x = (assets->body.x) - SDLCamera.X;
    assets->stats.health_bar.y = (assets->body.y - 4) - SDLCamera.Y;

    float max_hp = (float)assets->stats.hp;
    float hp_ratio = (float)assets->stats.hp / max_hp; 
    assets->stats.health_bar.w = (int)(assets->body.w * hp_ratio);
    assets->stats.health_bar.h = 4;

    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(SDLWindow.Renderer, &assets->stats.health_bar);
}

void RenderAndUpdateAsset(asset_t *enemies)
{
    enemies->x = enemies->body.x;
    enemies->y = enemies->body.y;
    enemies->body.x -= SDLCamera.X;
    enemies->body.y -= SDLCamera.Y;
    //SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 0, 0, 255);
    //SDL_RenderDrawRect(SDLWindow.Renderer, &enemies->body);

    if (enemies->texture)
    {
        SDL_SetTextureBlendMode(enemies->texture, SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(SDLWindow.Renderer, enemies->texture, NULL, &enemies->body) < 0)
        {
            fprintf(stderr, "Failed to render asset: %s\n", SDL_GetError());
            return;
        }
    }
  
    enemies->body.x = enemies->x;
    enemies->body.y = enemies->y;

    if (enemies->stats.hp)
    {
        RenderAssetHealthBar(enemies);
    }
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

typedef struct
{
    u32 r, g, b, a;
} color_t;

typedef struct
{
    bool mute;
    bool one;
    bool two;
    bool three;
    bool max;
    bool apply;

    int index;
    int volume_size_bars;
    int volume;
    color_t colors;
    SDL_Rect blocks[4];
} volume_controller_t;

void UpdateAndRenderVolumeBars(volume_controller_t *volume_controller)
{
    switch (volume_controller->index)
    {
        case 0:
        {
            if (volume_controller->mute)
            {
                volume_controller->volume = 0;
                volume_controller->mute = false;
            }
        } break;
        case 1:
        {
            volume_controller->colors.r = 0;
            volume_controller->colors.g = 255;
            volume_controller->colors.b = 0;
            volume_controller->colors.a = 255;
            
            if (volume_controller->one)
            {
                volume_controller->volume = 32;
                volume_controller->one = false;
            }
        } break;
        case 2:
        {
            volume_controller->colors.r = 255;
            volume_controller->colors.g = 255;
            volume_controller->colors.b = 0;
            volume_controller->colors.a = 255;

            if (volume_controller->two)
            {
                volume_controller->volume = 64;
                volume_controller->two = false;
            }
        } break;
        case 3:
        {
            volume_controller->colors.r = 255;
            volume_controller->colors.g = 165;
            volume_controller->colors.b = 0;
            volume_controller->colors.a = 255;

            if (volume_controller->three)
            {
                volume_controller->volume = 96;
                volume_controller->three = false;
            }
        } break;
        case 4:
        {
            volume_controller->colors.r = 255;
            volume_controller->colors.g = 0;
            volume_controller->colors.b = 0;
            volume_controller->colors.a = 255;
            
            if (volume_controller->max)
            {
                volume_controller->volume = SDL_MIX_MAXVOLUME;
                volume_controller->max = false;
            }
        } break;
        default:
        {
            
        } break;
    }

    // Render updated volume blocks and its color
    /*SDL_SetRenderDrawColor(SDLWindow.Renderer, 
                           volume_controller->colors.r, 
                           volume_controller->colors.g, 
                           volume_controller->colors.b, 
                           volume_controller->colors.a);*/

    for (int i = 0; i < volume_controller->index; ++i)
    {
        //SDL_RenderFillRect(SDLWindow.Renderer, &volume_controller->blocks[i]);
    }
}

void Audio_SFXCallback(void *user_data, u8 *stream, int length)
{
    wav_t *context = (wav_t *)user_data;

    // Clear the output buffer.
    SDL_memset(stream, 0, length);

    if (context->reset_audio) {
        context->offset = 0;
        context->reset_audio = false;
    }

    // Calculate how many bytes remain in the buffer.
    u32 remaining = context->length - context->offset;
    if ((u32)length > remaining) {
        // Mix only the remainder of the audio buffer.
        SDL_MixAudioFormat(stream, context->buffer + context->offset,
                           context->spec.format, remaining, context->volume);
        // Set offset to the end, so we don't loop.
        context->offset = context->length;
    } else {
        // Mix normally, with no wrap-around.
        SDL_MixAudioFormat(stream, context->buffer + context->offset,
                           context->spec.format, length, context->volume);
        context->offset += length;
        if (context->offset >= context->length) {
            context->offset = context->length;
        }
    }

    //printf("SFX played, offset now: %u\n", context->offset);
}

void Audio_MusicCallback(void *user_data, u8 *stream, int length)
{
    wav_t *context = (wav_t *)user_data;

    // Clear the output buffer.
    SDL_memset(stream, 0, length);

    // Calculate how many bytes remain in the buffer before wrap-around.
    u32 remaining = context->length - context->offset;
    if ((u32)length > remaining) {
        // Mix the remainder of the audio buffer.
        SDL_MixAudioFormat(stream, context->buffer + context->offset,
                           context->spec.format, remaining, context->volume);
        // Then wrap around and mix the beginning of the buffer.
        SDL_MixAudioFormat(stream + remaining, context->buffer,
                           context->spec.format, length - remaining, SDL_MIX_MAXVOLUME);
        context->offset = length - remaining;
    } else {
        // No wrap-around needed.
        SDL_MixAudioFormat(stream, context->buffer + context->offset,
                           context->spec.format, length, context->volume);
        context->offset += length;
        if (context->offset >= context->length) {
            context->offset = 0;
        }
    }

    //printf("MUSIC\n");
}

static void
LoadWavFileTest(wav_t *wav, const char *filename, bool music)
{
    // Must convert the raw audio data to match the system's sample rate 
    SDL_AudioSpec loadedSpec;
    u8 *loadedBuffer = NULL;
    u32 loadedLength = 0;

    if (SDL_LoadWAV(filename, &loadedSpec,
                &loadedBuffer, &loadedLength) == NULL)
    {
        SDL_Log("Failed to load wav %s\n", SDL_GetError());
        return;
    }
    
    SDL_AudioSpec desiredSpec;
    SDL_zero(desiredSpec);
    desiredSpec.freq = 48000;             // desired frequency
    desiredSpec.format = AUDIO_S16LSB;      // desired sample format
    desiredSpec.channels = 2;             // desired number of channels
    desiredSpec.samples = 512;            // desired buffer size
                    
    // TEMP
    if (music)
    {
        desiredSpec.callback = Audio_MusicCallback;  // our callback function
    }
    else
    {
        desiredSpec.callback = Audio_SFXCallback;  // our callback function
    }
    
    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt,
                          loadedSpec.format, loadedSpec.channels, loadedSpec.freq,
                          desiredSpec.format, desiredSpec.channels, desiredSpec.freq) < 0) {
        SDL_Log("SDL_BuildAudioCVT failed: %s\n", SDL_GetError());
        SDL_FreeWAV(loadedBuffer);
        SDL_Quit();
        return;
    }
   
    // Allocate a buffer for the converted audio data.
    cvt.len = loadedLength;
    cvt.buf = (Uint8 *)malloc(cvt.len * cvt.len_mult);
    if (!cvt.buf) {
        SDL_Log("Failed to allocate conversion buffer\n");
        SDL_FreeWAV(loadedBuffer);
        SDL_Quit();
        return;
    }
    
    // Copy the loaded data into the conversion buffer.
    memcpy(cvt.buf, loadedBuffer, loadedLength);
    SDL_FreeWAV(loadedBuffer); // free the original loaded buffer

    // Convert the audio data.
    if (SDL_ConvertAudio(&cvt) < 0) {
        SDL_Log("SDL_ConvertAudio failed: %s\n", SDL_GetError());
        free(cvt.buf);
        SDL_Quit();
        return;
    }
   
    u32 convertedLength = cvt.len_cvt;
    printf("Converted audio length: %u bytes\n", convertedLength);

    wav->volume = 16;
    wav->offset = 0;   
    wav->length = convertedLength;   
    wav->buffer = cvt.buf;   
    wav->spec = desiredSpec;  
    wav->spec.userdata = wav;  

    SDL_AudioSpec obtained;
    wav->device_id = SDL_OpenAudioDevice(NULL, 0, &wav->spec, &obtained, 0);
    if (wav->device_id == 0)
    {
        SDL_Log("Failed to open audio: %s\n", SDL_GetError());
        return;
    }
}


#define NAME_LIMIT 10
// general push and pop function
void PushCharStack(char *array, char value)
{
    int size = strlen(array);
    int limit = NAME_LIMIT;
    printf("size: %d - char: %c\n", size, value);
    if (limit > 0)
    {
        limit--; // do we decrement first? i'm thinking we do because we need to have a pocket of space first, otherwise fail
        array[size] = value;
        array[size + 1] = '\0';
    }
    
    // TODO: limit needs to be out of this function's scope to consider 0
    if (limit == 0)
    {
        printf("Name limit!\n");
    }
}

void PopStack(char *array)
{
    int size = strlen(array);
    if (size == 0)
    {
        printf("Cannot pop an empty string.\n");
        return;
    }

    char popped = array[size - 1];
    printf("popped: %c\n", popped);

    array[size - 1] = '\0';
}

// small helper function
void PushString(char *dest, char *src)
{
    int index = 0;
    int size = strlen(src);
    for (int i = 0; i < size; ++i)
    {
        dest[index] = src[i];
        dest[index + 1] = '\0';
        ++index;
    }
}

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

typedef struct
{
    // Could pack with a set of bools but would might be better and cleaner using each bit as a flag to enable or disable a scenario
    u8 result; 
    menu_item_t options[5];
    menu_item_t village_options[3];
    menu_item_t monster_options[5];
} scenario_t;

typedef struct
{
    int index; 
    bool is_active;    
    scenario_t scenario[8];

} test_personality_scenario_t; 

typedef struct
{
    const char text[5][100];
    int x[5];
    int y[5];
} define_scenario_t;

#define VILLAGE   0
#define MONSTER_1 1

#define MONSTER 0 // 0 for now

typedef enum
{
    SCENARIO_NONE,
    SCENARIO_VILLAGE    = (1 << 0),
    SCENARIO_MONSTER    = (1 << 1),
    SCENARIO_FOREST     = (1 << 2),
    SCENARIO_CAVE       = (1 << 3),
    SCENARIO_DESERT     = (1 << 4),
    SCENARIO_TOWER      = (1 << 5),
    SCENARIO_THEATER    = (1 << 6),
    SCENARIO_CASTLE     = (1 << 7),
} scenario_results;

define_scenario_t define_scenario[2] = {
    {
        // Text
        {
            {"Kill fewer than three people"},
            {"Kill three or more people, \nincluding women and the elderly, \nbut don't kill the children"},
            {"Kill three or more people, \nbut don't kill the women, \nthe elderly, or children"},
            {"Kill three or more people, \nincluding women and the elderly, \nbut don't kill the children"},
            {"Kill three or more people, \nincluding women and the elderly, \nbut don't kill the children"},
        },
        
        // X positions
        {1, 1, 1, 1, 1},
        
        // Y positions
        {1, 1, 1, 1, 1},

    },
    {
        // Text
        {
            {"Kill fewer than three people"},
            {"Kill three or more people, \nincluding women and the elderly, \nbut don't kill the children"},
            {"Kill three or more people, \nbut don't kill the women, \nthe elderly, or children"},
            {""},
            {""},
        },
        
        // X positions
        {2, 2, 2, 2, 2},
        
        // Y positions
        {2, 2, 2, 2, 2},

    }
};

const char village_scenario_options[3][100] = {
    {"Kill fewer than three people"},
    {"Kill three or more people, \nincluding women and the elderly, \nbut don't kill the children"},
    {"Kill three or more people, \nbut don't kill the women, \nthe elderly, or children"},
}; 

const char monster_scenario_options[5][100] = {
    {"Kill fewer than three people"},
    {"Kill three or more people, \nincluding women and the elderly, \nbut don't kill the children"},
    {"Kill three or more people, \nbut don't kill the women, \nthe elderly, or children"},
    {"Kill three or more poeple, \nincluding children"},
    {"Kill nine or more people, but \ndon't kill the man by the inn"}
};

int Scenario_GetNumOptions(scenario_results scenario)
{
    int results = 0;

    switch (scenario)
    {
        case VILLAGE:
        {
            results = 3;
        } break;
        case MONSTER_1:
        {
            results = 5;
        } break;
        default:
        {
            results = 0;
        } break;
    }


    return results;
}

void Personality_ScenarioInitialize(test_personality_scenario_t *personality_scenario, 
                                    const char scenario_options[][100],
                                    scenario_results scenario) 
{
    int num_options = Scenario_GetNumOptions(scenario);
    printf("num_options: %d\n", num_options);

    for (int i = 0; 
         i < num_options && 
         i < ArraySize(personality_scenario->scenario[scenario].options); 
         ++i)
    {
        personality_scenario->scenario[scenario].options[i].text = define_scenario[1].text[i];
        personality_scenario->scenario[scenario].options[i].x = define_scenario[1].x[i];
        personality_scenario->scenario[scenario].options[i].y = define_scenario[1].y[i];
    }

    for (int i = 0; i < num_options; ++i)
    {
        printf("scenario_text: %s\n", personality_scenario->scenario[scenario].options[i].text);
        printf("scenario_x: %d\n", personality_scenario->scenario[scenario].options[i].x);
        printf("scenario_y: %d\n", personality_scenario->scenario[scenario].options[i].y);
    }

}

// int screen_x_pos[5], int screen_y_pos[5], int x_offset[5], scenario_results scenario)


int main(int argc, char *argv[])
{
    srand(time(NULL));

    char open[2] = "";
    char close[2] = "fe";
    PushString(open, close);
    PopStack(open);

    printf("%s\n", open);


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

    wav_t title_screen_theme = {0};
    wav_t ambience = {0};
    wav_t sfx_attack = {0};
    wav_t sfx_move = {0};
    wav_t sfx_option = {0};

    LoadWavFile(&title_screen_theme, "assets/music/6. Simpler Times.wav");
    LoadWavFile(&ambience, "assets/music/8. Temple of Tomb.wav");
    LoadWavFile(&sfx_attack, "assets/sfx/attack.wav");
    LoadWavFile(&sfx_move, "assets/sfx/move.wav");
    LoadWavFile(&sfx_option, "assets/sfx/option.wav");

    mix_audio_t theme_title_mixed = {0};
    theme_title_mixed.format = AUDIO_S16LSB;
    theme_title_mixed.audio = malloc(title_screen_theme.length); // Free
    if (!theme_title_mixed.audio)
    {
        fprintf(stderr, "Failed in creating a new sound buffer.\n");
        return 1;
    }

    music_t music_volume[2] = {0};
    LoadWavFileTest(&music_volume[0].music, "assets/music/5. Smooth As Glass.wav", true);
    LoadWavFileTest(&music_volume[1].music, "assets/music/4. Church of Order.wav", true);

    sfx_t sfx_volume_test[2] = {0};
    LoadWavFileTest(&sfx_volume_test[0].sfx, "assets/sfx/fire_a.wav", false); 
    LoadWavFileTest(&sfx_volume_test[1].sfx, "assets/sfx/fire_b.wav", false); 


    // 2 different sfx "files"
    sfx_t sfx_volume[2] = {0};
    LoadWavFile(&sfx_volume[0].sfx, "assets/sfx/fire_a.wav"); 
    LoadWavFile(&sfx_volume[1].sfx, "assets/sfx/fire_b.wav"); 


    master_volume_t master_volume = {0};
    master_volume.volume = 32;
    for (int i = 0; i < 2; ++i)
    {
        master_volume.music[i].music = music_volume[i].music;
        master_volume.sfx[i] = sfx_volume[i];
    }

    // master volume has control of music's volume
    mix_audio_t mix = {0};
    mix.format = AUDIO_S16LSB;
    mix.audio = malloc(master_volume.music[0].music.length); // Free
    if (!mix.audio)
    {
        fprintf(stderr, "Failed in creating a new sound buffer.\n");
        return 1;
    }
    master_volume.music[0].volume = master_volume.volume;
    SDL_MixAudioFormat(mix.audio,  // dst 
                       master_volume.music[0].music.buffer,     // src 
                       mix.format, // format
                       master_volume.music[0].music.length,     // length
                       master_volume.music[0].volume);           // volume
   
    
    mix_audio_t sfx_mix = {0};
    sfx_mix.format = AUDIO_S16LSB;
    sfx_mix.audio = malloc(master_volume.sfx[0].sfx.length); // Free
    if (!sfx_mix.audio)
    {
        fprintf(stderr, "Failed in creating a new sound buffer.\n");
        return 1;
    }

    // TODO: Figure out how to update sound
    master_volume.sfx[0].volume = master_volume.volume;
    SDL_MixAudioFormat(sfx_mix.audio,  // dst 
               master_volume.sfx[0].sfx.buffer,     // src 
               sfx_mix.format, // format
               master_volume.sfx[0].sfx.length,     // length
               master_volume.sfx[0].volume);           // volume     


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
    LoadAsset(&player_asset, "assets/knight.png");
    InitializeAssetToRender(&player_asset, 10 * 16, 16 * 24, player_asset.w, player_asset.h);
    InitializeAssetStats(&player_asset, &player_stats);

    const char *enemy_filenames[2] = {
        "assets/enemy1.png",
        "assets/enemy2.png"
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

    stats_t enemy_stats = {0};
    enemy_stats.hp = 10;
    enemy_stats.atk = 9;
    enemy_stats.def = 8;
    enemy_stats.exp = 10;
    for (int i = 0; i < 3; ++i)
    {
        InitializeAssetStats(&enemy_arr[i], &enemy_stats);
        printf("enemy hp: %d -> %d\n", i, enemy_arr[i].stats.hp);
    }

    int frames_per_second = 60;
    int frames_per_ms = 1000 / frames_per_second;
    u32 frame_start;
    int frame_time;

    SDL_Texture *font_atlas = CreateFontAtlas(SDLWindow.Renderer);
    if (!font_atlas)
    {
        SDL_DestroyRenderer(SDLWindow.Renderer);
        SDL_DestroyWindow(SDLWindow.Window);
        SDL_Quit();
        return 1;
    }

    SDL_Color green = {0, 255, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    int screen_center_x = ASPECT_WIDTH / 2;
    int screen_center_y = ASPECT_HEIGHT / 2;
    int option_index = 0;
    menu_item_t menu_items[4] = {
        { "New Game", screen_center_x - 32, screen_center_y},
        { "Continue Game", screen_center_x - 32, screen_center_y + 16 },
        { "Settings", screen_center_x - 32, screen_center_y + 32 },
        { "Exit", screen_center_x - 32, screen_center_y + 48 },
    };

    asset_t up_cursor_asset = {0};
    LoadAsset(&up_cursor_asset, "assets/up_cursor.png");
    InitializeAssetToRender(&up_cursor_asset, 0, 0, up_cursor_asset.w, up_cursor_asset.h);
    
    asset_t down_cursor_asset = {0};
    LoadAsset(&down_cursor_asset, "assets/down_cursor.png");
    InitializeAssetToRender(&down_cursor_asset, 0, 0, down_cursor_asset.w, down_cursor_asset.h);
    
    asset_t right_cursor_asset = {0};
    LoadAsset(&right_cursor_asset, "assets/right_cursor.png");
    InitializeAssetToRender(&right_cursor_asset, 0, 0, right_cursor_asset.w, right_cursor_asset.h);
       
    asset_t left_cursor_asset = {0};
    LoadAsset(&left_cursor_asset, "assets/left_cursor.png");
    InitializeAssetToRender(&left_cursor_asset, 0, 0, left_cursor_asset.w, left_cursor_asset.h);
       
    asset_t new_game_asset = {0};
    LoadAsset(&new_game_asset, "assets/new_game_screen.png");
    InitializeAssetToRender(&new_game_asset, 0, 0, new_game_asset.w, new_game_asset.h); 

    typedef struct class_select_t
    {       
        asset_t asset;
        const char *name;
        const char *description;
    } class_select_t;

    const char *class_files[4] = {
        "assets/knight.png",
        "assets/paladin.png",
        "assets/wizard.png",
        "assets/archer.png"
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
    }
    
    InitializeAssetToRender(&character_creation_screen.info[0].asset, screen_center_x - 96, screen_center_y, 
                            character_creation_screen.info[0].asset.w, character_creation_screen.info[0].asset.h);
    InitializeAssetToRender(&character_creation_screen.info[1].asset, screen_center_x - 36, screen_center_y, 
                            character_creation_screen.info[1].asset.w, character_creation_screen.info[1].asset.h);  
    InitializeAssetToRender(&character_creation_screen.info[2].asset, screen_center_x + 28, screen_center_y, 
                            character_creation_screen.info[2].asset.w, character_creation_screen.info[2].asset.h);
    InitializeAssetToRender(&character_creation_screen.info[3].asset, screen_center_x + 80, screen_center_y, 
                            character_creation_screen.info[3].asset.w, character_creation_screen.info[3].asset.h);

    character_creation_screen.description_box.x = screen_center_x - 112; // 16px padding from start of X
    character_creation_screen.description_box.y = screen_center_y + 48;
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

    InitializeAssetToRender(&character_allocation_select_screen.info[0].asset, screen_center_x - 104, screen_center_y, 
                            character_allocation_select_screen.info[0].asset.w, character_allocation_select_screen.info[0].asset.h);
    InitializeAssetToRender(&character_allocation_select_screen.info[1].asset, screen_center_x - 28, screen_center_y, 
                            character_allocation_select_screen.info[1].asset.w, character_allocation_select_screen.info[1].asset.h);
    InitializeAssetToRender(&character_allocation_select_screen.info[2].asset, screen_center_x + 42, screen_center_y, 
                            character_allocation_select_screen.info[2].asset.w, character_allocation_select_screen.info[2].asset.h);

    character_allocation_select_screen.description_box.x = screen_center_x - 112; // 16px padding from start of X
    character_allocation_select_screen.description_box.y = screen_center_y + 48;
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
    confirmation.info.buttons[0].x = screen_center_x - 4;
    confirmation.info.buttons[0].y = screen_center_y;

    confirmation.info.buttons[1].text = "No";
    confirmation.info.buttons[1].x = screen_center_x - 2;
    confirmation.info.buttons[1].y = screen_center_y + 16;

    confirmation.box.x = screen_center_x - 20; // x render pos
    confirmation.box.y = screen_center_y - 20; // y render pos
    confirmation.box.w = 48;
    confirmation.box.h = 56;

    confirmation.box_border.x = screen_center_x - 21; // shift the x render pos 1 pixel back
    confirmation.box_border.y = screen_center_y - 21; // shift the y render pos 1 pixel up
    confirmation.box_border.w = 50; // padding of 1px per side for x 
    confirmation.box_border.h = 58; // padding of 1px per side for y

    /*

    const char *village_scenario[1] = {
        "Silver coins drop from the hanging bag of an elderly man's pocket\n as he walks through the market. What do you do?"
        // Steal the coins openly with pride. -> Show-off
        // Steal the coins sneakily. -> Slippery Devil
        // Don't steal the coins and return them. -> Shrinking Violet
    };

    const char *monster_scenario[1] = {
        "You are a man by day and a beast by night. You prey off human flesh and blood to survive. You come across a small and quiet village. What do you do?"
        // Kill fewer than three people -> Paragon
        // Kill three or more people, including women and the elderly, but don't kill the children -> Wimpy
        // Kill three or more people, but don't kill the women, the elderly, or children -> Spoilt Brat
        // Kill three or more poeple, including children -> Egghead
        // Kill nine or more people, but don't kill the man by the inn -> Klutz (did you know? the lore is so they accuse the man of missing people, because they're the only ones at night to see people)
    };
    */

    personality_scenario_t personality_scenario[1] = {0};
    
    personality_scenario[MONSTER].info.monster_options[0].text = "Kill fewer than three people";
    personality_scenario[MONSTER].info.monster_options[0].x = CenterRenderedText(screen_center_x, personality_scenario[MONSTER].info.monster_options[0].text, 0);
    personality_scenario[MONSTER].info.monster_options[0].y = screen_center_y;
    
    personality_scenario[MONSTER].info.monster_options[1].text = "Kill three or more people, \nincluding women and the elderly, \nbut don't kill the children";
    personality_scenario[MONSTER].info.monster_options[1].x = CenterRenderedText(screen_center_x, personality_scenario[MONSTER].info.monster_options[1].text, 176);
    personality_scenario[MONSTER].info.monster_options[1].y = screen_center_y + 16;

    personality_scenario[MONSTER].info.monster_options[2].text = "Kill three or more people, \nbut don't kill the women, \nthe elderly, or children";
    personality_scenario[MONSTER].info.monster_options[2].x = CenterRenderedText(screen_center_x, personality_scenario[MONSTER].info.monster_options[2].text, 164);
    personality_scenario[MONSTER].info.monster_options[2].y = screen_center_y + 48;
    
    personality_scenario[MONSTER].info.monster_options[3].text = "Kill three or more poeple, \nincluding children";
    personality_scenario[MONSTER].info.monster_options[3].x = CenterRenderedText(screen_center_x, personality_scenario[MONSTER].info.monster_options[3].text, 64);
    personality_scenario[MONSTER].info.monster_options[3].y = screen_center_y + 78;

    personality_scenario[MONSTER].info.monster_options[4].text = "Kill nine or more people, but \ndon't kill the man by the inn";
    personality_scenario[MONSTER].info.monster_options[4].x = CenterRenderedText(screen_center_x, personality_scenario[MONSTER].info.monster_options[4].text, 88);
    personality_scenario[MONSTER].info.monster_options[4].y = screen_center_y + 102;
   

    test_personality_scenario_t test_personality_scenario = {0};
    Personality_ScenarioInitialize(&test_personality_scenario, village_scenario_options, VILLAGE);
    Personality_ScenarioInitialize(&test_personality_scenario, monster_scenario_options, MONSTER_1);

    const char choice[3][100] = {
        {"Kill fewer than three people"},
        {"Kill three or more people, \nincluding women and the elderly, \nbut don't kill the children"},
        {"Kill three or more people, \nbut don't kill the women, \nthe elderly, or children"},
    };

    printf("choice 1: %s\n", choice[0]);

    personality_scenario[MONSTER].scenario.result |= SCENARIO_VILLAGE;
    printf("scenario result 1: 0x%02x\n", personality_scenario[MONSTER].scenario.result);
    if (personality_scenario[MONSTER].scenario.result & SCENARIO_VILLAGE)
        printf("HIT 1\n");
    
    personality_scenario[MONSTER].scenario.result &= ~(SCENARIO_VILLAGE);
    printf("scenario result 2: 0x%02x\n", personality_scenario[MONSTER].scenario.result);
    if (personality_scenario[MONSTER].scenario.result & 0x01)
        printf("HIT 2\n");


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
        { "Yes", screen_center_x - 2, screen_center_y },
        { "No", screen_center_x, screen_center_y + 16 },
    };
  
    const char *back_or_next_cursor_names[2] = {
        "Back(Q)",
        "Next(E)"
    };

    const char *back_or_next_cursor_files[2] = {
        "assets/left_cursor.png",
        "assets/right_cursor.png"
    };

    int back_or_next_cursor_index = 0;
    class_select_t back_or_next_cursor[2] = {0};
    for (int i = 0; i < ArraySize(back_or_next_cursor); ++i)
    {
        LoadAsset(&back_or_next_cursor[i].asset, back_or_next_cursor_files[i]);
        back_or_next_cursor[i].name = back_or_next_cursor_names[i];
    }

    InitializeAssetToRender(&back_or_next_cursor[0].asset, screen_center_x - 112, screen_center_y - 108, 
                            back_or_next_cursor[0].asset.w, back_or_next_cursor[0].asset.h);
    InitializeAssetToRender(&back_or_next_cursor[1].asset, screen_center_x + 96, screen_center_y - 108, 
                            back_or_next_cursor[1].asset.w, back_or_next_cursor[1].asset.h);

    int settings_option_index = 0;
    menu_item_t settings_options[5] = {
        { "Volume", screen_center_x - (GLYPH_WIDTH * strlen(settings_options[0].text) - 1) / 2, screen_center_y - 48 },
        { "Master", screen_center_x - (GLYPH_WIDTH * strlen(settings_options[1].text) - 1) / 2, screen_center_y + 0 },
        { "Music", screen_center_x - (GLYPH_WIDTH * strlen(settings_options[2].text) - 1) / 2, screen_center_y + 24 },
        { "SFX", screen_center_x - (GLYPH_WIDTH * strlen(settings_options[3].text) - 1) / 2, screen_center_y + 48 },
        { "Apply", screen_center_x - ((GLYPH_WIDTH * strlen(settings_options[4].text) - 1) / 2), screen_center_y + 76 }, 
        //{ "Apply", screen_center_x - ((GLYPH_WIDTH * strlen(settings_options[4].text) - 1) / 2) + 96, screen_center_y + 96 }, // old
    };

    // Back and Apply buttons are seperate
    int back_and_apply_buttons_index = 0;
    menu_item_t back_and_apply_buttons[2] = {
        { "Back(Q)", screen_center_x - ((GLYPH_WIDTH * strlen(back_and_apply_buttons[0].text) - 1) / 2) - 96, screen_center_y + 96 },
        { "Apply(E)", screen_center_x - ((GLYPH_WIDTH * strlen(back_and_apply_buttons[1].text) - 1) / 2) + 96, screen_center_y + 96 },
    };

    // TODO: create a highlight to indicate the player is currently "hovering" or selecting a bar to change
    // TODO: create an array of volume bars in respect 
    SDL_Rect master_volume_bar = {0};
    master_volume_bar.x = screen_center_x - ((96/2));
    master_volume_bar.y = settings_options[1].y + 10;
    master_volume_bar.w = 96; 
    master_volume_bar.h = GLYPH_HEIGHT + 2;
    
    SDL_Rect music_volume_bar = {0};
    music_volume_bar.x = screen_center_x - ((96/2));
    music_volume_bar.y = settings_options[2].y + 10;
    music_volume_bar.w = 96; 
    music_volume_bar.h = GLYPH_HEIGHT + 2;

    SDL_Rect sfx_volume_bar = {0};
    sfx_volume_bar.x = screen_center_x - ((96/2));
    sfx_volume_bar.y = settings_options[3].y + 10;
    sfx_volume_bar.w = 96; 
    sfx_volume_bar.h = GLYPH_HEIGHT + 2;

    color_t volume_colors = {0};
    volume_controller_t volume_controller[3] = {0};
    // For now, initialize seperately -> create a function
    for (int i = 0; i < ArraySize(volume_controller[0].blocks); ++i)
    {
        volume_controller[0].blocks[i].x = master_volume_bar.x + volume_controller[0].volume_size_bars;
        volume_controller[0].blocks[i].y = master_volume_bar.y;
        volume_controller[0].blocks[i].w = master_volume_bar.w / 4;
        volume_controller[0].blocks[i].h = master_volume_bar.h;
        
        volume_controller[0].volume_size_bars += (master_volume_bar.w / 4);
    }

    for (int i = 0; i < ArraySize(volume_controller[1].blocks); ++i)
    {
        volume_controller[1].blocks[i].x = music_volume_bar.x + volume_controller[1].volume_size_bars;
        volume_controller[1].blocks[i].y = music_volume_bar.y;
        volume_controller[1].blocks[i].w = music_volume_bar.w / 4;
        volume_controller[1].blocks[i].h = music_volume_bar.h;
        
        volume_controller[1].volume_size_bars += (music_volume_bar.w / 4);
    }

    for (int i = 0; i < ArraySize(volume_controller[2].blocks); ++i)
    {
        volume_controller[2].blocks[i].x = sfx_volume_bar.x + volume_controller[2].volume_size_bars;
        volume_controller[2].blocks[i].y = sfx_volume_bar.y;
        volume_controller[2].blocks[i].w = sfx_volume_bar.w / 4;
        volume_controller[2].blocks[i].h = sfx_volume_bar.h;
        
        volume_controller[2].volume_size_bars += (sfx_volume_bar.w / 4);
    }



    ////////////// Enter name grid 

    typedef struct
    {
        char glyph;
        int x, y;
    } glyph_grid_t;

    glyph_grid_t test_glyph_grid[50] = 
    {
        { 'A', screen_center_x - 52 , screen_center_y - 8 }, 
        { 'B', screen_center_x - 40 , screen_center_y - 8 },
        { 'C', screen_center_x - 28, screen_center_y - 8 }, 
        { 'D', screen_center_x - 16, screen_center_y - 8 }, 
        { 'E', screen_center_x - 4, screen_center_y - 8 }, 
        { 'F', screen_center_x + 16, screen_center_y - 8 }, 
        { 'G', screen_center_x + 28, screen_center_y - 8 }, 
        { 'H', screen_center_x + 40, screen_center_y - 8 }, 
        { 'I', screen_center_x + 52, screen_center_y - 8 }, 
        { 'J', screen_center_x + 64, screen_center_y - 8 }, 

        { 'K', screen_center_x - 52, screen_center_y + 2 }, 
        { 'L', screen_center_x - 40, screen_center_y + 2 }, 
        { 'M', screen_center_x - 28, screen_center_y + 2 }, 
        { 'N', screen_center_x - 16, screen_center_y + 2 }, 
        { 'O', screen_center_x - 4, screen_center_y + 2 }, 
        { 'P', screen_center_x + 16, screen_center_y + 2 }, 
        { 'Q', screen_center_x + 28, screen_center_y + 2 }, 
        { 'R', screen_center_x + 40, screen_center_y + 2 }, 
        { 'S', screen_center_x + 52, screen_center_y + 2 }, 
        { 'T', screen_center_x + 64, screen_center_y + 2 },

        { 'U', screen_center_x - 52, screen_center_y + 14 }, 
        { 'V', screen_center_x - 40, screen_center_y + 14 }, 
        { 'W', screen_center_x - 28, screen_center_y + 14 }, 
        { 'X', screen_center_x - 16, screen_center_y + 14 }, 
        { 'Y', screen_center_x - 4, screen_center_y + 14 }, 
        { 'Z', screen_center_x + 16, screen_center_y + 14 }, 
        { ' ', screen_center_x + 28, screen_center_y + 14 }, // Unused 
        { ' ', screen_center_x + 40, screen_center_y + 14 }, // Unused
        { ' ', screen_center_x + 52, screen_center_y + 14 }, // Unused
        { ' ', screen_center_x + 64, screen_center_y + 14 }, // Unused
  

        // Special characters 
        { '[', screen_center_x - 52, screen_center_y + 26 }, 
        { ']', screen_center_x - 40, screen_center_y + 26 }, 
        { '.', screen_center_x - 28, screen_center_y + 26 }, 
        { ',', screen_center_x - 16, screen_center_y + 26 }, 
        { '!', screen_center_x - 4, screen_center_y + 26 }, 
        { '?', screen_center_x + 16, screen_center_y + 26 }, 
        { '-', screen_center_x + 28, screen_center_y + 26 }, 
        { '_', screen_center_x + 40, screen_center_y + 26 }, 
        { ':', screen_center_x + 52, screen_center_y + 26 }, 
        { ';', screen_center_x + 60, screen_center_y + 26 }, 
        
        // Nums
        { '0', screen_center_x - 52, screen_center_y + 38 }, 
        { '1', screen_center_x - 40, screen_center_y + 38 }, 
        { '2', screen_center_x - 28, screen_center_y + 38 }, 
        { '3', screen_center_x - 16, screen_center_y + 38 }, 
        { '4', screen_center_x - 4, screen_center_y + 38 }, 
        { '5', screen_center_x + 16, screen_center_y + 38 }, 
        { '6', screen_center_x + 28, screen_center_y + 38 }, 
        { '7', screen_center_x + 40, screen_center_y + 38 }, 
        { '8', screen_center_x + 52, screen_center_y + 38 }, 
        { '9', screen_center_x + 64, screen_center_y + 38 },

    };

    int glyph_index = 0;
    menu_item_t glyph_grid[50] = {
        // 10x5 grid
        // Every glyph has a padding of 12px around itself
        // Upper case
        { "A", screen_center_x - 52 , screen_center_y - 8 }, 
        { "B", screen_center_x - 40 , screen_center_y - 8 },
        { "C", screen_center_x - 28, screen_center_y - 8 }, 
        { "D", screen_center_x - 16, screen_center_y - 8 }, 
        { "E", screen_center_x - 4, screen_center_y - 8 }, 
        { "F", screen_center_x + 16, screen_center_y - 8 }, 
        { "G", screen_center_x + 28, screen_center_y - 8 }, 
        { "H", screen_center_x + 40, screen_center_y - 8 }, 
        { "I", screen_center_x + 52, screen_center_y - 8 }, 
        { "J", screen_center_x + 64, screen_center_y - 8 }, 

        { "K", screen_center_x - 52, screen_center_y + 2 }, 
        { "L", screen_center_x - 40, screen_center_y + 2 }, 
        { "M", screen_center_x - 28, screen_center_y + 2 }, 
        { "N", screen_center_x - 16, screen_center_y + 2 }, 
        { "O", screen_center_x - 4, screen_center_y + 2 }, 
        { "P", screen_center_x + 16, screen_center_y + 2 }, 
        { "Q", screen_center_x + 28, screen_center_y + 2 }, 
        { "R", screen_center_x + 40, screen_center_y + 2 }, 
        { "S", screen_center_x + 52, screen_center_y + 2 }, 
        { "T", screen_center_x + 64, screen_center_y + 2 },

        { "U", screen_center_x - 52, screen_center_y + 14 }, 
        { "V", screen_center_x - 40, screen_center_y + 14 }, 
        { "W", screen_center_x - 28, screen_center_y + 14 }, 
        { "X", screen_center_x - 16, screen_center_y + 14 }, 
        { "Y", screen_center_x - 4, screen_center_y + 14 }, 
        { "Z", screen_center_x + 16, screen_center_y + 14 }, 
        { "", screen_center_x + 28, screen_center_y + 14 }, // Unused 
        { "", screen_center_x + 40, screen_center_y + 14 }, // Unused
        { "", screen_center_x + 52, screen_center_y + 14 }, // Unused
        { "", screen_center_x + 64, screen_center_y + 14 }, // Unused
  

        // Special characters 
        { "[", screen_center_x - 52, screen_center_y + 26 }, 
        { "]", screen_center_x - 40, screen_center_y + 26 }, 
        { ".", screen_center_x - 28, screen_center_y + 26 }, 
        { ",", screen_center_x - 16, screen_center_y + 26 }, 
        { "!", screen_center_x - 4, screen_center_y + 26 }, 
        { "?", screen_center_x + 16, screen_center_y + 26 }, 
        { "-", screen_center_x + 28, screen_center_y + 26 }, 
        { "_", screen_center_x + 40, screen_center_y + 26 }, 
        { ":", screen_center_x + 52, screen_center_y + 26 }, 
        { ";", screen_center_x + 60, screen_center_y + 26 }, 
        
        // Nums
        { "0", screen_center_x - 52, screen_center_y + 38 }, 
        { "1", screen_center_x - 40, screen_center_y + 38 }, 
        { "2", screen_center_x - 28, screen_center_y + 38 }, 
        { "3", screen_center_x - 16, screen_center_y + 38 }, 
        { "4", screen_center_x - 4, screen_center_y + 38 }, 
        { "5", screen_center_x + 16, screen_center_y + 38 }, 
        { "6", screen_center_x + 28, screen_center_y + 38 }, 
        { "7", screen_center_x + 40, screen_center_y + 38 }, 
        { "8", screen_center_x + 52, screen_center_y + 38 }, 
        { "9", screen_center_x + 64, screen_center_y + 38 },

        // Buttons (seperate?) -> Also should not be hardcoded to say keyboard keys
        //{ "Confirm", screen_center_x, screen_center_y },
    };

    // Something maybe unnecessary, but lines underneath the to be entered glyphs to indicate where the player is entering their name
    int name_bar_index = 0;
    menu_item_t name_bar_underline[NAME_LIMIT] = {
        { "_", screen_center_x - 32, screen_center_y - 28 }, 
        { "_", screen_center_x - 24, screen_center_y - 28 }, 
        { "_", screen_center_x - 16, screen_center_y - 28 }, 
        { "_", screen_center_x - 8, screen_center_y - 28 }, 
        { "_", screen_center_x - 0, screen_center_y - 28 }, 
        { "_", screen_center_x + 8, screen_center_y - 28 }, 
        { "_", screen_center_x + 16, screen_center_y - 28 }, 
        { "_", screen_center_x + 24, screen_center_y - 28 }, 
        { "_", screen_center_x + 32, screen_center_y - 28 }, 
        { "_", screen_center_x + 40, screen_center_y - 28 }, 
    };

    int name_pos = 0; 
    int name_limit = NAME_LIMIT;
    char temp_glyph;

    typedef struct 
    {
        char str[1];
        int x, y;
    } i_want_to_see_t;

    i_want_to_see_t name_array[NAME_LIMIT] = {
        { "", screen_center_x - 32, screen_center_y - 28 }, 
        { "", screen_center_x - 24, screen_center_y - 28 }, 
        { "", screen_center_x - 16, screen_center_y - 28 }, 
        { "", screen_center_x - 8, screen_center_y - 28 }, 
        { "", screen_center_x - 0, screen_center_y - 28 }, 
        { "", screen_center_x + 8, screen_center_y - 28 }, 
        { "", screen_center_x + 16, screen_center_y - 28 }, 
        { "", screen_center_x + 24, screen_center_y - 28 }, 
        { "", screen_center_x + 32, screen_center_y - 28 }, 
        { "", screen_center_x + 40, screen_center_y - 28 }, 
    };


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
    class_base_stats_t class_base_stats[4] = {0};
    class_base_stats[0].strength = 11;
    class_base_stats[0].resilience = 11;
    class_base_stats[0].agility = 6;
    class_base_stats[0].stamina = 13;
    class_base_stats[0].wisdom = 3;
    class_base_stats[0].luck = 4;
    class_base_stats[0].max_hp = (3 * class_base_stats[0].stamina) + 5; // formula from DQ3 -> HP = [3 * VIT / 2] + 5
    class_base_stats[0].hp = class_base_stats[0].max_hp; // 
    class_base_stats[0].max_mp = class_base_stats[0].wisdom; // MP == Wisdom
    class_base_stats[0].hp = class_base_stats[0].max_mp; // 

    printf("Max HP: %d\n", class_base_stats[0].max_hp);

       
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
   
    //float a = (float)class_stat_growth[0].strength.growth_per_level[0];
    //float b = personality_stat_growth[0].strength_growth;
    //float calc =  a * b;
    //int result = floor(calc); 

    typedef struct
    {
         int index;
         u32 experience;
         struct 
         {
            u32 remaining;
         } experience_next_level[20];
    } character_experience_t;


    typedef struct
    {
        char name[10];
        u32 level; 
        u32 experience;

        // 20 is max level
        int experience_index;
        u32 experience_to_next_level[20];
        
        // character class model
        asset_t model;

        struct 
        {
            char name[32];
            class_base_stats_t base_stats;
    } class;

        struct
        {
            char name[32]; // leave here for now
        } personality;

        int equipment_index;
        menu_item_t equipments_temp;
        struct
        {
            // Instead of packing this with char for names, we should pack it with equipment_t types
            char main_hand[12];
            char off_hand[12];
            char head[12];
            char chest[12];
            char assessories_left[12];
            char assessories_right[12];
        } equipment;
    } character_data_t;
    
    character_data_t character_data = {0};
    character_data.level = 1;
    character_data.experience = 0;

    int experience_index = character_data.equipment_index;
    character_data.experience_to_next_level[experience_index] = 1000;


    

    /////////////////////////////////////////////////////////////////////////

    // Start event
    bool some_input = true;
    int questions_answered_index = 1;
    is_title_screen = true;
    Running = true;

    bool volume_settings_touched = false;

    bool is_final_question = false;
    bool is_quiz = false;
    bool is_glyph_selected = false;
    bool is_glyph_deleted = false;
    bool entering_glyph = false;
                            
    u32 current_row = glyph_index / 10;
    u32 current_col = glyph_index % 5;
    while (Running) 
    {
        frame_start = SDL_GetTicks();
                                                              
        // Poll events
        while (SDL_PollEvent(&SDLWindow.e))
        {
            switch (SDLWindow.e.type)
            {
                case SDL_QUIT:
                {
                    Running = false;
                } break;
                case SDL_KEYDOWN:
                {  

                    switch (SDLWindow.e.key.keysym.sym)
                    {
                        
                        case SDLK_w:
                        {
                            Orientation.up = true;
                            if (is_title_screen)
                            {
                                option_index--;
                                if (option_index < 0)
                                    option_index = ArraySize(menu_items) - 1;
                                PlaySFX(&sfx_volume_test[0].sfx);
                            }
                            
                            if (is_new_game)
                            {
                                button_select--;
                                if (button_select < 0)
                                    button_select = ArraySize(confirmation_buttons) - 1;
                                PlaySFX(&sfx_volume_test[0].sfx);
                            }
                            
                            if (confirmation.is_active)
                            {
                                confirmation.index--;
                                if (confirmation.index < 0)
                                    confirmation.index = ArraySize(confirmation.info.buttons) - 1;
                                PlaySFX(&sfx_volume_test[0].sfx);
                            }

                            if (is_settings)
                            {
                                settings_option_index--;
                                if (settings_option_index < 0)
                                    settings_option_index = ArraySize(settings_options) - 1;
                            }
                        
                            if (is_name_submission)
                            {
                                current_row = (current_row - 1 + 5) % 5;
                                glyph_index = current_row * 10 + current_col;
                                printf("current_row: %d\n", current_row);
                            }

                            if (personality_scenario[0].is_active)
                            {
                                personality_scenario[0].index--;
                                if (personality_scenario[0].index < 0)
                                    personality_scenario[0].index = ArraySize(personality_scenario[0].info.monster_options) - 1;
                            }

                        } break;
                        case SDLK_s:
                        {
                            Orientation.down = true;

                            if (is_title_screen)
                            {
                                option_index++;
                                if (option_index >= ArraySize(menu_items))
                                    option_index = 0;
                                PlaySFX(&sfx_volume_test[0].sfx);
                            }

                            if (is_new_game)
                            {
                                button_select++;
                                if (button_select >= ArraySize(confirmation_buttons))
                                    button_select = 0;
                                PlaySFX(&sfx_volume_test[0].sfx);
                            }
    
                            if (confirmation.is_active)
                            {
                                confirmation.index++;
                                if (confirmation.index >= ArraySize(confirmation.info.buttons))
                                    confirmation.index = 0;
                                PlaySFX(&sfx_volume_test[0].sfx);
                            }

                            if (is_settings)
                            {
                                settings_option_index++;
                                if (settings_option_index >= ArraySize(settings_options))
                                    settings_option_index = ArraySize(settings_options) - 1;
                            }

                            if (is_name_submission)
                            {
                                current_row = (current_row + 1) % 5;
                                glyph_index = current_row * 10 + current_col;
                                printf("current_row: %d\n", current_row);
                            }  

                            if (personality_scenario[0].is_active)
                            {
                                personality_scenario[0].index++;
                                if (personality_scenario[0].index >= ArraySize(personality_scenario[0].info.monster_options))
                                    personality_scenario[0].index = 0;
                            }
                        } break;
                        case SDLK_a:
                        {
                            Orientation.left = true;
                            if (character_creation_screen.is_active && !confirmation.is_active)
                            {
                                character_creation_screen.index--;
                                if (character_creation_screen.index < 0)
                                    character_creation_screen.index = ArraySize(character_creation_screen.info) - 1;
                                PlaySFX(&sfx_volume_test[0].sfx);
                            }

                            if (character_allocation_select_screen.is_active && !confirmation.is_active)
                            {
                                character_allocation_select_screen.index--;
                                if (character_allocation_select_screen.index < 0)
                                    character_allocation_select_screen.index = ArraySize(character_allocation_select_screen.info) - 1;
                                PlaySFX(&sfx_volume_test[0].sfx);
                            }
                      
                            // switch case below
                            if (is_settings && settings_option_index == 1)
                            {
                                volume_controller[0].index--;
                                if (volume_controller[0].index < 0)
                                    volume_controller[0].index = 0;
                                PlaySFX(&sfx_volume_test[0].sfx);
                                volume_settings_touched = true;
                            }

                            if (is_settings && settings_option_index == 2)
                            {
                                volume_controller[1].index--;
                                if (volume_controller[1].index < 0)
                                    volume_controller[1].index = 0;
                                PlaySFX(&sfx_volume_test[0].sfx);
                                volume_settings_touched = true;
                            }

                            if (is_settings && settings_option_index == 3)
                            {
                                volume_controller[2].index--;
                                if (volume_controller[2].index < 0)
                                    volume_controller[2].index = 0;
                                PlaySFX(&sfx_volume_test[0].sfx);
                                volume_settings_touched = true;
                            } 

                            if (is_settings)
                            {
                                switch (settings_option_index)
                                {
                                    case 0:
                                    {
                                        volume_settings_state = VOL_SETTINGS_NONE;
                                    } break;
                                    case 1:
                                    {
                                        volume_settings_state = VOL_SETTINGS_MASTER;
                                    } break;
                                    case 2:
                                    {
                                        volume_settings_state = VOL_SETTINGS_MUSIC;
                                    } break;
                                    case 3:
                                    {
                                        volume_settings_state = VOL_SETTINGS_SFX;
                                    } break;
                                }

                                for (int i = 0; i < 3; ++i)
                                {
                                    switch (volume_controller[i].index)
                                    {
                                        case 0:
                                        {
                                            volume_controller[i].mute = true;
                                        } break;
                                        case 1:                                        
                                        {
                                            volume_controller[i].one = true;
                                        } break;
                                        case 2: 
                                        {
                                            volume_controller[i].two = true;
                                        } break;
                                        case 3:   
                                        {
                                            volume_controller[i].three = true;
                                        } break;
                                        case 4:
                                        {
                                            volume_controller[i].max = true;
                                        } break;
                                        default:
                                        {
                                            volume_controller[i].one = true;
                                        } break;
                                    }
                                }
                            }

                            if (is_name_submission)
                            {
                                current_col = (current_col - 1 + 10) % 10;
                                glyph_index = current_row * 10 + current_col;
                                printf("current_col: %d\n", current_col);
                            }
                        } break;
                        case SDLK_d:
                        {
                            Orientation.right = true;
                            if (character_creation_screen.is_active && !confirmation.is_active)
                            {
                                character_creation_screen.index++;
                                if (character_creation_screen.index >= ArraySize(character_creation_screen.info))
                                    character_creation_screen.index = 0;
                                PlaySFX(&sfx_volume_test[0].sfx);
                            }

                            if (character_allocation_select_screen.is_active && !confirmation.is_active)
                            {
                                character_allocation_select_screen.index++;
                                if (character_allocation_select_screen.index >= ArraySize(character_allocation_select_screen.info))
                                    character_allocation_select_screen.index = 0;
                                PlaySFX(&sfx_volume_test[0].sfx);
                            }

                            if (is_settings)
                            {
                                switch (settings_option_index)
                                {
                                    case 0:
                                    {
                                        volume_settings_state = VOL_SETTINGS_NONE;
                                    } break;
                                    case 1:
                                    {
                                        volume_settings_state = VOL_SETTINGS_MASTER;

                                        // Turn this into a function
                                        volume_controller[0].index++;
                                        if (volume_controller[0].index >= ArraySize(settings_options))
                                            volume_controller[0].index = ArraySize(settings_options) - 1;
                  
                                        PlaySFX(&sfx_volume_test[0].sfx);

                                        volume_settings_touched = true;
                                    } break;
                                    case 2:
                                    {
                                        volume_settings_state = VOL_SETTINGS_MUSIC;

                                        volume_controller[1].index++;
                                        if (volume_controller[1].index >= ArraySize(settings_options))
                                            volume_controller[1].index = ArraySize(settings_options) - 1;
                                        PlaySFX(&sfx_volume_test[0].sfx);
                                        volume_settings_touched = true;

                                    } break;
                                    case 3:
                                    {
                                        volume_settings_state = VOL_SETTINGS_SFX;
    
                                        volume_controller[2].index++;
                                        if (volume_controller[2].index >= ArraySize(settings_options))
                                            volume_controller[2].index = ArraySize(settings_options) - 1;
                                        PlaySFX(&sfx_volume_test[0].sfx);
                                        volume_settings_touched = true;
                                    } break;
                                }

                                for (int i = 0; i < ArraySize(volume_controller); ++i)
                                {
                                    switch (volume_controller[i].index)
                                    {
                                        case 0:
                                        {
                                            volume_controller[i].mute = true;
                                        } break;
                                        case 1: 
                                        {
                                            volume_controller[i].one = true;
                                        } break;
                                        case 2: 
                                        {
                                            volume_controller[i].two = true;
                                        } break;
                                        case 3: 
                                        {
                                            volume_controller[i].three = true;
                                        } break;
                                        case 4: 
                                        {
                                            volume_controller[i].max = true;
                                        } break;
                                    }
                                }
                            }
                           
                            
                            if (is_name_submission)
                            {
                                current_col = (current_col + 1) % 10;
                                glyph_index = current_row * 10 + current_col;
                                printf("current_col: %d\n", current_col);
                            }
                        } break;
                        case SDLK_q:
                        {
                            if (is_settings)
                            {
                                back_and_apply_buttons_index--;
                                if (back_and_apply_buttons_index < 0)
                                    back_and_apply_buttons_index = 0;
                                PlaySFX(&sfx_volume_test[0].sfx);
                                printf("%d\n", back_and_apply_buttons_index);
                                printf("%s\n", back_and_apply_buttons[back_or_next_cursor_index].text);
                                

                                switch (back_and_apply_buttons_index)
                                {
                                    case 0: // back
                                    {
                                        printf("are we in back?\n");
                                        is_title_screen = true;
                                        is_settings = false;
                                        volume_settings_touched = false;
                                        title_screen_state = TITLE_NONE;
                                    } break;
                                }
                                
                            }
                        } break;
                        case SDLK_e:
                        {
                            if (is_settings)
                            {
                                back_and_apply_buttons_index++;
                                if (back_and_apply_buttons_index >= ArraySize(back_and_apply_buttons))
                                    back_and_apply_buttons_index = ArraySize(back_and_apply_buttons) - 1;
                                PlaySFX(&sfx_volume_test[0].sfx);

                                printf("%s\n", back_and_apply_buttons[back_or_next_cursor_index].text);
                                
                                switch (back_and_apply_buttons_index)
                                {
                                    case 1: // back
                                    {
                                        printf("are we in apply?\n");
                                        is_title_screen = true;
                                        is_settings = false;
                                        title_screen_state = TITLE_NONE;
                                    } break;
                                }
                                
                            }
                        } break;
                        case SDLK_BACKSPACE:
                        {
                            if (is_name_submission && !is_glyph_deleted)
                            {
                                entering_glyph = true;
                                character_class_name_submission_state = CLASS_NAME_DELETE;
                            }
                        } break;
                        case SDLK_RETURN:
                        {
                            // TEST
                            if (is_title_screen)
                            {
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
                                switch (character_creation_screen.index)
                                {
                                    case 0:
                                    {
                                        character_class_selection_state = CLASS_KNIGHT;
                                    } break;
                                    case 1:
                                    {
                                        character_class_selection_state = CLASS_PALADIN;
                                    } break;
                                    case 2:
                                    {
                                        character_class_selection_state = CLASS_MAGE;
                                    } break; 
                                    case 3:
                                    {
                                        character_class_selection_state = CLASS_ARCHER;
                                    } break;
                                    default:
                                    {
                                        character_class_selection_state = CLASS_NONE;
                                    } break;
                                }
                            }
                           
                            if (character_allocation_select_screen.is_active)
                            {
                                switch (character_allocation_select_screen.index)
                                {
                                    case 0:
                                    {
                                        character_class_point_allocation_method_state = CLASS_POINTS_PERSONALITY; 
                                    } break;
                                    case 1:
                                    {
                                        character_class_point_allocation_method_state = CLASS_POINTS_PRESET; 
                                    } break;
                                    case 2:
                                    {
                                        character_class_point_allocation_method_state = CLASS_POINTS_MANUAL; 
                                    } break;
                                    default:
                                    {
                                        character_class_point_allocation_method_state = CLASS_POINTS_PERSONALITY; 
                                    } break;
                                }
                            }

                            if (character_creation_screen.is_active && confirmation.is_active)
                            {
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
                                            character_class_selection_state = CLASS_NONE;
                                        } break;
                                        default:
                                        {
                                            yes_or_no_buttons_state = CONFIRMATION_NONE;
                                        } break;
                                    }
                                 
                            }
                          
                            if (character_allocation_select_screen.is_active && confirmation.is_active)
                            {
                                switch (confirmation.index)
                                {
                                    case 0:
                                    {
                                        if (character_allocation_select_screen.index == 0)
                                        {
                                            character_allocation_select_screen.is_active = false;
                                            is_personality_test = true;
                                            is_quiz = true;
                                        }
                                        
                                        confirmation.is_active = false; 
                                    } break;
                                    case 1:
                                    {
                                        confirmation.is_active = false;
                                        confirmation.index = 0;
                                        character_class_point_allocation_method_state = CLASS_POINTS_NONE;
                                    } break;
                                    default:
                                    {
                                        yes_or_no_buttons_state = CONFIRMATION_NONE;
                                    } break;
                                }
                            }

                            if (is_personality_test && is_quiz && confirmation.is_active)
                            {
                                switch (confirmation.index)
                                {
                                    case 0: // yes
                                    {
                                        // turn into a function
                                        switch (questions_answered_index)
                                        {
                                            case 1:
                                            {
                                                questions_answered_index = 7;
                                            } break;
                                            case 2:
                                            {
                                                questions_answered_index = 14;
                                            } break;
                                            case 3:
                                            {
                                                questions_answered_index = 6;
                                            } break;
                                            case 4:
                                            {
                                                questions_answered_index = 15;
                                            } break;
                                            case 5:
                                            {
                                                questions_answered_index = 8;
                                            } break;
                                            case 6:
                                            {
                                                questions_answered_index = 7;
                                            } break;
                                            case 7:
                                            {
                                                questions_answered_index = 10;
                                            } break;
                                            case 8:
                                            {
                                                questions_answered_index = 10;
                                            } break;
                                            case 9:
                                            {
                                                questions_answered_index = 11;
                                            } break;
                                            case 10:
                                            {
                                                questions_answered_index = 14;
                                            } break;
                                            case 11:
                                            {
                                                questions_answered_index = 14;
                                            } break;
                                            case 12:
                                            {
                                                questions_answered_index = 31;
                                            } break;
                                            case 13:
                                            {
                                                questions_answered_index = 25;
                                            } break;
                                            case 14:
                                            {
                                                questions_answered_index = 18;
                                            } break;
                                            case 15:
                                            {
                                                questions_answered_index = 16;
                                            } break;
                                            case 16:
                                            {
                                                questions_answered_index = 17;
                                            } break;
                                            case 17:
                                            {
                                                questions_answered_index = 21;
                                            } break;
                                            case 18:
                                            {
                                                questions_answered_index = 19;
                                            } break;
                                            case 19:
                                            {
                                                questions_answered_index = 20;
                                            } break;
                                            case 20:
                                            {
                                                questions_answered_index = 21;
                                            } break; 
                                            case 21:
                                            {
                                                questions_answered_index = 23;
                                            } break;
                                            case 22:
                                            {
                                                questions_answered_index = 38;
                                            } break;
                                            case 23:
                                            {
                                                questions_answered_index = 24;
                                            } break;
                                            case 24:
                                            {
                                                questions_answered_index = 34;
                                            } break;
                                            case 25:
                                            {
                                                questions_answered_index = 31;
                                            } break;
                                            case 26:
                                            {
                                                questions_answered_index = 27;
                                            } break;
                                            case 27:
                                            {
                                                questions_answered_index = 28;
                                            } break;
                                            case 28:
                                            {
                                                questions_answered_index = 29;
                                            } break;
                                            case 29:
                                            {
                                                questions_answered_index = 30;
                                            } break;
                                            case 30:
                                            {
                                                character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_VILLAGE; // final question
                                            } break;
                                            case 31:
                                            {
                                                questions_answered_index = 32;
                                            } break;
                                            case 32:
                                            {
                                                questions_answered_index = 33;
                                            } break;
                                            case 33:
                                            {
                                                character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_DESERT; // final question
                                            } break;
                                            case 34:
                                            {
                                                character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_MONSTER; // final question
                                            } break;
                                            case 35:
                                            {
                                                questions_answered_index = 0; // final question
                                            } break;
                                            case 36:
                                            {
                                                questions_answered_index = 37;
                                            } break;
                                            case 37:
                                            {
                                                questions_answered_index = 43;
                                            } break;
                                            case 38:
                                            {
                                                questions_answered_index = 39;
                                            } break;
                                            case 39:
                                            {
                                                character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_TOWER;
                                            } break;
                                            case 40:
                                            {
                                                questions_answered_index = 42;
                                            } break; 
                                            case 41:
                                            {
                                                questions_answered_index = 43;
                                            } break;
                                            case 42:
                                            {
                                                questions_answered_index = 43;
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
                                                questions_answered_index = 47;
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
                                                character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_CASTLE;
                                            } break;
                                            case 49:
                                            {
                                               character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_CASTLE; 
                                            } break;
                                        }
                                    } break;
                                    case 1: // no
                                    {
                                        switch (questions_answered_index)
                                        {
                                            case 1:
                                            {
                                                questions_answered_index = 9;
                                            } break;
                                            case 2:
                                            {
                                                questions_answered_index = 8;
                                            } break;
                                            case 3:
                                            {
                                                questions_answered_index = 8;
                                            } break;
                                            case 4:
                                            {
                                                questions_answered_index = 6;
                                            } break;
                                            case 5:
                                            {
                                                questions_answered_index = 16;
                                            } break;
                                            case 6:
                                            {
                                                questions_answered_index = 8;
                                            } break;
                                            case 7:
                                            {
                                                questions_answered_index = 5;
                                            } break;
                                            case 8:
                                            {
                                                questions_answered_index = 9;
                                            } break;
                                            case 9:
                                            {
                                                questions_answered_index = 12;
                                            } break;
                                            case 10:
                                            {
                                                questions_answered_index = 13;
                                            } break;
                                            case 11:
                                            {
                                                questions_answered_index = 13;
                                            } break;
                                            case 12:
                                            {
                                                questions_answered_index = 14;
                                            } break;
                                            case 13:
                                            {
                                                questions_answered_index = 15;
                                            } break;
                                            case 14:
                                            {
                                                questions_answered_index = 19;
                                            } break;
                                            case 15:
                                            {
                                                questions_answered_index = 20;
                                            } break;
                                            case 16:
                                            {
                                                questions_answered_index = 22;
                                            } break;
                                            case 17:
                                            {
                                                questions_answered_index = 25;
                                            } break;
                                            case 18:
                                            {
                                                questions_answered_index = 23;
                                            } break;
                                            case 19:
                                            {
                                                questions_answered_index = 25;
                                            } break;
                                            case 20:
                                            {
                                                questions_answered_index = 22;
                                            } break; 
                                            case 21:
                                            {
                                                questions_answered_index = 23;
                                            } break;
                                            case 22:
                                            {
                                                questions_answered_index = 38;
                                            } break;
                                            case 23:
                                            { 
                                                questions_answered_index = 40;
                                            } break;
                                            case 24:
                                            {
                                                questions_answered_index = 25;
                                            } break;
                                            case 25:
                                            {
                                                questions_answered_index = 26;
                                            } break;
                                            case 26:
                                            {
                                                questions_answered_index = 28;
                                            } break;
                                            case 27:
                                            {
                                                questions_answered_index = 29;
                                            } break;
                                            case 28:
                                            {
                                                questions_answered_index = 30;
                                            } break;
                                            case 29:
                                            {
                                                questions_answered_index = 30;
                                            } break;
                                            case 30:
                                            {
                                                questions_answered_index = 40; 
                                            } break;
                                            case 31:
                                            {
                                                questions_answered_index = 34;
                                            } break;
                                            case 32:
                                            {
                                                questions_answered_index = 36;
                                            } break;
                                            case 33:
                                            {
                                                questions_answered_index = 36;
                                            } break;
                                            case 34:
                                            {
                                                questions_answered_index = 36;
                                            } break;
                                            case 35:
                                            {
                                                questions_answered_index = 36; 
                                            } break;
                                            case 36:
                                            {
                                                questions_answered_index = 48;
                                            } break;
                                            case 37:
                                            {
                                                questions_answered_index = 49;
                                            } break;
                                            case 38:
                                            {
                                                questions_answered_index = 40;
                                            } break;
                                            case 39:
                                            {
                                                questions_answered_index = 41; 
                                            } break;
                                            case 40:
                                            {
                                                questions_answered_index = 41;
                                            } break; 
                                            case 41:
                                            {
                                                questions_answered_index = 42;
                                            } break;
                                            case 42:
                                            {
                                                questions_answered_index = 44;
                                            } break;
                                            case 43:
                                            {
                                                questions_answered_index = 45; 
                                            } break;
                                            case 44:
                                            {
                                                questions_answered_index = 45;
                                            } break;
                                            case 45:
                                            {
                                                questions_answered_index = 46;
                                            } break;
                                            case 46:
                                            {
                                                questions_answered_index = 47; 
                                            } break;
                                            case 47:
                                            {
                                                character_class_personality_test_result_state = CLASS_PERSONALITY_RESULT_THEATER; 
                                            } break;
                                            case 48:
                                            {
                                                questions_answered_index = 49;
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
                                switch (settings_option_index)
                                {
                                    case 4: // apply
                                    {
                                        for (int i = 0; i < 3; ++i)
                                        {
                                            volume_controller[i].apply = true;
                                        }
                                        volume_settings_state = VOL_SETTINGS_APPLY;
                                    } break;
                                    default:
                                    { 
                                         
                                    } break;
                                }
                            }

                            if (is_name_submission && !is_glyph_selected)
                            {
                                printf("glyph_index: %d\n", glyph_index);
                                glyph_index = current_row * 10 + current_col;
                                if (glyph_index > 0 || glyph_index <= 50)
                                {
                                    entering_glyph = true;
                                    character_class_name_submission_state = CLASS_NAME_ENTER;
                                }

                            }

                            if (is_personality_test)
                            {

                                switch (personality_scenario[MONSTER].scenario.result)
                                {
                                    case SCENARIO_MONSTER:
                                    {
                                        switch (personality_scenario[MONSTER].index)
                                        {
                                            // Kill fewer than three people -> Paragon
                                            // Kill three or more people, including women and the elderly, but don't kill the children -> Wimpy
                                            // Kill three or more people, but don't kill the women, the elderly, or children -> Spoilt Brat
                                            // Kill three or more poeple, including children -> Egghead
                                            // Kill nine or more people, but don't kill the man by the inn -> Klutz (did you know? the lore is so they accuse the man of missing people, because they're the only ones at night to see people)
                                            case 0: 
                                            {
                                                printf("Paragon\n");
                                                //personality_scenario.is_active = false; // set this on to go to the next screen
                                            } break;
                                            case 1: 
                                            {
                                                printf("Wimpy\n");
                                            } break;
                                            case 2: 
                                            {
                                                printf("Spoilt Brat\n");
                                            } break;
                                            case 3: 
                                            {
                                                printf("Egghead\n");
                                            } break;
                                            case 4: 
                                            {
                                                printf("Klutz\n");
                                            } break;
                                            default:
                                            {

                                            } break;
                                        } 
                                        
                                    } break;
                                }
                                
                            }
                            


                        } break;
                        default:
                        {

                        } break;
                    }
                } break;
                default:
                {
                   // Nothing 
                } break;
            }
            
        }
        
        SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
        SDL_RenderClear(SDLWindow.Renderer);
       
        if (is_title_screen)
        {
            switch (title_screen_state)
            {
                case TITLE_NONE:
                {
                    //SDL_PauseAudioDevice(master_volume.music[0].music.device_id, 0);

                    SDL_RenderCopy(SDLWindow.Renderer, title_screen_asset.texture, NULL, &title_screen_asset.body);
                    CursorForItems(&menu_items[option_index], &right_cursor_asset, 4, 1);
                    RenderAndUpdateAsset(&right_cursor_asset);
                    
                    // Render options
                    for (int i = 0; i < ArraySize(menu_items); ++i)
                    {
                        RenderText(SDLWindow.Renderer, font_atlas, menu_items[i].x, menu_items[i].y,
                                   menu_items[i].text, white);
                    }
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
        }
    
        if (is_settings) 
        {
            SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
            CursorForItems(&settings_options[settings_option_index], &right_cursor_asset, 6, 1);
            RenderAndUpdateAsset(&right_cursor_asset);
            
            for (int i = 0; i < ArraySize(settings_options) - 1; ++i) // Ignore rendering the apply button 
            {
                RenderText(SDLWindow.Renderer, font_atlas,
                           settings_options[i].x, 
                           settings_options[i].y,
                           settings_options[i].text, 
                           white);
            }

            // If any of the volume settings has been touched, then render the apply button
            // TODO: Cursor should not be able to hover over apply when it isn't rendered yet.
            if (volume_settings_touched) 
            {
                RenderText(SDLWindow.Renderer, font_atlas,
                           settings_options[4].x, 
                           settings_options[4].y,
                           settings_options[4].text, 
                           white);
            }

            // This currently renders the back and apply button, but I prefer just the back button, where the apply button is better suited where 
            // and who it was supposed to render with, the volume settings.
            for (int i = 0; i < ArraySize(back_and_apply_buttons); ++i)
            {
                RenderText(SDLWindow.Renderer, font_atlas,
                           back_and_apply_buttons[i].x, 
                           back_and_apply_buttons[i].y,
                           back_and_apply_buttons[i].text, 
                           white);
            }

            SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(SDLWindow.Renderer, &master_volume_bar);
            SDL_RenderDrawRect(SDLWindow.Renderer, &music_volume_bar);
            SDL_RenderDrawRect(SDLWindow.Renderer, &sfx_volume_bar);

            switch (volume_settings_state)
            {
                case VOL_SETTINGS_NONE:
                {
                    // Nothing
                } break;
                case VOL_SETTINGS_MASTER:
                {
                    switch (volume_controller[0].index)
                    {
                        // Usage code of updating a volume bar
                        case 0:
                        {
                            if (volume_controller[0].mute)
                            {
                                volume_controller[0].volume = 0;
                                volume_controller[0].mute = false;
                            }
                        } break;
                        case 1:
                        {
                            volume_controller[0].colors.r = 0;
                            volume_controller[0].colors.g = 255;
                            volume_controller[0].colors.b = 0;
                            volume_controller[0].colors.a = 255;
                            
                            if (volume_controller[0].one)
                            {
                                volume_controller[0].volume = 32;
                                volume_controller[0].one = false;
                            }
                        } break;
                        case 2:
                        {
                            volume_controller[0].colors.r = 255;
                            volume_controller[0].colors.g = 255;
                            volume_controller[0].colors.b = 0;
                            volume_controller[0].colors.a = 255;

                            if (volume_controller[0].two)
                            {
                                volume_controller[0].volume = 64;
                                volume_controller[0].two = false;
                            }
                        } break;
                        case 3:
                        {
                            volume_controller[0].colors.r = 255;
                            volume_controller[0].colors.g = 165;
                            volume_controller[0].colors.b = 0;
                            volume_controller[0].colors.a = 255;

                            if (volume_controller[0].three)
                            {
                                volume_controller[0].volume = 96;
                                volume_controller[0].three = false;
                            }
                        } break;
                        case 4:
                        {
                            volume_controller[0].colors.r = 255;
                            volume_controller[0].colors.g = 0;
                            volume_controller[0].colors.b = 0;
                            volume_controller[0].colors.a = 255;
                            
                            if (volume_controller[0].max)
                            {
                                volume_controller[0].volume = SDL_MIX_MAXVOLUME;
                                volume_controller[0].max = false;
                            }
                        } break;
                        default:
                        {
                            
                        } break;
                    }

                    // Render updated volume blocks and its color
                    
                    for (int j = 0; j < 3; ++j)
                    {
                        for (int i = 0; i < volume_controller[j].index; ++i)
                        {
                            SDL_SetRenderDrawColor(SDLWindow.Renderer, volume_controller[j].colors.r, volume_controller[j].colors.g, volume_controller[j].colors.b, volume_controller[j].colors.a);
                            SDL_RenderFillRect(SDLWindow.Renderer, &volume_controller[j].blocks[i]);
                        }
                    }
                } break;
                case VOL_SETTINGS_MUSIC:
                {
                    UpdateAndRenderVolumeBars(&volume_controller[1]);

                    // this will have to do!
                    for (int j = 0; j < 3; ++j)
                    {
                        for (int i = 0; i < volume_controller[j].index; ++i)
                        {
                            SDL_SetRenderDrawColor(SDLWindow.Renderer, volume_controller[j].colors.r, volume_controller[j].colors.g, volume_controller[j].colors.b, volume_controller[j].colors.a);
                            SDL_RenderFillRect(SDLWindow.Renderer, &volume_controller[j].blocks[i]);
                        }
                    }
                    
                } break;
                case VOL_SETTINGS_SFX:
                {
                    UpdateAndRenderVolumeBars(&volume_controller[2]);
                    for (int j = 0; j < 3; ++j)
                    {
                        for (int i = 0; i < volume_controller[j].index; ++i)
                        {
                            SDL_SetRenderDrawColor(SDLWindow.Renderer, volume_controller[j].colors.r, volume_controller[j].colors.g, volume_controller[j].colors.b, volume_controller[j].colors.a);
                            SDL_RenderFillRect(SDLWindow.Renderer, &volume_controller[j].blocks[i]);
                        }
                    }

                } break;
                case VOL_SETTINGS_APPLY:
                {
                    // TODO: Audio change should only update on apply 
                    switch (settings_option_index)
                    {
                        case 4:
                        {
                            if (volume_controller[0].apply)
                            {
                                printf("volume_apply\n");
                                music_volume[0].music.volume = volume_controller[0].volume;
                                volume_controller[0].apply = false;
                            }
                        }
                    }


                    for (int j = 0; j < 3; ++j)
                    {
                        for (int i = 0; i < volume_controller[j].index; ++i)
                        {
                            SDL_SetRenderDrawColor(SDLWindow.Renderer, volume_controller[j].colors.r, volume_controller[j].colors.g, volume_controller[j].colors.b, volume_controller[j].colors.a);
                            SDL_RenderFillRect(SDLWindow.Renderer, &volume_controller[j].blocks[i]);
                        }
                    }
                } break;
                default:
                {
                    volume_settings_state = VOL_SETTINGS_NONE;
                } break;
            }
        }


        if (is_new_game)
        {
            SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);

            if (character_creation_screen.is_active)
            {
                CursorForAssets(&character_creation_screen.info[character_creation_screen.index].asset, &up_cursor_asset, 4, 16);
                RenderAndUpdateAsset(&up_cursor_asset);
                for (int i = 0; i < ArraySize(back_or_next_cursor); ++i)
                {
                    RenderText(SDLWindow.Renderer, font_atlas, 
                               back_or_next_cursor[i].asset.x - 12,
                               back_or_next_cursor[i].asset.y + 16,
                               back_or_next_cursor[i].name,
                               white);
                    RenderAndUpdateAsset(&back_or_next_cursor[i].asset);
                }

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
                

                SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(SDLWindow.Renderer, &character_creation_screen.description_box);
            
                if (!confirmation.is_active)
                {
                    switch (character_class_selection_state)
                    {
                        case CLASS_NONE:
                        {

                        } break;
                        case CLASS_KNIGHT:
                        {
                            // TODO: Initialize class into character data entirely
                            PushString(character_data.class.name, "Knight");
                            printf("player_class: %s\n", character_data.class.name);
        
                            character_data.class.base_stats = class_base_stats[0];
                            printf("player_class strength: %d\n", character_data.class.base_stats.strength);

                            confirmation.is_active = true; 
                        } break;
                        case CLASS_PALADIN:
                        {
                           confirmation.is_active = true; 
                        } break;
                        case CLASS_MAGE:
                        {
                           confirmation.is_active = true; 
                        } break;
                        case CLASS_ARCHER:
                        {
                           confirmation.is_active = true; 
                        } break;
                        default:
                        {
                            character_class_selection_state = CLASS_NONE;
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
       
                CursorForAssets(&character_allocation_select_screen.info[character_allocation_select_screen.index].asset, &up_cursor_asset, 24, 20);
                RenderAndUpdateAsset(&up_cursor_asset);
                
                RenderTextWithNewlines(SDLWindow.Renderer, font_atlas,
                           (ASPECT_WIDTH / 2) - 80,
                           (ASPECT_HEIGHT / 2) - 48,
                           "How will you allocate your\npoints for your character?",
                           white,
                           2);
                  
                for (int i = 0; i < ArraySize(back_or_next_cursor); ++i)
                {
                    RenderText(SDLWindow.Renderer, font_atlas, 
                               back_or_next_cursor[i].asset.x - 12,
                               back_or_next_cursor[i].asset.y + 16,
                               back_or_next_cursor[i].name,
                               white);
                    RenderAndUpdateAsset(&back_or_next_cursor[i].asset);
                }

    
                for (int i = 0; i < ArraySize(character_allocation_select_screen.info); ++i)
                {
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

                    RenderAndUpdateAsset(&character_allocation_select_screen.info[i].asset);
                }
                  
                SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(SDLWindow.Renderer, &character_allocation_select_screen.description_box);
           
                if (!confirmation.is_active)
                {
                    switch (character_class_point_allocation_method_state)
                    {
                        case CLASS_POINTS_NONE:
                        {

                        } break;
                        case CLASS_POINTS_PERSONALITY:
                        {
                            confirmation.is_active = true;
                        } break;
                        case CLASS_POINTS_PRESET:
                        {
                            confirmation.is_active = true;
                        } break;
                        case CLASS_POINTS_MANUAL:
                        {
                            confirmation.is_active = true;
                        } break;
                        default:
                        {
                            character_class_point_allocation_method_state = CLASS_POINTS_NONE;
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
                    {
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
                    ++questions_answered_index;
                    
                    static bool follow_up_question_selected = false;
                    if (!follow_up_question_selected)
                    {
                        int skip_count = 5;
                        int total = ArraySize(question_table) - skip_count;
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
                        int total = ArraySize(question_table) ;
                        rand_index = rand() % 5;
                        first_question_selected = true;
                    }
                }  

                SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                RenderWrappedTextCentered(SDLWindow.Renderer, font_atlas, 
                                      question_table[questions_answered_index], white, 
                                      0, -32,        // containerX, containerY
                                      256, 240,    // containerW, containerH
                                      2);          // lineSpacing

                if (is_quiz)
                {
                    switch (character_class_personality_test_result_state)
                    {
                        case CLASS_PERSONALITY_RESULT_NONE:
                        {
                            confirmation.is_active = true;
                        } break;
                        case CLASS_PERSONALITY_RESULT_VILLAGE:
                        {
                            is_village = true;
                            is_quiz = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_MONSTER:
                        {
                            is_monster = true;
                            personality_scenario[MONSTER].scenario.result |= SCENARIO_MONSTER;
                            personality_scenario[MONSTER].is_active = true;
                            is_quiz = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_FOREST:
                        {
                            is_forest = true;
                            is_quiz = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_CAVE:
                        {
                            is_cave = true;
                            is_quiz = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_DESERT:
                        {
                            is_desert = true;
                            is_quiz = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_TOWER:
                        {
                            is_tower = true;
                            is_quiz = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_THEATER:
                        {
                            is_theater = true;
                            is_quiz = false;
                        } break;
                        case CLASS_PERSONALITY_RESULT_CASTLE:
                        {
                            is_castle = true;
                            is_quiz = false;
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

                if (is_village)
                {
                    printf("is_village!\n");
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);

                }

                //if (is_monster)
                if (personality_scenario[MONSTER].scenario.result & SCENARIO_MONSTER)
                {
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font_atlas,
                                   CenterRenderedText(screen_center_x, "Scenario", 0), 
                                   screen_center_y - 108,
                                   "Scenario", 
                                   white);

                    // TODO: Render box and text together 
                    RenderWrappedTextCentered(SDLWindow.Renderer, font_atlas, 
                                              monster_scenario[0], white, 
                                              8, -64,        // containerX, containerY
                                              224, 240,    // containerW, containerH
                                              2);          // lineSpacing
                   
                    personality_scenario[MONSTER].scenario_box.x = screen_center_x - 124; 
                    personality_scenario[MONSTER].scenario_box.y = screen_center_y - 96; 
                    personality_scenario[MONSTER].scenario_box.w = 240; 
                    personality_scenario[MONSTER].scenario_box.h = 64; 

                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(SDLWindow.Renderer, &personality_scenario[MONSTER].scenario_box);

                    if (personality_scenario[MONSTER].is_active)
                    {
                        CursorForItems(&personality_scenario[MONSTER].info.monster_options[personality_scenario[MONSTER].index], &right_cursor_asset, 4, 1);
                        RenderAndUpdateAsset(&right_cursor_asset);

                        for (int i = 0; i < ArraySize(personality_scenario[MONSTER].info.monster_options); ++i)
                        {
                            RenderTextWithNewlines(SDLWindow.Renderer, font_atlas,
                                                   personality_scenario[MONSTER].info.monster_options[i].x,
                                                   personality_scenario[MONSTER].info.monster_options[i].y,
                                                   personality_scenario[MONSTER].info.monster_options[i].text,
                                                   white,
                                                   2);
                        }
                    }


                    //is_monster = false;
                
                    //is_name_submission = true;
                    //entering_glyph = true;
                }
                if (is_forest)
                {
                    printf("is_forest!\n");
                }
                if (is_cave)
                {
                    printf("is_cave!\n");
                }
                if (is_desert)
                {
                    printf("is_desert!\n");
                }
                if (is_tower)
                {
                    printf("is_tower!\n");
                }
                if (is_theater)
                {
                    printf("is_theater!\n");
                }
                if (is_castle)
                {
                    printf("is_castle!\n");
                }  
            }

            if (is_name_submission) //enter name screen
            {
                SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);

                // Create a grid of glyphs to iterate over either with menu_items_t or class_items_t so
                // we can iterate through each one with the cursor. Entering a glyph will push that onto 
                // a stack, where that stack is the name bar where you spell out your name. Deleting a 
                // glyph will pop it off the stack. Name limit can be set.
                              
                CursorForItems(&glyph_grid[glyph_index], &right_cursor_asset, 4, 1);
                RenderAndUpdateAsset(&right_cursor_asset);
                for (int i = 0; i < ArraySize(glyph_grid); ++i)
                {
                    RenderText(SDLWindow.Renderer, font_atlas,
                            glyph_grid[i].x, 
                            glyph_grid[i].y,
                            glyph_grid[i].text, 
                            white);

                }
             
                if (entering_glyph && name_limit <= 10)
                {
                    switch (character_class_name_submission_state)
                    {
                        case CLASS_NAME_NONE:
                        {
                        } break;
                        case CLASS_NAME_ENTER:
                        { 
                            is_glyph_selected = true;
                            entering_glyph = false;
                        } break;
                        case CLASS_NAME_DELETE:
                        {
                            is_glyph_deleted = true;
                            entering_glyph = false;
                        } break;
                        default:    
                        {
                            character_class_name_submission_state = CLASS_NAME_NONE;
                        } break;
                    }
                }
                

                if (is_glyph_selected)
                {
                    // TODO: Remove selected glyph
                    // Render selected glyph
                    printf("select!\n");
                    printf("name_pos: %d\n", name_pos);
                    if (name_limit > 0 && name_pos <= 10)
                    {
                        temp_glyph = test_glyph_grid[glyph_index].glyph;
                        PushCharStack(name_array[name_pos].str, temp_glyph);
                        PushCharStack(character_data.name, temp_glyph);
                        --name_limit;
                        ++name_pos;
                    }

                    printf("player_name: %s\n", character_data.name);
                    printf("name_limit: %d\n", name_limit);
                    is_glyph_selected = false;
                }
            
                if (is_glyph_deleted)
                {
                    printf("delete!\n");
                    printf("name_pos: %d\n", name_pos);
                    if (name_limit < 10 && name_pos >= 0)
                    {
                        // Update the position before popping, otherwise we're popping the empty char from the new position 
                        // PushCharStack had placed us, and that results in having to press the backspace twice to delete, and 
                        // that leaves a weird render artifact for the glyphs to be rendered ontop of each other.
                        ++name_limit;
                        --name_pos;
                        temp_glyph = test_glyph_grid[glyph_index].glyph;
                        PopStack(name_array[name_pos].str);
                        PopStack(character_data.name);
                    }

                    printf("player_name: %s\n", character_data.name);
                    printf("name_limit: %d\n", name_limit);
                    is_glyph_deleted = false;
                }


                for (int i = 0; i < ArraySize(name_bar_underline); ++i)
                {
                    RenderText(SDLWindow.Renderer, font_atlas,
                           name_bar_underline[i].x,
                           name_bar_underline[i].y,
                           name_bar_underline[i].text,
                           white);
                }

                // Array sitting outside to hold an array of chars that's passed in for every glyph we've pressed enter on, from which we pushed into
                for (int i = 0; i < ArraySize(name_array); ++i)
                {
                    RenderText(SDLWindow.Renderer, font_atlas,
                               name_bar_underline[i].x, 
                               name_bar_underline[i].y - 4,
                               name_array[i].str, 
                               white);
                }

            }
        }

    
        // Final screen from new_game is the character overview into the game


        if (is_game_running)
        {
            //PlayMusic(&ambience);
            UpdatePlayer(&player_asset, enemy_arr, &sfx_move);
            for (int i = 0; i < ArraySize(enemy_arr); ++i)
            {
                if (AABB(&player_asset.body, &enemy_arr[i].body))
                {
                    CombatCheck(&player_asset, &enemy_arr[i]);
                    CombatUpdate(&player_asset, &enemy_arr[i], &sfx_attack);
                    CollisionXCheck(&player_asset, &enemy_arr[i]);
                    CollisionYCheck(&player_asset, &enemy_arr[i]);
                }
            }

            for (int i = 0; i < ArraySize(walls); ++i)
            {
                if (AABB(&player_asset.body, &walls[i].body))
                {
                    CollisionXCheck(&player_asset, &walls[i]);
                    CollisionYCheck(&player_asset, &walls[i]);
                }
            }


            if (AABB(&player_asset.body, &down_stairs_asset.body))
            {
               
            }

            if (room_select == 1)
            {
                AttachCameraToPlayer(&player_asset, &room_asset[0]);
            } 
            else if (room_asset == 2)
            {
                AttachCameraToPlayer(&player_asset, &room_asset[1]);
            }


            SDL_SetRenderTarget(SDLWindow.Renderer, SDLCamera.TargetTexture);
            SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
            SDL_RenderClear(SDLWindow.Renderer);

            if (room_select == 1)
            {
                RenderAndUpdateAsset(&room_asset[0]);
            }
            else if (room_select == 2)
            {
                RenderAndUpdateAsset(&room_asset[1]);
            }
            
            RenderAndUpdateAsset(&down_stairs_asset);
            RenderAndUpdateAsset(&player_asset);



            for (int i = 0; i < ArraySize(enemy_arr); ++i)
            {   
                if (enemy_arr[i].texture)
                {
                    RenderAndUpdateAsset(&enemy_arr[i]);
                }
            }

            for (int i = 0; i < ArraySize(walls); ++i)
            {
                RenderAndUpdateAsset(&walls[i]);
            }

            // Set render target to camera
            SDL_SetRenderTarget(SDLWindow.Renderer, NULL);
            SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
            SDL_RenderClear(SDLWindow.Renderer);
        
            // Render to camera
            SDL_RenderCopy(SDLWindow.Renderer, SDLCamera.TargetTexture, NULL, NULL);
        }

        SDL_RenderSetLogicalSize(SDLWindow.Renderer, ASPECT_WIDTH, ASPECT_HEIGHT);    
        SDL_RenderPresent(SDLWindow.Renderer);

        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < frames_per_ms)
        {
            SDL_Delay(frames_per_ms - frame_time);
        }


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


