#include "name_entry.h"

#include "gui_text.h"
#include "helper_funcs.h"

name_entry_ascii_t ascii_to_glyph_grid[50] = {
    { { CENTER_TEXT_X("A", - 52), SCREEN_CENTER_Y - 8}, 'A', }, 
    { { CENTER_TEXT_X("B", - 40), SCREEN_CENTER_Y - 8}, 'B', },
    { { CENTER_TEXT_X("C", - 28), SCREEN_CENTER_Y - 8}, 'C', }, 
    { { CENTER_TEXT_X("D", - 16), SCREEN_CENTER_Y - 8}, 'D', }, 
    { { CENTER_TEXT_X("E", - 4),  SCREEN_CENTER_Y - 8}, 'E', }, 
    { { CENTER_TEXT_X("F", + 16), SCREEN_CENTER_Y - 8}, 'F', }, 
    { { CENTER_TEXT_X("G", + 28), SCREEN_CENTER_Y - 8}, 'G', }, 
    { { CENTER_TEXT_X("H", + 40), SCREEN_CENTER_Y - 8}, 'H', }, 
    { { CENTER_TEXT_X("I", + 52), SCREEN_CENTER_Y - 8}, 'I', }, 
    { { CENTER_TEXT_X("J", + 64), SCREEN_CENTER_Y - 8}, 'J', }, 

    { { CENTER_TEXT_X("K", - 52), SCREEN_CENTER_Y + 2}, 'K', }, 
    { { CENTER_TEXT_X("L", - 40), SCREEN_CENTER_Y + 2}, 'L', }, 
    { { CENTER_TEXT_X("M", - 28), SCREEN_CENTER_Y + 2}, 'M', }, 
    { { CENTER_TEXT_X("N", - 16), SCREEN_CENTER_Y + 2}, 'N', }, 
    { { CENTER_TEXT_X("O", - 4),  SCREEN_CENTER_Y + 2}, 'O', }, 
    { { CENTER_TEXT_X("P", + 16), SCREEN_CENTER_Y + 2}, 'P', }, 
    { { CENTER_TEXT_X("Q", + 28), SCREEN_CENTER_Y + 2}, 'Q', }, 
    { { CENTER_TEXT_X("R", + 40), SCREEN_CENTER_Y + 2}, 'R', }, 
    { { CENTER_TEXT_X("S", + 52), SCREEN_CENTER_Y + 2}, 'S', }, 
    { { CENTER_TEXT_X("T", + 64), SCREEN_CENTER_Y + 2}, 'T', },

    { { CENTER_TEXT_X("U", - 52), SCREEN_CENTER_Y + 14}, 'U', }, 
    { { CENTER_TEXT_X("V", - 40), SCREEN_CENTER_Y + 14}, 'V', }, 
    { { CENTER_TEXT_X("W", - 28), SCREEN_CENTER_Y + 14}, 'W', }, 
    { { CENTER_TEXT_X("X", - 16), SCREEN_CENTER_Y + 14}, 'X', }, 
    { { CENTER_TEXT_X("Y", - 4),  SCREEN_CENTER_Y + 14}, 'Y', }, 
    { { CENTER_TEXT_X("Z", + 16), SCREEN_CENTER_Y + 14}, 'Z', }, 
    { { CENTER_TEXT_X("", + 28),  SCREEN_CENTER_Y + 14}, ' ', }, // Unused 
    { { CENTER_TEXT_X("", + 40),  SCREEN_CENTER_Y + 14}, ' ', }, // Unused
    { { CENTER_TEXT_X("", + 52),  SCREEN_CENTER_Y + 14}, ' ', }, // Unused
    { { CENTER_TEXT_X("", + 64),  SCREEN_CENTER_Y + 14}, ' ', }, // Unused


    // Special characters 
    { { CENTER_TEXT_X("[", - 52), SCREEN_CENTER_Y + 26}, '[', }, 
    { { CENTER_TEXT_X("]", - 40), SCREEN_CENTER_Y + 26}, ']', }, 
    { { CENTER_TEXT_X(".", - 28), SCREEN_CENTER_Y + 26}, '.', }, 
    { { CENTER_TEXT_X(",", - 16), SCREEN_CENTER_Y + 26}, ',', }, 
    { { CENTER_TEXT_X("!", - 4),  SCREEN_CENTER_Y + 26}, '!', }, 
    { { CENTER_TEXT_X("?", + 16), SCREEN_CENTER_Y + 26}, '?', }, 
    { { CENTER_TEXT_X("-", + 28), SCREEN_CENTER_Y + 26}, '-', }, 
    { { CENTER_TEXT_X("_", + 40), SCREEN_CENTER_Y + 26}, '_', }, 
    { { CENTER_TEXT_X(":", + 52), SCREEN_CENTER_Y + 26}, ':', }, 
    { { CENTER_TEXT_X(";", + 60), SCREEN_CENTER_Y + 26}, ';', }, 
    
    // Nums
    { { CENTER_TEXT_X("0", - 52), SCREEN_CENTER_Y + 38}, '0', }, 
    { { CENTER_TEXT_X("1", - 40), SCREEN_CENTER_Y + 38}, '1', }, 
    { { CENTER_TEXT_X("2", - 28), SCREEN_CENTER_Y + 38}, '2', }, 
    { { CENTER_TEXT_X("3", - 16), SCREEN_CENTER_Y + 38}, '3', }, 
    { { CENTER_TEXT_X("4", - 4),  SCREEN_CENTER_Y + 38}, '4', }, 
    { { CENTER_TEXT_X("5", + 16), SCREEN_CENTER_Y + 38}, '5', }, 
    { { CENTER_TEXT_X("6", + 28), SCREEN_CENTER_Y + 38}, '6', }, 
    { { CENTER_TEXT_X("7", + 40), SCREEN_CENTER_Y + 38}, '7', }, 
    { { CENTER_TEXT_X("8", + 52), SCREEN_CENTER_Y + 38}, '8', }, 
    { { CENTER_TEXT_X("9", + 64), SCREEN_CENTER_Y + 38}, '9', },
};

