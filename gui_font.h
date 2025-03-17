#ifndef gui_font_H
#define gui_font_H

#include "common.h"

extern const u8 ASCII2Font[];
extern const u8 GUIFontData[][5];

typedef struct text_t
{   
    int font_height;
    int font_width;
    SDL_Texture *font_atlas;
} text_t;

text_t CreateText(SDL_Texture *font_atlas);

SDL_Texture* CreateFontAtlas(SDL_Renderer *renderer);
void RenderText(SDL_Renderer *renderer, SDL_Texture *fontAtlas, int x, int y,
                const char *text, SDL_Color color);
char** WrapText(const char *text, int maxWidth, int *outLineCount);
void RenderWrappedTextCentered(SDL_Renderer *renderer, SDL_Texture *fontAtlas,
                               const char *text, SDL_Color color,
                               int containerX, int containerY,
                               int containerW, int containerH,
                               int lineSpacing /* extra spacing between lines */);

void RenderWrappedText(SDL_Renderer *renderer, SDL_Texture *fontAtlas,
                       const char *text, SDL_Color color,
                       int containerX, int containerY,
                       int containerW, int containerH,
                       int lineSpacing);

void RenderTextWithNewlines(SDL_Renderer *renderer, SDL_Texture *fontAtlas,
                            int startX, int startY,
                            const char *text, SDL_Color color, int lineSpacing);

#endif
