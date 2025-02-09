#include "common.h"
#include "gui.h"

#include <windows.h>
#include <xinput.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <SDL2/SDL.h>

#define WINDOW_WIDTH  480
#define WINDOW_HEIGHT 464

#define ASPECT_WIDTH  256
#define ASPECT_HEIGHT 240

static bool Running;



#define ArraySize(array) (sizeof(array) / sizeof((array)[0]))

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
    TITLE_NEW_GAME,
    TITLE_LOAD_GAME,
    TITLE_SETTINGS,
    TITLE_EXIT,
    TITLE_UNKNOWN,
} title_screen_state_e;
title_screen_state_e title_screen_state = TITLE_UNKNOWN;

typedef enum
{
    NEW_GAME_CHARACTER_SELECT,
    NEW_GAME_UNKNOWN
} new_game_state_e;
new_game_state_e new_game_state = NEW_GAME_CHARACTER_SELECT;

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
    u32 length;
    u8 *buffer;
    SDL_AudioSpec spec;
    SDL_AudioDeviceID device_id;
} wav_t;

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

// TODO: Create slider for volume adjustment in settings
static void 
MixedAudio(wav_t *sound, mix_audio_t *mix_audio, int volume)
{
    if (volume < 0)
        volume = 0;
    if (volume > 128)
        volume = SDL_MIX_MAXVOLUME;

    if (!mix_audio->audio)
    {
        fprintf(stderr, "mix_audio->audio is NULL, expected allocation.\n");
        return;
    }

    // TODO: One big malloc where we pass in an array of audio to mix
    //SDL_AudioFormat format = AUDIO_S16LSB;


    SDL_MixAudioFormat(mix_audio->audio, 
                       sound->buffer, 
                       mix_audio->format,
                       sound->length,
                       volume);
    
    // Maybe return this function as a u8* and pass into PlayMusic as an option?
    // or pack mix_audio_t into wav_t as an option innately, user doesn't have to use if not needed

    if (SDL_GetQueuedAudioSize(sound->device_id) == 0)
    {
        SDL_QueueAudio(sound->device_id, mix_audio->audio, sound->length);            
    }
    SDL_PauseAudioDevice(sound->device_id, 0);
}

static void
PlaySFX(wav_t *sound)
{
    SDL_ClearQueuedAudio(sound->device_id);
    SDL_QueueAudio(sound->device_id, sound->buffer,          
                   sound->length);
    SDL_PauseAudioDevice(sound->device_id, 0);  
}

static void 
PauseAudio(wav_t *sound)
{
    SDL_PauseAudioDevice(sound->device_id, 1);
}

#define GLYPH_WIDTH  6
#define GLYPH_HEIGHT 5
#define NUM_GLYPHS ArraySize(GUIFontData) 

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