name_entry_glyph_t glyph_grid[60] = {
    // 10x6 grid
    // Every glyph has a padding of 12px around itself
    { { CENTER_TEXT_X("A", - 52), SCREEN_CENTER_Y - 8}, "A", }, 
    { { CENTER_TEXT_X("B", - 40), SCREEN_CENTER_Y - 8}, "B", },
    { { CENTER_TEXT_X("C", - 28), SCREEN_CENTER_Y - 8}, "C", }, 
    { { CENTER_TEXT_X("D", - 16), SCREEN_CENTER_Y - 8}, "D", }, 
    { { CENTER_TEXT_X("E", - 4),  SCREEN_CENTER_Y - 8}, "E", }, 
    { { CENTER_TEXT_X("F", + 16), SCREEN_CENTER_Y - 8}, "F", }, 
    { { CENTER_TEXT_X("G", + 28), SCREEN_CENTER_Y - 8}, "G", }, 
    { { CENTER_TEXT_X("H", + 40), SCREEN_CENTER_Y - 8}, "H", }, 
    { { CENTER_TEXT_X("I", + 52), SCREEN_CENTER_Y - 8}, "I", }, 
    { { CENTER_TEXT_X("J", + 64), SCREEN_CENTER_Y - 8}, "J", }, 

    { { CENTER_TEXT_X("K", - 52), SCREEN_CENTER_Y + 2}, "K", }, 
    { { CENTER_TEXT_X("L", - 40), SCREEN_CENTER_Y + 2}, "L", }, 
    { { CENTER_TEXT_X("M", - 28), SCREEN_CENTER_Y + 2}, "M", }, 
    { { CENTER_TEXT_X("N", - 16), SCREEN_CENTER_Y + 2}, "N", }, 
    { { CENTER_TEXT_X("O", - 4),  SCREEN_CENTER_Y + 2}, "O", }, 
    { { CENTER_TEXT_X("P", + 16), SCREEN_CENTER_Y + 2}, "P", }, 
    { { CENTER_TEXT_X("Q", + 28), SCREEN_CENTER_Y + 2}, "Q", }, 
    { { CENTER_TEXT_X("R", + 40), SCREEN_CENTER_Y + 2}, "R", }, 
    { { CENTER_TEXT_X("S", + 52), SCREEN_CENTER_Y + 2}, "S", }, 
    { { CENTER_TEXT_X("T", + 64), SCREEN_CENTER_Y + 2}, "T", },

    { { CENTER_TEXT_X("U", - 52), SCREEN_CENTER_Y + 14}, "U", }, 
    { { CENTER_TEXT_X("V", - 40), SCREEN_CENTER_Y + 14}, "V", }, 
    { { CENTER_TEXT_X("W", - 28), SCREEN_CENTER_Y + 14}, "W", }, 
    { { CENTER_TEXT_X("X", - 16), SCREEN_CENTER_Y + 14}, "X", }, 
    { { CENTER_TEXT_X("Y", - 4),  SCREEN_CENTER_Y + 14}, "Y", }, 
    { { CENTER_TEXT_X("Z", + 16), SCREEN_CENTER_Y + 14}, "Z", }, 
    { { CENTER_TEXT_X("", + 28),  SCREEN_CENTER_Y + 14}, "", }, // Unused 
    { { CENTER_TEXT_X("", + 40),  SCREEN_CENTER_Y + 14}, "", }, // Unused
    { { CENTER_TEXT_X("", + 52),  SCREEN_CENTER_Y + 14}, "", }, // Unused
    { { CENTER_TEXT_X("", + 64),  SCREEN_CENTER_Y + 14}, "", }, // Unused


    // Special characters 
    { { CENTER_TEXT_X("[", - 52), SCREEN_CENTER_Y + 26}, "[", }, 
    { { CENTER_TEXT_X("]", - 40), SCREEN_CENTER_Y + 26}, "]", }, 
    { { CENTER_TEXT_X(".", - 28), SCREEN_CENTER_Y + 26}, ".", }, 
    { { CENTER_TEXT_X(",", - 16), SCREEN_CENTER_Y + 26}, ",", }, 
    { { CENTER_TEXT_X("!", - 4),  SCREEN_CENTER_Y + 26}, "!", }, 
    { { CENTER_TEXT_X("?", + 16), SCREEN_CENTER_Y + 26}, "?", }, 
    { { CENTER_TEXT_X("-", + 28), SCREEN_CENTER_Y + 26}, "-", }, 
    { { CENTER_TEXT_X("_", + 40), SCREEN_CENTER_Y + 26}, "_", }, 
    { { CENTER_TEXT_X(":", + 52), SCREEN_CENTER_Y + 26}, ":", }, 
    { { CENTER_TEXT_X(";", + 60), SCREEN_CENTER_Y + 26}, ";", }, 
    
    // Nums
    { { CENTER_TEXT_X("0", - 52), SCREEN_CENTER_Y + 38}, "0", }, 
    { { CENTER_TEXT_X("1", - 40), SCREEN_CENTER_Y + 38}, "1", }, 
    { { CENTER_TEXT_X("2", - 28), SCREEN_CENTER_Y + 38}, "2", }, 
    { { CENTER_TEXT_X("3", - 16), SCREEN_CENTER_Y + 38}, "3", }, 
    { { CENTER_TEXT_X("4", - 4),  SCREEN_CENTER_Y + 38}, "4", }, 
    { { CENTER_TEXT_X("5", + 16), SCREEN_CENTER_Y + 38}, "5", }, 
    { { CENTER_TEXT_X("6", + 28), SCREEN_CENTER_Y + 38}, "6", }, 
    { { CENTER_TEXT_X("7", + 40), SCREEN_CENTER_Y + 38}, "7", }, 
    { { CENTER_TEXT_X("8", + 52), SCREEN_CENTER_Y + 38}, "8", }, 
    { { CENTER_TEXT_X("9", + 64), SCREEN_CENTER_Y + 38}, "9", },

    { { CENTER_TEXT_X("Confirm", 88), SCREEN_CENTER_Y + 96}, "", }, // Unused
    { { CENTER_TEXT_X("Confirm", 88), SCREEN_CENTER_Y + 96}, "", }, // Unused
    { { CENTER_TEXT_X("Confirm", 88), SCREEN_CENTER_Y + 96}, "", }, // Unused
    { { CENTER_TEXT_X("Confirm", 88), SCREEN_CENTER_Y + 96}, "", }, // Unused
    { { CENTER_TEXT_X("Confirm", 88), SCREEN_CENTER_Y + 96}, "", }, // Unused
    { { CENTER_TEXT_X("Confirm", 88), SCREEN_CENTER_Y + 96}, "", }, // Unused
    { { CENTER_TEXT_X("Confirm", 88), SCREEN_CENTER_Y + 96}, "", }, // Unused
    { { CENTER_TEXT_X("Confirm", 88), SCREEN_CENTER_Y + 96}, "", }, // Unused
    { { CENTER_TEXT_X("Confirm", 88), SCREEN_CENTER_Y + 96}, "", }, // Unused
    { { CENTER_TEXT_X("Confirm", 88), SCREEN_CENTER_Y + 96}, "Confirm", }
};

