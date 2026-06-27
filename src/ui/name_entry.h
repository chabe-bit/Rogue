#ifndef NAME_ENTRY_H
#define NAME_ENTRY_H

#include "common.h"

#define NAME_ENTRY_LIMIT 10
#define NAME_ENTRY_GRID_ROWS 6 
#define NAME_ENTRY_GRID_COLS 10

typedef struct
{
    bool is_active;
    bool glyph_entered;
    bool glyph_deleted;
    bool name_confirmed;

    u32 index;
    u32 name_pos;
    u32 name_limit;
    u32 current_row;
    u32 current_col;

    char name[NAME_ENTRY_LIMIT];
} name_entry_t;

typedef struct 
{
    vec2_t pos;
    char name[NAME_ENTRY_LIMIT];
    char *underline;
} name_entry_bar_t;

typedef struct
{
    vec2_t pos;
    char ascii;
} name_entry_ascii_t;

typedef struct
{
    vec2_t pos;
    char *glyph;
} name_entry_glyph_t;

extern name_entry_ascii_t ascii_to_glyph_grid[50];
extern name_entry_glyph_t glyph_grid[60];
extern name_entry_bar_t name_entry_bar[NAME_ENTRY_LIMIT];

void NameEntry_Init(name_entry_t *name_entry);

void NameEntry_MoveUp(name_entry_t *name_entry);
void NameEntry_MoveDown(name_entry_t *name_entry);
void NameEntry_MoveLeft(name_entry_t *name_entry);
void NameEntry_MoveRight(name_entry_t *name_entry);

void NameEntry_EnterGlyph(name_entry_t *name_entry, 
                          name_entry_bar_t *name_entry_bar, 
                          name_entry_ascii_t *ascii_to_glyph_grid);

void NameEntry_DeleteGlyph(name_entry_t *name_entry, 
                           name_entry_bar_t *name_entry_bar);

void NameEntry_RenderNameUnderline(name_entry_bar_t *name_entry_bar, 
                                   SDL_Renderer *Renderer, 
                                   SDL_Texture *font_atlas, 
                                   SDL_Color color);

void NameEntry_RenderName(name_entry_bar_t *name_entry_bar, 
                          SDL_Renderer *Renderer, 
                          SDL_Texture *font_atlas, 
                          SDL_Color color);

char *NameEntry_GetName(name_entry_t *name_entry);
void NameEntry_ConfirmName(name_entry_t *name_entry, 
                           name_entry_bar_t *name_entry_bar);








#endif