void TestRenderText(SDL_Renderer *renderer, text_t *text_data, const char *text, int x, int y, SDL_Color color)
{
    // Since our atlas is white, we can tint it by setting the texture color mod.
    SDL_SetTextureColorMod(text_data->font_atlas, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(text_data->font_atlas, color.a);

    const int atlas_cols = 16;

    // For each character in the text:
    for (const char *p = text; *p != '\0'; p++) {
        u1 ascii = (u1)*p;
        // Look up the corresponding glyph index.
        // (Assuming ASCII2Font is defined for your ASCII range.)
        u1 glyph_index = ASCII2Font[ascii];

        // Compute source rectangle in the atlas.
        SDL_Rect src;
        src.x = (glyph_index % atlas_cols) * GLYPH_WIDTH;
        src.y = (glyph_index / atlas_cols) * GLYPH_HEIGHT;
        src.w = GLYPH_WIDTH;
        src.h = GLYPH_HEIGHT;

        // Destination rectangle on screen.
        SDL_Rect dst;
        dst.x = x;
        dst.y = y;
        dst.w = GLYPH_WIDTH;
        dst.h = GLYPH_HEIGHT;

        // Render this glyph.
        SDL_RenderCopy(renderer, text_data->font_atlas, &src, &dst);

        // Advance the x position.
        x += GLYPH_WIDTH;
    }
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
const char *question_table[49] = 
{
    // Starting questions 
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
    "Do you have trouble sleeping because you are thining too much?",
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

void AskStartingQuestion(SDL_Renderer *renderer, SDL_Texture *font_atlas)
{
    SDL_Color white = {255, 255, 255, 255};

    // Only the starting questions
    static bool first_question_selected = false;
    static int rand_index = -1;
    if (!first_question_selected)
    {
        rand_index = rand() % 5;
        first_question_selected = true;
    }
    
    char buffer[512];
    
    int i = 1;
    snprintf(buffer, ArraySize(question_table) + 100, "%d. %s", i, question_table[rand_index]);

    // Skips past the starting questions
    static bool follow_up_question_select = false;
    static int rand_index2 = -1;
    if (!follow_up_question_select)
    {
        int skip_count = 5;
        int total = ArraySize(question_table) - skip_count;
        rand_index2 = skip_count + (rand() % total);
        follow_up_question_select = true;
    }

    RenderWrappedTextCentered(renderer, font_atlas, 
                          buffer, white, 
                          0, -32,        // containerX, containerY
                          256, 240,    // containerW, containerH
                          2);          // lineSpacing
   

    ++i;
    
    


    printf("%s\n", question_table[rand_index]);
}

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

void NewGameOption()
{
    // Implement logic on new_game select
    
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

// New game screen
typedef struct new_game_data_t
{
    bool class_has_been_selected;
    
    /* Values of stats can be negative if debuffs are applied, thus using ints */

    // STR determines the physical damage of your character from weapons
    int strength;
    
    // DEX determines the accuracy and evasiveness of your character
    int dexterity;

    // END determines the defense of your character
    int toughness;

    // VIT determines the amount of health of your character
    int vitality;

    // INT determines the magic damage and amount of mana of your character
    int intelligense;

    const char name[64];
} new_game_data_t;

typedef struct font_t
{
    SDL_Texture *font_atlas;
} font_t;


typedef struct input_t
{
    u8 *key;
} input_t;

typedef struct title_screen_t
{
    int index;
    input_t input;
    font_t font;
    wav_t theme;
    asset_t screen;
    asset_t cursor;
    SDL_Color color;
    menu_item_t menu[4];
} title_screen_t;

void UpdateTitleScreen(title_screen_t *title_screen)
{
    if (title_screen->input.key[SDL_SCANCODE_RETURN])
    {
        if (title_screen->index == TITLE_NEW_GAME)
        {
            game_state = GAME_STATE_NEW_GAME;
        }
        
        if (title_screen->index == TITLE_LOAD_GAME)
        {
            game_state = GAME_STATE_LOAD_GAME;
        }

    }
    
    PlayMusic(&title_screen->theme); 
    int index = title_screen->index;
    CursorForItems(&title_screen->menu[index], &title_screen->cursor, 4, 1);
}

void RenderTitleScreen(title_screen_t *title_screen)
{
    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
    SDL_RenderClear(SDLWindow.Renderer);
    SDL_RenderCopy(SDLWindow.Renderer, title_screen->screen.texture, NULL, &title_screen->screen.body);

    RenderAndUpdateAsset(&title_screen->cursor);
    for (int i = 0; i < ArraySize(title_screen->menu); ++i)
    {
        RenderText(SDLWindow.Renderer, title_screen->font.font_atlas, title_screen->menu[i].x, title_screen->menu[i].y,
                   title_screen->menu[i].text, title_screen->color);
    }
}

typedef struct new_game_screen_t
{
    int index;
    input_t input;
    font_t font;
    wav_t theme;
    asset_t screen;
    asset_t cursor;
    SDL_Color color;
    menu_item_t menu[2];   
} new_game_screen_t;

void UpdateNewGameScreen(new_game_screen_t *new_game_screen)
{
    int index = new_game_screen->index;
    CursorForItems(&new_game_screen->menu[index], &new_game_screen->cursor, 4, 1);
}

void RenderNewGameScreen(new_game_screen_t *new_game_screen)
{
    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
    SDL_RenderClear(SDLWindow.Renderer);
    SDL_RenderCopy(SDLWindow.Renderer, new_game_screen->screen.texture, NULL, &new_game_screen->screen.body);


    RenderAndUpdateAsset(&new_game_screen->cursor);
    for (int i = 0; i < ArraySize(new_game_screen->menu); ++i)
    {
        RenderText(SDLWindow.Renderer, new_game_screen->font.font_atlas, new_game_screen->menu[i].x, new_game_screen->menu[i].y,
                    new_game_screen->menu[i].text, new_game_screen->color);
    }
}


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
    /*
   typedef struct wav_t
{
    u32 length;
    u8 *buffer;
    SDL_AudioSpec spec;
    SDL_AudioDeviceID device_id;
} wav_t;
*/
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

    
    // 2 different music "files" 
    music_t music_volume[2] = {0};
    LoadWavFile(&music_volume[0].music, "assets/music/5. Smooth As Glass.wav");
    LoadWavFile(&music_volume[1].music, "assets/music/4. Church of Order.wav");

    // 2 different sfx "files"
    sfx_t sfx_volume[2] = {0};
    LoadWavFile(&sfx_volume[0].sfx, "assets/sfx/fire_a.wav"); 
    LoadWavFile(&sfx_volume[1].sfx, "assets/sfx/fire_b.wav"); 

    master_volume_t master_volume = {0};
    master_volume.volume = 32;
    for (int i = 0; i < 2; ++i)
    {
        master_volume.music[i] = music_volume[i];
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

    master_volume.sfx[0].volume = 0;
    

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


    int selected_character_index = 0;
    class_select_t class_select[4] = {0};
    for (int i = 0; i < ArraySize(class_select); ++i)
    {
        LoadAsset(&class_select[i].asset, class_files[i]);
        class_select[i].name = class_names[i];
        class_select[i].description = class_description[i];
    }

    InitializeAssetToRender(&class_select[0].asset, screen_center_x - 96, screen_center_y, 
                            class_select[0].asset.w, class_select[0].asset.h);
    InitializeAssetToRender(&class_select[1].asset, screen_center_x - 36, screen_center_y, 
                            class_select[1].asset.w, class_select[1].asset.h);
    InitializeAssetToRender(&class_select[2].asset, screen_center_x + 28, screen_center_y, 
                           class_select[2].asset.w, class_select[2].asset.h);
    InitializeAssetToRender(&class_select[3].asset, screen_center_x + 80, screen_center_y, 
                            class_select[3].asset.w, class_select[3].asset.h);

    SDL_Rect description_box;
    description_box.x = screen_center_x - 112; // 16px padding from start of X
    description_box.y = screen_center_y + 48;
    description_box.w = 224; // 16px padding from end of X
    description_box.h = 64;

    const char *stat_options_name[2] = {
        "Personality\n   Test",
        "  Manual\nAllocation"
    };
    
    const char *stat_options_description[2] = {
        "Take a personality test to determine your point allocation.",
        "Manually allocate your points."
    };

    
    int stat_option_select = 0;
    class_select_t stat_options[2] = {0};
    for (int i = 0; i < ArraySize(stat_options); ++i)
    {
        //LoadAsset(&stat_options[i].asset, "assets/up_cursor.png");
        stat_options[i].name = stat_options_name[i];
        stat_options[i].description = stat_options_description[i];
        stat_options[i].asset.w = 81;
        stat_options[i].asset.h = 81;
    }
    

    InitializeAssetToRender(&stat_options[0].asset, screen_center_x - 96, screen_center_y + 32, 
                            stat_options[0].asset.w, stat_options[0].asset.h);
    InitializeAssetToRender(&stat_options[1].asset, screen_center_x + 36, screen_center_y + 32, 
                            stat_options[1].asset.w, stat_options[1].asset.h);


    u8 *input = SDL_GetKeyboardState(NULL);
    input_t input_ = {0};
    input_.key = SDL_GetKeyboardState(NULL);

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
    menu_item_t settings_options[4] = {
        { "Volume", screen_center_x - (GLYPH_WIDTH * strlen(settings_options[0].text) - 1) / 2, screen_center_y - 48 },
        { "Master", screen_center_x - (GLYPH_WIDTH * strlen(settings_options[1].text) - 1) / 2, screen_center_y + 0 },
        { "Music", screen_center_x - (GLYPH_WIDTH * strlen(settings_options[2].text) - 1) / 2, screen_center_y + 24 },
        { "SFX", screen_center_x - (GLYPH_WIDTH * strlen(settings_options[3].text) - 1) / 2, screen_center_y + 48 },
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

    int block_size = 0;
    int block_index = 0;
    SDL_Rect volume_bar_blocks[5] = {0};
    for (int i = 0; i < ArraySize(volume_bar_blocks); ++i)
    {
        volume_bar_blocks[i].x = master_volume_bar.x + block_size;
        volume_bar_blocks[i].y = master_volume_bar.y;
        volume_bar_blocks[i].w = master_volume_bar.w / 4;
        volume_bar_blocks[i].h = master_volume_bar.h;
        
        block_size += (master_volume_bar.w / 4);
    }
    


    // min -> 0 
    // max -> 128
    // divided into blocks of 4, size of 32 each

    // Master Volume [ |  |  |  ]
    // ...


    // Start event
    bool some_input = true;
    int questions_answered_index = 0;
    is_title_screen = true;
    Running = true;
    int volume = 32;
    while (Running)
    {
        frame_start = SDL_GetTicks();
    
        // TODO: Figure out how to update sound
        master_volume.sfx[0].volume = volume;
        SDL_MixAudioFormat(sfx_mix.audio,  // dst 
                   master_volume.sfx[0].sfx.buffer,     // src 
                   sfx_mix.format, // format
                   master_volume.sfx[0].sfx.length,     // length
                   master_volume.sfx[0].volume);           // volume      
                                                           
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
                            Orientation.up = true;
                            if (is_title_screen)
                            {
                                option_index--;
                                if (option_index < 0)
                                    option_index = ArraySize(menu_items) - 1;
                                //PlaySFX(&sfx_option);  
                                SDL_ClearQueuedAudio(master_volume.sfx[0].sfx.device_id);
                                SDL_QueueAudio(master_volume.sfx[0].sfx.device_id, sfx_mix.audio,          
                                               master_volume.sfx[0].sfx.length);
                                SDL_PauseAudioDevice(master_volume.sfx[0].sfx.device_id, 0);  

                            }
                            
                            if (is_new_game)
                            {
                                button_select--;
                                if (button_select < 0)
                                    button_select = ArraySize(confirmation_buttons) - 1;
                                PlaySFX(&sfx_option);
                            }
                            
                            if (is_confirmation)
                            {
                                confirmation_select--;
                                if (confirmation_select < 0)
                                    confirmation_select = ArraySize(confirmation_buttons) - 1;
                                PlaySFX(&sfx_option);
                            }

                            if (is_settings)
                            {
                                settings_option_index--;
                                if (settings_option_index < 0)
                                    settings_option_index = ArraySize(settings_options) - 1;
                            }
                        
                            break;
                        case SDLK_s:
                            Orientation.down = true;

                            if (is_title_screen)
                            {
                                option_index++;
                                if (option_index >= ArraySize(menu_items))
                                    option_index = 0;
                                //PlaySFX(&sfx_option);  
                                SDL_ClearQueuedAudio(master_volume.sfx[0].sfx.device_id);
                                SDL_QueueAudio(master_volume.sfx[0].sfx.device_id, sfx_mix.audio,          
                                               master_volume.sfx[0].sfx.length);
                                SDL_PauseAudioDevice(master_volume.sfx[0].sfx.device_id, 0);  
                            }

                            if (is_new_game)
                            {
                                button_select++;
                                if (button_select >= ArraySize(confirmation_buttons))
                                    button_select = 0;
                                PlaySFX(&sfx_option);
                            }
    
                            if (is_confirmation)
                            {
                                confirmation_select++;
                                if (confirmation_select >= ArraySize(confirmation_buttons))
                                    confirmation_select = 0;
                                PlaySFX(&sfx_option);
                            }

                            if (is_settings)
                            {
                                settings_option_index++;
                                if (settings_option_index >= ArraySize(settings_options))
                                    settings_option_index = ArraySize(settings_options) - 1;
                            }
                            


                            break;
                        case SDLK_a:
                            Orientation.left = true;
                            if (is_class_select && !is_confirmation)
                            {
                                selected_character_index--;
                                if (selected_character_index < 0)
                                    selected_character_index = ArraySize(class_select) - 1;
                                PlaySFX(&sfx_option);

                            }

                            if (class_has_been_selected)
                            {
                                stat_option_select--;
                                if (stat_option_select < 0)
                                    stat_option_select = ArraySize(stat_options) - 1;
                                PlaySFX(&sfx_option);
                            }
                       
                            if (is_settings)
                            {
                                block_index--;
                                if (block_index < 0)
                                    block_index = 0;

                                volume -= 32;
                                if (volume <= 0)
                                {
                                    volume = 0;
                                }
                                SDL_ClearQueuedAudio(master_volume.sfx[0].sfx.device_id);
                                SDL_QueueAudio(master_volume.sfx[0].sfx.device_id, sfx_mix.audio,          
                                               master_volume.sfx[0].sfx.length);
                                SDL_PauseAudioDevice(master_volume.sfx[0].sfx.device_id, 0);  

                                printf("left -> block_index: %d\n", block_index);
                            } 

                            break;
                        case SDLK_d:
                            Orientation.right = true;
                            if (is_class_select && !is_confirmation)
                            {
                                selected_character_index++;
                                if (selected_character_index >= ArraySize(class_select))
                                    selected_character_index = 0;
                                PlaySFX(&sfx_option);
                            }

                            if (class_has_been_selected)
                            {
                                stat_option_select++;
                                if (stat_option_select >= ArraySize(stat_options))
                                    stat_option_select = 0;
                                PlaySFX(&sfx_option);
                            }

                            if (is_settings)
                            {
                                block_index++;
                                if (block_index > ArraySize(settings_options))
                                    block_index = ArraySize(settings_options);
                               
                                volume += 32;
                                if (volume >= 128)
                                {
                                    volume = 128;
                                }
                                SDL_ClearQueuedAudio(master_volume.sfx[0].sfx.device_id);
                                SDL_QueueAudio(master_volume.sfx[0].sfx.device_id, sfx_mix.audio,          
                                               master_volume.sfx[0].sfx.length);
                                SDL_PauseAudioDevice(master_volume.sfx[0].sfx.device_id, 0);  
                                printf("right -> block_index: %d\n", block_index);
                            }

                            break;
                        case SDLK_q:
                        {

                        } break;
                        case SDLK_RETURN:
                        {
                            if (is_class_select && !is_confirmation)
                            {
                                is_confirmation = true;
                            }
    
                            if (class_has_been_selected && stat_option_select == 0)
                            {
                                class_has_been_selected = false;
                                is_personality_test = true;
                            }
                           
                            if (is_personality_test)
                            {
                                if (button_select == 0 || button_select ==1)
                                {
                                    question_confirmation = true;
                                }
                                
                            }
                          
                            if (!some_input)
                            {
                                is_settings_master_volume = true;
                            }

                        } break;
                        case SDLK_TAB:
                        {       
                            if (is_confirmation)
                            {
                                is_confirmation = false;
                                is_class_select = false;
                                class_has_been_selected = true;
                            }

                        } break;
                        default:
                            break;
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
            // Update
            //PlayMusic(&title_screen_theme);
            //PlayMusic(&master_volume.music[0].music);
            if (SDL_GetQueuedAudioSize(music_volume[0].music.device_id) == 0)
            {
                SDL_QueueAudio(music_volume[0].music.device_id, mix.audio, music_volume[0].music.length);            
            }
            SDL_PauseAudioDevice(music_volume[0].music.device_id, 0);
            
            //MixedAudio(&title_screen_theme, &theme_title_mixed, 20);
            if (input[SDL_SCANCODE_RETURN] && option_index == 0)
            {
                is_title_screen = false;
                is_new_game = true;
            }

            if (input[SDL_SCANCODE_RETURN] && option_index == 1)
            {
                is_title_screen = false;
                is_game_running = true;
                PauseAudio(&title_screen_theme);
            }
    
            if (input[SDL_SCANCODE_RETURN] && option_index == 2)
            {
                is_title_screen = false;
                is_settings = true;
            }
            
            if (input[SDL_SCANCODE_RETURN] && option_index == 3)
            {
                // TODO: Properly handle exit 
                ExitOption();
            }

            CursorForItems(&menu_items[option_index], &right_cursor_asset, 4, 1);
            
            // Render
            SDL_RenderCopy(SDLWindow.Renderer, title_screen_asset.texture, NULL, &title_screen_asset.body);
            
            for (int i = 0; i < ArraySize(menu_items); ++i)
            {
                RenderText(SDLWindow.Renderer, font_atlas, menu_items[i].x, menu_items[i].y,
                           menu_items[i].text, white);
            }
    
            RenderAndUpdateAsset(&right_cursor_asset);

        }
    
        if (is_settings)
        {
            SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                
            CursorForItems(&settings_options[settings_option_index], &right_cursor_asset, 6, 1);
            RenderAndUpdateAsset(&right_cursor_asset);
            
            for (int i = 0; i < ArraySize(settings_options); ++i) 
            {
                RenderText(SDLWindow.Renderer, font_atlas,
                           settings_options[i].x, 
                           settings_options[i].y,
                           settings_options[i].text, 
                           white);
            }

            SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(SDLWindow.Renderer, &master_volume_bar);
            SDL_RenderDrawRect(SDLWindow.Renderer, &music_volume_bar);
            SDL_RenderDrawRect(SDLWindow.Renderer, &sfx_volume_bar);
          
            if (settings_option_index == 1)
            {
                settings_test = true;
            }
        }
        
        if (settings_test)
        {
            for (int i = 0; i < ArraySize(volume_bar_blocks); ++i)
            {
                if (block_index == 1)
                {
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 255, 0, 255);
                    SDL_RenderFillRect(SDLWindow.Renderer, &volume_bar_blocks[0]);
                }
                
                if (block_index == 2)
                {
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 255, 0, 255);
                    SDL_RenderFillRect(SDLWindow.Renderer, &volume_bar_blocks[0]);
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 0, 255);
                    SDL_RenderFillRect(SDLWindow.Renderer, &volume_bar_blocks[1]);
                }
                
                if (block_index == 3)
                {
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 255, 0, 255);
                    SDL_RenderFillRect(SDLWindow.Renderer, &volume_bar_blocks[0]);
                    
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 0, 255);
                    SDL_RenderFillRect(SDLWindow.Renderer, &volume_bar_blocks[1]);
                    
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 165, 0, 255);
                    SDL_RenderFillRect(SDLWindow.Renderer, &volume_bar_blocks[2]);
                }

                if (block_index == 4)
                {
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 255, 0, 255);
                    SDL_RenderFillRect(SDLWindow.Renderer, &volume_bar_blocks[0]);
                    
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 0, 255);
                    SDL_RenderFillRect(SDLWindow.Renderer, &volume_bar_blocks[1]);
                    
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 165, 0, 255);
                    SDL_RenderFillRect(SDLWindow.Renderer, &volume_bar_blocks[2]);
                
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 0, 0, 255);
                    SDL_RenderFillRect(SDLWindow.Renderer, &volume_bar_blocks[3]);

                }
            }
        }



        if (is_new_game)
        {
            PauseAudio(&title_screen_theme);
            SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);

            is_class_select = true;
            if (is_class_select) 
            {
                CursorForAssets(&class_select[selected_character_index].asset, &up_cursor_asset, 4, 16);
                    
                for (int i = 0; i < ArraySize(back_or_next_cursor); ++i)
                {
                    RenderText(SDLWindow.Renderer, font_atlas, 
                               back_or_next_cursor[i].asset.x - 12,
                               back_or_next_cursor[i].asset.y + 16,
                               back_or_next_cursor[i].name,
                               white);
                    RenderAndUpdateAsset(&back_or_next_cursor[i].asset);
                }

                RenderAndUpdateAsset(&up_cursor_asset);
                for (int i = 0; i < ArraySize(class_select); ++i)
                {
                    RenderText(SDLWindow.Renderer, font_atlas, class_select[i].asset.x - 8, class_select[i].asset.y - 16, 
                               class_select[i].name, white);
                    RenderWrappedText(SDLWindow.Renderer, font_atlas, 
                                              class_select[selected_character_index].description, white, 
                                              description_box.x + 2,
                                              description_box.y + 8, // containerX, containerY
                                              description_box.w, 
                                              description_box.h,    // containerW, containerH
                                              4);          // lineSpacing
    
                    RenderAndUpdateAsset(&class_select[i].asset);
                }
                
                SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(SDLWindow.Renderer, &description_box);
               
                // TODO: Fix input to accept return
                if (is_confirmation)
                {
                    CursorForItems(&confirmation_buttons[confirmation_select], &right_cursor_asset, 4, 1);
                    RenderAndUpdateAsset(&right_cursor_asset);
                    for (int i = 0; i < ArraySize(confirmation_buttons); ++i) 
                    {
                        RenderText(SDLWindow.Renderer, font_atlas,
                                   confirmation_buttons[i].x, 
                                   confirmation_buttons[i].y,
                                   confirmation_buttons[i].text, 
                                   white);
                    }
                }
            }
        }


                 
        if (class_has_been_selected)
        {
            SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
   
            CursorForAssets(&stat_options[stat_option_select].asset, &up_cursor_asset, 24, -20);
            RenderAndUpdateAsset(&up_cursor_asset);
            
            RenderTextWithNewlines(SDLWindow.Renderer, font_atlas,
                       (ASPECT_WIDTH / 2) - 80,
                       (ASPECT_HEIGHT / 2) - 32,
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

            for (int i = 0; i < ArraySize(stat_options); ++i)
            {
                RenderTextWithNewlines(SDLWindow.Renderer, font_atlas, 
                           stat_options[i].asset.x, 
                           stat_options[i].asset.y, 
                           stat_options[i].name, 
                           white, 
                           2);
                RenderAndUpdateAsset(&stat_options[i].asset);
            }
        }

        if (is_personality_test)
        {
            static int rand_index = 0;

            if (question_confirmation)
            {
                printf("TRUE!\n");
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
                    rand_index = rand() % 5;
                    first_question_selected = true;
                }
                
            }  

            char buffer[512];
            snprintf(buffer, ArraySize(question_table) + 512, "%d. %s", questions_answered_index, question_table[questions_answered_index]);
            SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
            RenderWrappedTextCentered(SDLWindow.Renderer, font_atlas, 
                                  buffer, white, 
                                  0, -32,        // containerX, containerY
                                  256, 240,    // containerW, containerH
                                  2);          // lineSpacing

            CursorForItems(&confirmation_buttons[button_select], &right_cursor_asset, 12, 1);
            RenderAndUpdateAsset(&right_cursor_asset);
            for (int i = 0; i < ArraySize(confirmation_buttons); ++i)
            {
                RenderText(SDLWindow.Renderer, font_atlas, 
                           confirmation_buttons[i].x - 8, 
                           confirmation_buttons[i].y,
                           confirmation_buttons[i].text, 
                           white);
            }

        }

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