name_entry_bar_t name_entry_bar[NAME_ENTRY_LIMIT] = {
    { { CENTER_TEXT_X("_", - 32), SCREEN_CENTER_Y - 28 }, "", "_", }, 
    { { CENTER_TEXT_X("_", - 24), SCREEN_CENTER_Y - 28 }, "", "_", }, 
    { { CENTER_TEXT_X("_", - 15), SCREEN_CENTER_Y - 28 }, "", "_", }, 
    { { CENTER_TEXT_X("_", - 8),  SCREEN_CENTER_Y - 28 }, "", "_", }, 
    { { CENTER_TEXT_X("_", - 0),  SCREEN_CENTER_Y - 28 }, "", "_", }, 
    { { CENTER_TEXT_X("_", + 8),  SCREEN_CENTER_Y - 28 }, "", "_", }, 
    { { CENTER_TEXT_X("_", + 16), SCREEN_CENTER_Y - 28 }, "", "_", }, 
    { { CENTER_TEXT_X("_", + 24), SCREEN_CENTER_Y - 28 }, "", "_", }, 
    { { CENTER_TEXT_X("_", + 32), SCREEN_CENTER_Y - 28 }, "", "_", }, 
    { { CENTER_TEXT_X("_", + 40), SCREEN_CENTER_Y - 28 }, "", "_", }, 
};

void NameEntry_Init(name_entry_t *name_entry)
{
    name_entry->index       = 0;
    name_entry->name_pos    = 0;
    name_entry->name_limit  = NAME_ENTRY_LIMIT;
    
    name_entry->is_active      = false;
    name_entry->glyph_entered  = false;
    name_entry->glyph_deleted  = false;
    name_entry->name_confirmed  = false;

    name_entry->current_row = name_entry->index / NAME_ENTRY_GRID_COLS;
    name_entry->current_col = name_entry->index % NAME_ENTRY_GRID_ROWS;
}

void NameEntry_MoveUp(name_entry_t *name_entry)
{
    name_entry->current_row = (name_entry->current_row - 1 + NAME_ENTRY_GRID_ROWS) % NAME_ENTRY_GRID_ROWS;
    name_entry->index = name_entry->current_row * 
                        NAME_ENTRY_GRID_COLS + 
                        name_entry->current_col;
}

void NameEntry_MoveDown(name_entry_t *name_entry)
{
    name_entry->current_row = (name_entry->current_row + 1) % NAME_ENTRY_GRID_ROWS;
    name_entry->index = name_entry->current_row * 
                        NAME_ENTRY_GRID_COLS + 
                        name_entry->current_col;
}

void NameEntry_MoveLeft(name_entry_t *name_entry)
{
    name_entry->current_col = (name_entry->current_col - 1 + NAME_ENTRY_GRID_COLS) % NAME_ENTRY_GRID_COLS;
    name_entry->index = name_entry->current_row * 
                        NAME_ENTRY_GRID_COLS + 
                        name_entry->current_col;
}

void NameEntry_MoveRight(name_entry_t *name_entry)
{
    name_entry->current_col = (name_entry->current_col + 1) % NAME_ENTRY_GRID_COLS;
    name_entry->index = name_entry->current_row * NAME_ENTRY_GRID_COLS + name_entry->current_col;
}

void NameEntry_EnterGlyph(name_entry_t *name_entry, name_entry_bar_t *name_entry_bar, name_entry_ascii_t *ascii_to_glyph_grid)
{
    char temp;
    if (name_entry->glyph_entered)
    {
        if (name_entry->name_limit > 0 && name_entry->name_pos <= 10)
        {
            // If we're on the maid grid and not on the last row of the grid (buttons)
            if (name_entry->index >= 0 && name_entry->index < 50)
            {
                temp = ascii_to_glyph_grid[name_entry->index].ascii;
                PushCharStack(name_entry_bar[name_entry->name_pos].name, temp);
                
                --name_entry->name_limit;
                ++name_entry->name_pos;
            }

        }
    }
        
    name_entry->glyph_entered = false;
}

void NameEntry_DeleteGlyph(name_entry_t *name_entry, name_entry_bar_t *name_entry_bar)
{
    if (name_entry->glyph_deleted)
    {
        if (name_entry->name_limit < 10 && name_entry->name_pos >= 0)
        {
            ++name_entry->name_limit;
            --name_entry->name_pos;
            
            PopStack(name_entry_bar[name_entry->name_pos].name);
        }
    }

    name_entry->glyph_deleted = false;
}

void NameEntry_RenderNameUnderline(name_entry_bar_t *name_entry_bar, SDL_Renderer *Renderer, SDL_Texture *font_atlas, SDL_Color color)
{
    for (int i = 0; i < NAME_ENTRY_LIMIT; ++i)
    {
        RenderText(Renderer, font_atlas,
               name_entry_bar[i].pos.x,
               name_entry_bar[i].pos.y,
               name_entry_bar[i].underline,
               color);
    }
}

void NameEntry_RenderName(name_entry_bar_t *name_entry_bar, SDL_Renderer *Renderer, SDL_Texture *font_atlas, SDL_Color color)
{
    for (int i = 0; i < NAME_ENTRY_LIMIT; ++i)
    {
        RenderText(Renderer, font_atlas,
                   name_entry_bar[i].pos.x, 
                   name_entry_bar[i].pos.y - 4,
                   name_entry_bar[i].name, 
                   color);
    }
}

char *NameEntry_GetName(name_entry_t *name_entry)
{
    char *name = NULL;
    if (name_entry->name == NULL)
    {
        fprintf(stderr, "Name is NULL! -> %s\n", name_entry->name);
        return name;
    }

    name = name_entry->name;
    return name;
}

void NameEntry_ConfirmName(name_entry_t *name_entry, name_entry_bar_t *name_entry_bar)
{
    if (name_entry->name_confirmed)
    {
        for (int i = 0; i < NAME_ENTRY_LIMIT; ++i)
        {
            PushCharStack(name_entry->name, *name_entry_bar[i].name);
        }

        char *test_name = NameEntry_GetName(name_entry);
    
        printf("test_name: %s\n", test_name);


        printf("name_entry->name: %s\n", name_entry->name);
        printf("Confirmed!\n");
    }



    name_entry->name_confirmed = false;
}


