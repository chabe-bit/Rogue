#include "common.h"
#include "helper.h"
#include "global_states.h"
#include "colors.h"

#include "personality_test.h"
#include "personality_results.h"
#include "personality_scenario.h"

#include "class_base_stats.h"
#include "class_stat_growth.h"

#include "sound.h"
#include "name_entry.h"
#include "gui_font.h"

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
    i32 X, Y, W, H;
    i32 TargetWidth, TargetHeight;
    SDL_Texture *TargetTexture;
} camera_t;
camera_t SDLCamera;

typedef struct
{
    i32 hp, atk, def, exp;
    SDL_Rect health_bar;
} stats_t;

typedef struct
{
    SDL_Color color[MAX_COLOR];
    SDL_Texture *atlas;
} font_t;

typedef struct
{
    bool is_up;
    bool is_down;
    bool is_left;
    bool is_right;
} asset_front_face_t;
asset_front_face_t front_face_zero = {0};

typedef struct
{
    bool is_front;
    asset_front_face_t front;
    SDL_Rect range[4]; 
} asset_line_of_sight_t;

typedef struct
{
    bool is_front;
    asset_front_face_t front;
    SDL_Rect range; 
} test_asset_line_of_sight_t;

typedef struct
{
    SDL_Rect range; // As far as the asset's LOS, +1
} asset_chase_radius_t;

typedef struct
{
    bool up, down, left, right;
} asset_direction_t;

typedef struct
{
    bool has_movement;
    bool has_movement_priority;
    bool has_renderer;
    bool has_physics;
    bool has_collided;
    bool is_occupied;
} asset_conditions_t; // temp 

typedef struct
{
    i32 x, y;
    i32 w, h;
    
    asset_direction_t direction;
    asset_conditions_t conditions;
    asset_line_of_sight_t los; // line of sight
    test_asset_line_of_sight_t test_los[4];
    asset_chase_radius_t chase_radius;

    SDL_Rect body;
    SDL_Rect adjacent_hitboxes[4];
    SDL_Texture *texture;
} asset_t;

typedef struct
{
    i32 x, y, w, h;
    SDL_Rect body;
    SDL_Texture *texture;
} asset_cursor_t;

void CursorForItems(option_t *title_screen_options, asset_t *cursor, i32 x_offset, int y_offset)
{
    cursor->body.x = title_screen_options->x - cursor->w - x_offset;
    cursor->body.y = title_screen_options->y + (GLYPH_HEIGHT / 2) - (cursor->h / 2) - y_offset; // Subtracting by 1 at the end is the center the cursor
}

void Cursor(asset_t *cursor, vec2_t *pos, i32 x_offset, int y_offset)
{
    cursor->body.x = pos->x - (cursor->w) + x_offset;
    cursor->body.y = pos->y + (GLYPH_HEIGHT / 2) - (cursor->h / 2) + y_offset; 
}

void CursorForAssets(asset_t *asset, asset_t *cursor, i32 x_offset, int y_offset)
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
    i32 channels;
    unsigned char *data = stbi_load(filename, &asset->w, &asset->h, &channels, STBI_default);
    if (data == NULL)
    {
        fprintf(stderr, "Failed to load files: %s\n", filename);
        return;
    }
    
    i32 fmt = channels == 2 ? SDL_PIXELFORMAT_RGBA8888 : SDL_PIXELFORMAT_RGBA32;
    i32 pitch = asset->w * channels;

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

    i32 channels;
    unsigned char *data = stbi_load(filename, &asset.w, &asset.h, &channels, STBI_default);
    if (data == NULL)
    {
        fprintf(stderr, "Failed to load files: %s\n", filename);
        return asset;
    }
    
    i32 fmt = channels == 2 ? SDL_PIXELFORMAT_RGBA8888 : SDL_PIXELFORMAT_RGBA32;
    i32 pitch = asset.w * channels;

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

    // Prefer to initialize seperately, but here for noe

    stbi_image_free(data);  

    return asset;
}

bool AABB_Detection(SDL_Rect *A, SDL_Rect *B)
{
    if(A->y + A->h <= B->y) 
        return false;

    if(A->y >= B->y + B->h) 
        return false;

    if(A->x + A->w <= B->x) 
        return false;

    if(A->x >= B->x + B->w) 
        return false;

    return true;
}

void AABB_Resolution(asset_t *A, asset_t *B)
{
    if (AABB_Detection(&A->body, &B->body) && 
        (A->conditions.has_physics && B->conditions.has_physics))
    {
        A->body.x = A->x;
        A->body.y = A->y;

        if (B->conditions.has_movement)
            B->conditions.has_movement = false;

    }
    else
    {
        B->conditions.has_movement = true;
    }
}

void AABB_AdjHitboxResolution(asset_t *A, asset_t *B)
{
    // Ideal function for combat; for example, if an enemy intiates combat
    // with the player via melee, the AI of the enemy is to stay and attack, 
    // where the player has the choice to fight or flee, and is not 'locked'.
    // Enemy is released through flee AI.
    
    // If Asset B enters the adjacent hitbox of Asset A, Asset B caanot 
    // move until Asset A releases it by moving or is released from some other
    // condition. 
    if ((AABB_Detection(&A->adjacent_hitboxes[0], &B->body)) ||
         (AABB_Detection(&A->adjacent_hitboxes[1], &B->body)) ||
         (AABB_Detection(&A->adjacent_hitboxes[2], &B->body)) ||
         (AABB_Detection(&A->adjacent_hitboxes[3], &B->body)) ||
         (AABB_Detection(&A->body, &B->body)))
    {
        // Push back collision 
        B->body.x = B->x;
        B->body.y = B->y;

        // Asset A's collision with Asset B is handled here
        if (AABB_Detection(&A->body, &B->body))
        {
            A->body.x = A->x;
            A->body.y = A->y;
        }

        // Toggles Asset B's movement off
        if (B->conditions.has_movement)
        {
            B->conditions.has_movement = false;
        }

    }
    else
    {
        // Releases Asset B if out of Asset A's adjacent hitbox
        B->conditions.has_movement = true;
    }
}

void AABB_LOSResolution(asset_t *A, asset_t *B, font_t *font)
{
    if ((AABB_Detection(&A->los.range[0], &B->body)) ||
     (AABB_Detection(&A->los.range[1], &B->body)) ||
     (AABB_Detection(&A->los.range[2], &B->body)) ||
     (AABB_Detection(&A->los.range[3], &B->body)) ||
     (AABB_Detection(&A->body, &B->body)))
    {
        printf("in los range!\n");
        
        // Initiate combat mode!
        RenderText(SDLWindow.Renderer, font->atlas,
                   SET_TEXT_CENTER_X("COMBAT MODE", 0),
                   SCREEN_CENTER_Y,
                   "COMBAT MODE",
                   font->color[COLOR_WHITE]); 
    }
}

void AABB_ChaseRadiusResolution(asset_t *A, asset_t *B, font_t *font)
{
    if (AABB_Detection(&A->chase_radius.range, &B->body))
    {
       // printf("in chase range!\n");
        
        
    }
}

bool AABB_DetectionV2(asset_t *A, asset_t *B)
{
    if(A->body.y + A->body.h <= B->body.y) 
        return false;

    if(A->body.y >= B->body.y + B->body.h) 
        return false;

    if(A->body.x + A->body.w <= B->body.x) 
        return false;

    if(A->body.x >= B->body.x + B->body.w) 
        return false;

    return true;
}

void AABB_ResolutionV2(asset_t *A, asset_t *B)
{
    if (AABB_DetectionV2(A, B) &&
        (A->conditions.has_physics && B->conditions.has_physics))
    {
        A->body.x = A->x;
        A->body.y = A->y;

        B->body.x = B->x;
        B->body.y = B->y;

        A->conditions.has_collided = true;
        B->conditions.has_collided = true;
    }
}

typedef enum
{
    ASSET_TOP,
    ASSET_BOT,
    ASSET_LEFT,
    ASSET_RIGHT
} asset_index_type;

void UpdateAssetLOS(asset_t *asset)
{
    asset->los.range[ASSET_TOP].x = asset->body.x; 
    asset->los.range[ASSET_TOP].y = (asset->body.y - (24*3));
    asset->los.range[ASSET_TOP].w = 16;
    asset->los.range[ASSET_TOP].h = (24*3);

    asset->los.range[ASSET_BOT].x = asset->body.x; 
    asset->los.range[ASSET_BOT].y = (asset->body.y + 24);
    asset->los.range[ASSET_BOT].w = 16;
    asset->los.range[ASSET_BOT].h = (24*3);

    asset->los.range[ASSET_LEFT].x = (asset->body.x - (16*3)); 
    asset->los.range[ASSET_LEFT].y = asset->body.y;
    asset->los.range[ASSET_LEFT].w = (16*3);
    asset->los.range[ASSET_LEFT].h = 24;

    asset->los.range[ASSET_RIGHT].x = (asset->body.x + 16); 
    asset->los.range[ASSET_RIGHT].y = asset->body.y;
    asset->los.range[ASSET_RIGHT].w = (16*3);
    asset->los.range[ASSET_RIGHT].h = 24;
}

void UpdateAssetAdjHitboxes(asset_t *asset)
{
    asset->adjacent_hitboxes[ASSET_TOP].x = asset->body.x; 
    asset->adjacent_hitboxes[ASSET_TOP].y = (asset->body.y - 24);
    asset->adjacent_hitboxes[ASSET_TOP].w = 16;
    asset->adjacent_hitboxes[ASSET_TOP].h = 24;

    asset->adjacent_hitboxes[ASSET_BOT].x = asset->body.x; 
    asset->adjacent_hitboxes[ASSET_BOT].y = (asset->body.y + 24);
    asset->adjacent_hitboxes[ASSET_BOT].w = 16;
    asset->adjacent_hitboxes[ASSET_BOT].h = 24;

    asset->adjacent_hitboxes[ASSET_LEFT].x = (asset->body.x- 16); 
    asset->adjacent_hitboxes[ASSET_LEFT].y = asset->body.y;
    asset->adjacent_hitboxes[ASSET_LEFT].w = 16;
    asset->adjacent_hitboxes[ASSET_LEFT].h = 24;

    asset->adjacent_hitboxes[ASSET_RIGHT].x = (asset->body.x + 16); 
    asset->adjacent_hitboxes[ASSET_RIGHT].y = asset->body.y;
    asset->adjacent_hitboxes[ASSET_RIGHT].w = 16;
    asset->adjacent_hitboxes[ASSET_RIGHT].h = 24; 
}

void UpdateAssetChaseRadius(asset_t *asset)
{
    // Asset chase radius -> 9x9
    asset->chase_radius.range.x = (asset->body.x) - (asset->w * 4); 
    asset->chase_radius.range.y = (asset->body.y) - (asset->h * 4);
    asset->chase_radius.range.w = asset->w * 9;
    asset->chase_radius.range.h = asset->h * 9;
}

void UpdateAssetProperties(asset_t *asset)
{
    UpdateAssetLOS(asset);
    UpdateAssetAdjHitboxes(asset);
    UpdateAssetChaseRadius(asset);
}

bool UpdateAssetMovement(asset_t *asset, sound_wav_t *sound)
{    
    // Not needed but will leave this here since we're doing 
    // this in RenderAssetInWorldSpace. 
    //asset->x = asset->body.x; 
    //asset->y = asset->body.y; 

    if (asset->conditions.has_movement)
    {
        if (asset->direction.up)
        {       
            asset->body.y -= asset->body.h;

            Sound_PlaySFX(sound);
            asset->direction.up = false;

            return true;
        }

        if (asset->direction.down)
        {
            asset->body.y += asset->body.h; 
           
            Sound_PlaySFX(sound);
            asset->direction.down = false;

            return true;
        }

        if (asset->direction.left)
        {
            asset->body.x -= asset->body.w; 
          
            Sound_PlaySFX(sound);
            asset->direction.left = false;

            return true;
        }

        if (asset->direction.right)
        {
            asset->body.x += asset->body.w; 
           
            Sound_PlaySFX(sound);
            asset->direction.right = false;

            return true;
        }
    }

    return false;
}

typedef enum
{
    ASSET_NPC_UP,
    ASSET_NPC_DOWN,
    ASSET_NPC_LEFT,
    ASSET_NPC_RIGHT,
} asset_npc_movement_type;

void UpdateAssetNPCMovement(asset_t *asset, sound_wav_t *sound)
{
    asset->x = asset->body.x; 
    asset->y = asset->body.y;

    i32 direction = 2; //rand() % 4;
 
    if (asset->conditions.has_movement)
    {
        switch (direction)
        {
            case ASSET_NPC_UP:
            {
                asset->body.y -= asset->body.h;
            } break;
            case ASSET_NPC_DOWN:
            {
                asset->body.y += asset->body.h; 
            } break;
            case ASSET_NPC_LEFT:
            {
                asset->body.x -= asset->body.w;
            } break;
            case ASSET_NPC_RIGHT:
            {
                asset->body.x += asset->body.w; 
            } break;
        }

    }

}

void UpdatePushableAsset(asset_t *asset, asset_t *player)
{
    asset->x = asset->body.x; 
    asset->y = asset->body.y; 

    if (asset->conditions.has_movement)
    {
        if (asset->direction.up)
        {      
            if (AABB_Detection(&asset->body, &player->body) && 
                asset->conditions.has_physics)
            {
                asset->body.y -= asset->body.h;
            }

            asset->direction.up = false;
        }

        if (asset->direction.down)
        {
            if (AABB_Detection(&asset->body, &player->body) && 
                asset->conditions.has_physics)
            {
                asset->body.y += asset->body.h; 
            }
                
            asset->direction.down = false;
        }

        if (asset->direction.left)
        {
            if (AABB_Detection(&asset->body, &player->body) && 
                asset->conditions.has_physics)
            {
                asset->body.x -= asset->body.w; 
            }
           
            asset->direction.left = false;
        }

        if (asset->direction.right)
        {
            if (AABB_Detection(&asset->body, &player->body) && 
                asset->conditions.has_physics)
            {
                asset->body.x += asset->body.w; 
            }
           
            asset->direction.right = false;
        }
    }
}

void InitializeAssetToRender(asset_t *asset, i32 x, int y, int w, int h)
{
    asset->body.x = x;
    asset->body.y = y;
    asset->body.w = w;
    asset->body.h = h;
}

void InitializeAssetConditions(asset_t *asset)
{
    asset->conditions.has_physics = true;
    asset->conditions.has_movement = true;
}

void RenderAssetLOS(asset_t *asset)
{
    // THIS is to just so we can render the boxes, debug code.
    asset->los.range[ASSET_TOP].x = asset->body.x - SDLCamera.X; 
    asset->los.range[ASSET_TOP].y = (asset->body.y - (24*3)) - SDLCamera.Y;

    asset->los.range[ASSET_BOT].x = asset->body.x - SDLCamera.X; 
    asset->los.range[ASSET_BOT].y = (asset->body.y + 24) - SDLCamera.Y;

    asset->los.range[ASSET_LEFT].x = (asset->body.x - (16*3)) - SDLCamera.X; 
    asset->los.range[ASSET_LEFT].y = asset->body.y - SDLCamera.Y;

    asset->los.range[ASSET_RIGHT].x = (asset->body.x + 16) - SDLCamera.X; 
    asset->los.range[ASSET_RIGHT].y = asset->body.y - SDLCamera.Y;

    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 255, 0, 255);
    for (int i = 0; i < 4; ++i)
    {
        SDL_RenderDrawRect(SDLWindow.Renderer, &asset->los.range[i]);
    }
}

void RenderAssetAdjHitboxes(asset_t *asset)
{
    asset->adjacent_hitboxes[ASSET_TOP].x = asset->body.x - SDLCamera.X; 
    asset->adjacent_hitboxes[ASSET_TOP].y = (asset->body.y - 24) - SDLCamera.Y;
    asset->adjacent_hitboxes[ASSET_TOP].w = 16;
    asset->adjacent_hitboxes[ASSET_TOP].h = 24;

    asset->adjacent_hitboxes[ASSET_BOT].x = asset->body.x - SDLCamera.X; 
    asset->adjacent_hitboxes[ASSET_BOT].y = (asset->body.y + 24) - SDLCamera.Y;
    asset->adjacent_hitboxes[ASSET_BOT].w = 16;
    asset->adjacent_hitboxes[ASSET_BOT].h = 24;

    asset->adjacent_hitboxes[ASSET_LEFT].x = (asset->body.x - 16) - SDLCamera.X; 
    asset->adjacent_hitboxes[ASSET_LEFT].y = asset->body.y - SDLCamera.Y;
    asset->adjacent_hitboxes[ASSET_LEFT].w = 16;
    asset->adjacent_hitboxes[ASSET_LEFT].h = 24;

    asset->adjacent_hitboxes[ASSET_RIGHT].x = (asset->body.x + 16) - SDLCamera.X; 
    asset->adjacent_hitboxes[ASSET_RIGHT].y = asset->body.y - SDLCamera.Y;
    asset->adjacent_hitboxes[ASSET_RIGHT].w = 16;
    asset->adjacent_hitboxes[ASSET_RIGHT].h = 24;

    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 255, 0, 255);
    for (int i = 0; i < 4; ++i)
    {
        SDL_RenderDrawRect(SDLWindow.Renderer, &asset->adjacent_hitboxes[i]);
    }
}

void RenderAssetChaseRadius(asset_t *asset)
{
    asset->chase_radius.range.x = (asset->body.x - asset->w * 4) - SDLCamera.X; 
    asset->chase_radius.range.y = (asset->body.y - asset->h * 4) - SDLCamera.Y;
    asset->chase_radius.range.w = asset->w * 9;
    asset->chase_radius.range.h = asset->h * 9;

    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(SDLWindow.Renderer, &asset->chase_radius.range);
}

// Render and update any asset that moves in world space
void RenderAssetInWorldSpace(asset_t *asset)
{
    // Temp store asset body's current position 
    // NOTE: This is to handle our collision detection to push the asset
    // back into its position before the collision.
    asset->x = asset->body.x;
    asset->y = asset->body.y;

    // Update asset body's position to be placed in world space by removing the camera coords
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

    // Return the asset's current position into camera space
    asset->body.x = asset->x;
    asset->body.y = asset->y;
    
   // RenderAssetLOS(asset);
   // RenderAssetAdjHitboxes(asset);
   // RenderAssetChaseRadius(asset);
}

void Asset_SetPosition(asset_t *asset, vec2_t *coords)
{
    asset->body.x = coords->x;
    asset->body.y = coords->y;
}

// temp
void RenderAssetInWorldSpaceWithCoords(asset_t *asset, int x, int y)
{
    asset->x = x;
    asset->y = y;
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

    asset->body.x = x;
    asset->body.y = y;
}

// Render any asset anywhere in world space
void RenderAsset(asset_t *asset, i32 x, int y, int w, int h)
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
void RenderAssetInCameraSpace(asset_t *asset, i32 x, int y)
{
    asset->body.x = x;
    asset->body.y = y;

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

void RenderAssetInCameraSpaceDIMENSION(asset_t asset, i32 x, int y, int w, int h)
{
    asset.body.x = x;
    asset.body.y = y;
    asset.body.w = w;
    asset.body.h = h;

    if (asset.texture)
    {
        SDL_SetTextureBlendMode(asset.texture, SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(SDLWindow.Renderer, asset.texture, NULL, &asset.body) < 0)
        {
            fprintf(stderr, "Failed to render asset: %s\n", SDL_GetError());
            return;
        }
    }
}

void SetAssetAdjacentHitBoxes(asset_t *asset)
{
    asset->adjacent_hitboxes[ASSET_TOP].x = asset->body.x; 
    asset->adjacent_hitboxes[ASSET_TOP].y = (asset->body.y - 24);
    asset->adjacent_hitboxes[ASSET_TOP].w = 16;
    asset->adjacent_hitboxes[ASSET_TOP].h = 24;

    asset->adjacent_hitboxes[ASSET_BOT].x = asset->body.x; 
    asset->adjacent_hitboxes[ASSET_BOT].y = (asset->body.y + 24);
    asset->adjacent_hitboxes[ASSET_BOT].w = 16;
    asset->adjacent_hitboxes[ASSET_BOT].h = 24;

    asset->adjacent_hitboxes[ASSET_LEFT].x = (asset->body.x - 16); 
    asset->adjacent_hitboxes[ASSET_LEFT].y = asset->body.y;
    asset->adjacent_hitboxes[ASSET_LEFT].w = 16;
    asset->adjacent_hitboxes[ASSET_LEFT].h = 24;

    asset->adjacent_hitboxes[ASSET_RIGHT].x = (asset->body.x + 16); 
    asset->adjacent_hitboxes[ASSET_RIGHT].y = asset->body.y;
    asset->adjacent_hitboxes[ASSET_RIGHT].w = 16;
    asset->adjacent_hitboxes[ASSET_RIGHT].h = 24;
}

void SetAssetLOS(asset_t *asset)
{
    asset->los.range[ASSET_TOP].x = asset->body.x; 
    asset->los.range[ASSET_TOP].y = (asset->body.y - (24*3));
    asset->los.range[ASSET_TOP].w = 16;
    asset->los.range[ASSET_TOP].h = (24*3);

    asset->los.range[ASSET_BOT].x = asset->body.x; 
    asset->los.range[ASSET_BOT].y = (asset->body.y + 24);
    asset->los.range[ASSET_BOT].w = 16;
    asset->los.range[ASSET_BOT].h = (24*3);

    asset->los.range[ASSET_LEFT].x = (asset->body.x - (16*3)); 
    asset->los.range[ASSET_LEFT].y = asset->body.y;
    asset->los.range[ASSET_LEFT].w = (16*3);
    asset->los.range[ASSET_LEFT].h = 24;

    asset->los.range[ASSET_RIGHT].x = (asset->body.x + 16); 
    asset->los.range[ASSET_RIGHT].y = asset->body.y;
    asset->los.range[ASSET_RIGHT].w = (16*3);
    asset->los.range[ASSET_RIGHT].h = 24;
}

void SetAssetChaseRadius(asset_t *asset)
{
    // Asset chase radius -> 9x9
    asset->chase_radius.range.x = (asset->body.x) - (asset->w * 4); 
    asset->chase_radius.range.y = (asset->body.y) - (asset->h * 4);
    asset->chase_radius.range.w = asset->w * 9;
    asset->chase_radius.range.h = asset->h * 9;
}

typedef enum
{
    ASSET_CONDITION_PHYSICS,
    ASSET_CONDITION_RENDERER,
    ASSET_CONDITION_MOVEMENT
} asset_condition_type;

void SetAssetConditionAllOn(asset_t *asset)
{
    asset->conditions.has_physics = true;
    asset->conditions.has_renderer = true;
    asset->conditions.has_movement = true;

}

void SetAssetConditionAllOff(asset_t *asset)
{
    asset->conditions.has_physics = false;
    asset->conditions.has_renderer = false;
    asset->conditions.has_movement = false;
}

void SetAssetConditionOn(asset_t *asset, i32 index)
{
    switch (index)
    {
        case ASSET_CONDITION_PHYSICS: 
        {
            asset->conditions.has_physics = true;
        } break;
        case ASSET_CONDITION_RENDERER: 
        {
            asset->conditions.has_renderer = true;
        } break;
        case ASSET_CONDITION_MOVEMENT: 
        {
            asset->conditions.has_movement = true;
        } break;
    }
}

void SetAssetConditionOff(asset_t *asset, i32 index)
{
    switch (index)
    {
        case ASSET_CONDITION_PHYSICS: 
        {
            asset->conditions.has_physics = false;
        } break;
        case ASSET_CONDITION_RENDERER: 
        {
            asset->conditions.has_renderer = false;
        } break;
        case ASSET_CONDITION_MOVEMENT: 
        {
            asset->conditions.has_movement = false;
        } break;
    }
}

vec2_t Vec2_SetPosition(i32 x, i32 y)
{
    vec2_t pos = {0};

    pos.x = x;
    pos.y = y;
    
    return pos;
}

void SetAssetPosition(asset_t *asset, i32 x, int y)
{
    asset->body.x = x;
    asset->body.y = y;
}

void InitializeAsset(asset_t *asset, vec2_t *pos)
{
    SetAssetPosition(asset, pos->x, pos->y);
    SetAssetConditionAllOn(asset);

    SetAssetLOS(asset);
    SetAssetAdjacentHitBoxes(asset);
    SetAssetChaseRadius(asset);
}


#define MAX_CARD_STACK 4
typedef struct
{
    i32 index;
    asset_t asset[MAX_CARD_STACK];  
} asset_card_stack_t;

asset_t Asset_Zero()
{
    asset_t zero = {0};
    return zero;
}

void AssetCard_Push(asset_card_stack_t *stack, asset_t card)
{
    if (stack->index < MAX_CARD_STACK)
    {
        stack->asset[stack->index] = card; 

        // Update the asset's position based on index
        switch (stack->index)
        {
            // Hardcoding to handle only enemy cards for noe
            case 0:
            {
                stack->asset[0].x = SCREEN_CENTER_X + (120 - card.w);
                stack->asset[0].y = SCREEN_CENTER_Y - 112;
            } break;
            case 1:
            {
                stack->asset[1].x = SCREEN_CENTER_X + (120 - card.w);
                stack->asset[1].y = (SCREEN_CENTER_Y - 112 + card.h * 1) + 8;
            } break;
            case 2:
            {
                stack->asset[2].x = SCREEN_CENTER_X + (120 - card.w);
                stack->asset[2].y = (SCREEN_CENTER_Y - 112 + card.h * 2) + 16;
            } break;
            case 3:
            {
                stack->asset[3].x = SCREEN_CENTER_X + (120 - card.w);
                stack->asset[3].y = (SCREEN_CENTER_Y - 112 + card.h * 3) + 24;
            } break;
        }

        printf("card stack x: %d\n", stack->asset[stack->index].x); 
        stack->index++;
    }
}

void AssetCard_Pop(asset_card_stack_t *stack)
{
    if (stack->index > 0)
        stack->index--;
}

void AssetCard_Render(asset_card_stack_t *stack)
{
    // Render only to the total number of assets counted from stack->index
    for (int i = 0; i < stack->index; ++i)
    {
        RenderAssetInCameraSpace(&stack->asset[i],
                                 stack->asset[i].x,
                                 stack->asset[i].y);
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

void Font_Init(font_t *font)
{
    font->atlas = CreateFontAtlas(SDLWindow.Renderer);
    if (!font->atlas)
    {
        SDL_DestroyRenderer(SDLWindow.Renderer);
        SDL_DestroyWindow(SDLWindow.Window);
        SDL_Quit();
        return;
    }

    font->color[0] = color.red;
    font->color[1] = color.green;
    font->color[2] = color.blue;
    font->color[3] = color.white;
    font->color[4] = color.black;
    font->color[5] = color.orange;
}

typedef struct
{
    vec2_t pos;
    const char *text;
} button_t;

typedef struct
{
    i32 index;
    bool is_active;
    SDL_Rect box;
    SDL_Rect box_border;

    font_t *font;
    button_t button[2];
} confirm_buttons_t;

void GUI_UpdateCursor(asset_t *cursor, i32 x, i32 y, i32 x_offset, int y_offset)
{
    cursor->body.x = x - cursor->w + x_offset;
    cursor->body.y = y + (GLYPH_HEIGHT / 2) - (cursor->h / 2) + y_offset; 
}

void GUI_RenderCursor(asset_t *cursor)
{
    if (cursor->texture)
    {
        SDL_SetTextureBlendMode(cursor->texture, SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(SDLWindow.Renderer, cursor->texture, NULL, &cursor->body) < 0)
        {
            fprintf(stderr, "Failed to render asset: %s\n", SDL_GetError());
            return;
        }
    }   
}

void ConfirmationButtons_Init(confirm_buttons_t *confirm, font_t *font)
{
    confirm->font = font;

    confirm->button[0].text = "Yes";
    confirm->button[0].pos.x = SET_TEXT_CENTER_X("Yes", 0);
    confirm->button[0].pos.y = SCREEN_CENTER_Y;

    confirm->button[1].text = "No";
    confirm->button[1].pos.x = SET_TEXT_CENTER_X("No", 0);
    confirm->button[1].pos.y = SCREEN_CENTER_Y + 16;

    confirm->box.x = SCREEN_CENTER_X - (48 / 2);
    confirm->box.y = SCREEN_CENTER_Y - 20;
    confirm->box.w = 48;
    confirm->box.h = 56;

    confirm->box_border.x = SCREEN_CENTER_X - (50 / 2);
    confirm->box_border.y = SCREEN_CENTER_Y - 20;
    confirm->box_border.w = 50;
    confirm->box_border.h = 58;
}

void ConfirmationButtons_RenderBox(confirm_buttons_t *confirm)
{
    // Box border
    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(SDLWindow.Renderer, &confirm->box_border);
    
    // Border
    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(SDLWindow.Renderer, &confirm->box_border);
}

void ConfirmationButtons_RenderText(confirm_buttons_t *confirm)
{
    for (i32 i = 0; i < ArraySize(confirm->button); ++i) 
    {
        RenderText(SDLWindow.Renderer, confirm->font->atlas,
                   confirm->button[i].pos.x, 
                   confirm->button[i].pos.y,
                   confirm->button[i].text, 
                   confirm->font->color[COLOR_WHITE]);
    }
}

// ----------------------- EXPERIEMENTAL --------------------------
// Personality Test inspired by DQ3 Remake
// https://game8.co/games/Dragon-Quest-3/archives/464271
void Personality_InitQuestions(personality_test_t *personality_test)
{
    personality_test->index = 1; // Starting question begins at 1
    personality_test->is_active = false;

    for (i32 i = 0; i < PERSONALITY_TEST_QUESTIONS; ++i)
    {
        personality_test->table[i] = personality_questions[i]; 
    }
}

void Personality_BeginQuestions(personality_test_t *personality_test)
{
    // This function randomizes between the first five questions on starting the 
    // quiz. From there the quiz will follow based on the player's input. 

    static i32 rand_index = 0;

    // Only the starting 5 questions
    static bool first_question_selected = false;
    if (!first_question_selected)
    {
        i32 total = ArraySize(personality_test->table);
        rand_index = 1 + rand() % 5;
        personality_test->index = rand_index;

        printf("rand_index: %d\n", rand_index);
        first_question_selected = true;
    }
}



i32 Personality_GetScenarioOptionCount(personality_scenario_t *scenario)
{
    i32 result = 0;
  
    if (scenario->result & SCENARIO_VILLAGE)
        result = VILLAGE_OPTION_COUNT;

    if (scenario->result & SCENARIO_MONSTER)
        result = MONSTER_OPTION_COUNT;
   
    if (scenario->result & SCENARIO_FOREST)
        result = FOREST_INDEX;

    if (scenario->result & SCENARIO_CAVE)
        result = CAVE_OPTION_COUNT;
   
    if (scenario->result & SCENARIO_DESERT)
        result = DESERT_OPTION_COUNT;

    if (scenario->result & SCENARIO_TOWER)
        result = TOWER_OPTION_COUNT;
   
    if (scenario->result & SCENARIO_THEATER)
        result = THEATER_OPTION_COUNT;

    return result;
}


i32 Personality_ActuallyGettingTheOptionCount(i32 index)
{
    i32 result = 0;

    switch (index)
    {
        case VILLAGE_INDEX:
        {
            result = VILLAGE_OPTION_COUNT; 
        } break;
        case MONSTER_INDEX:
        {
            result = MONSTER_OPTION_COUNT; 
        } break;
        case CAVE_INDEX:
        {
            result = CAVE_OPTION_COUNT; 
        } break;
        case DESERT_INDEX:
        {
            result = DESERT_OPTION_COUNT; 
        } break;
        case TOWER_INDEX:
        {
            result = TOWER_OPTION_COUNT; 
        } break;
        case THEATER_INDEX:
        {
            result = THEATER_OPTION_COUNT; 
        } break;
    }

    return result;
}

void Personality_InitScenarioOptions(personality_scenario_t *personality, option_t options[], i32 index)
{
    i32 option_count = Personality_ActuallyGettingTheOptionCount(index);

    for (int i = 0; i < option_count; ++i)
        personality->scenario[index].options[i] = options[i];

    printf("option: %s\n", personality->scenario[index].options[0].text);
}

i32 Personality_GetScenario(personality_scenario_t *scenario)
{
    i32 result = 0;
  
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

void Personality_RenderScenarioResults(personality_results_t *personality, font_t *font)
{
    RenderText(SDLWindow.Renderer, font->atlas,
               SET_TEXT_CENTER_X(personality->name, 0),
               SCREEN_CENTER_Y - 80,
               personality->name,
               font->color[COLOR_WHITE]);

    for (i32 i = 0; i < PERSONALITY_DESCRIPTION_SIZE; ++i)
    {
        RenderText(SDLWindow.Renderer, font->atlas,
                   SET_TEXT_CENTER_X(personality->description[i], 0),
                   SCREEN_CENTER_Y + personality->x_coords[i],
                   personality->description[i],
                   font->color[COLOR_WHITE]);
    }
}

personality_results_t Personality_GetScenarioResults(personality_scenario_t *scenario, personality_types state)
{
    static personality_results_t results = {0};

    if (scenario->load_results)
    {
        switch (state)
        {
            // VILLAGE
            case PERSONALITY_SHOW_OFF:
            {
                results = show_off;
            } break;
            case PERSONALITY_SLIPPERY_DEVIL:
            {
                results = slippery_devil;
            } break;
            case PERSONALITY_SHRINKING_VIOLET:
            {
                results = shrinking_violet;
            } break;
    
            // MONSTER
            case PERSONALITY_PARAGON:
            {
                results = paragon;
            } break;
            case PERSONALITY_WIMP:
            {
                results = wimp;
            } break;
            case PERSONALITY_SPOILT_BRAT:
            {
                results = spoilt_brat;
            } break;
            case PERSONALITY_EGGHEAD:
            {
                results = egghead;
            } break;
            case PERSONALITY_KLUTZ:
            {
                results = klutz;
            } break;

            // DESERT 
            case PERSONALITY_DAREDEVIL:
            {
                results = daredevil;
            } break;
            case PERSONALITY_IDEALIST:
            {
                results = idealist;
            } break; 
            case PERSONALITY_THUG:
            {
                results = thug;
            } break;

            // CAVE
            case PERSONALITY_STRAIGHT_ARROW:
            {
                results = straight_arrow;
            } break;
            case PERSONALITY_MULE:
            {
                results = mule;
            } break;
            case PERSONALITY_NARCISSIST:
            {
                results = narcissist;
            } break;
            case PERSONALITY_SORE_LOSER:
            {
                results = sore_loser;
            } break;

            // FOREST 
            case PERSONALITY_LAZYBONES:
            {
                results = lazybones;
                scenario->load_results = false;
            } break;
            case PERSONALITY_PLUGGER:
            {
                results = plugger;
            } break;
            case PERSONALITY_DRUDGE:
            {
                results = drudge;
            } break;
            case PERSONALITY_TOUGH_COOKIE:
            {
                results = tough_cookie;
            } break;

            // TOWER
            case PERSONALITY_DAYDREAMER:
            {
                results = daydreamer;
            } break;
            case PERSONALITY_SOCIALITE:
            {
                results = socialite;
            } break;

            // THEATER
            case PERSONALITY_FREE_SPIRIT:
            {
                results = free_spirit;
            } break;
            case PERSONALITY_CRYBABY:
            {
                results = crybaby;
            } break;
            case PERSONALITY_LONE_WOLF:
            {
                results = lone_wolf;
            } break;
            case PERSONALITY_LOUT:
            {
                results = lout;
            } break;

            default:
            {
                state = PERSONALITY_UNUSED;
            } break;
        
        
        }
            
        scenario->load_results = false;
        
    }

    return results;
}

void Personality_MoveUpScenario(personality_scenario_t *personality)
{
    i32 SCENARIO = Personality_GetScenario(personality);
    i32 OPTIONS = Personality_GetScenarioOptionCount(personality);

    if (personality->scenario[SCENARIO].is_active)
    {
        personality->scenario[SCENARIO].index--;
        if (personality->scenario[SCENARIO].index < 0)
            personality->scenario[SCENARIO].index = OPTIONS - 1;
    }
}

void Personality_MoveDownScenario(personality_scenario_t *personality)
{
    i32 SCENARIO = Personality_GetScenario(personality);
    i32 OPTIONS = Personality_GetScenarioOptionCount(personality);

    if (personality->scenario[SCENARIO].is_active)
    {
        personality->scenario[SCENARIO].index++;
        if (personality->scenario[SCENARIO].index >= OPTIONS)
            personality->scenario[SCENARIO].index = 0;
    }
}

void Personality_GetStatAttrColors(personality_results_t *personality)
{
     
}

typedef struct
{
    vec2_t pos;
    char text[32];
} class_status_overview_t;

void StatOverview_Init(char *buffer, char *dst, char *src, size_t total_width)
{
    size_t append_len = strlen(src);
    size_t insert_index = total_width - append_len;

    strncpy(buffer, dst, total_width);
    buffer[total_width] = '\0';

    strncpy(buffer + insert_index, src, append_len);
    buffer[total_width] = '\0';

    strncpy(dst, buffer, total_width + 1);
}

typedef struct
{
    char *name;
    asset_t slot;
    asset_t type[5];
} game_rarity_roller_t;

game_rarity_roller_t Game_RarityRoller(asset_t rarity_types[])
{
    // The color of the slots will change upon the rarity of the
    // item; weapons, armor and items such as potions.
    // Default color is common -> color.white (maybe)

    u32 rarity_roller = rand() % 99;
    game_rarity_roller_t rarity_agent = {0}; // Agent because why not

    // Out of 100 values, there are 5 different regions, each of which will have own threshold/percentage-rate
    i32 rarity_common_threshold     = 60; // 60% 
    i32 rarity_rare_threshold       = 85; // 25% 
    i32 rarity_epic_threshold       = 93; // 8%
    i32 rarity_legenary_threshold   = 98; // 5%
    i32 rarity_mythical_threshold   = 99; // 1% 
    
    if (rarity_roller < rarity_common_threshold)
    {
        rarity_agent.name = "Common";
        rarity_agent.slot = rarity_types[0];
        printf("Rarity: %s | Rolled: %d\n", rarity_agent.name, rarity_roller);
    }
    else if (rarity_roller < rarity_rare_threshold && 
             rarity_roller > rarity_common_threshold)
    {
        rarity_agent.name = "Rare";
        rarity_agent.slot = rarity_types[1];
        printf("Rarity: %s | Rolled: %d\n", rarity_agent.name, rarity_roller);
    }
    else if (rarity_roller < rarity_epic_threshold && 
             rarity_roller > rarity_rare_threshold)
    {
        rarity_agent.name = "Epic";
        rarity_agent.slot = rarity_types[2];
        printf("Rarity: %s | Rolled: %d\n", rarity_agent.name, rarity_roller);
    }
    else if (rarity_roller < rarity_legenary_threshold && 
             rarity_roller > rarity_epic_threshold)
    {
        rarity_agent.name = "Legendary";
        rarity_agent.slot = rarity_types[3];
        printf("Rarity: %s | Rolled: %d\n", rarity_agent.name, rarity_roller);
    }
    else if (rarity_roller < rarity_mythical_threshold && 
             rarity_roller > rarity_legenary_threshold)
    {
        rarity_agent.name = "Mythical";
        rarity_agent.slot = rarity_types[4];
        printf("Rarity: %s | Rolled: %d\n", rarity_agent.name, rarity_roller);
    }
    else
    {
        rarity_agent.name = "Invalid";
        fprintf(stderr, "Invalid rarity rolled: %d\n", rarity_roller);
    }    

    return rarity_agent;
}


void Personality_EnterScenario(personality_scenario_t *scenario, font_t *font, i32 index)
{

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
        
    // Only call once rather than every frame, caused an issue of the screen to flicker on startup.
    SDL_RenderSetLogicalSize(SDLWindow.Renderer, ASPECT_WIDTH, ASPECT_HEIGHT);    


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
    Personality_InitQuestions(&personality_test);

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

    bool is_name_submission = false;
    bool is_class_overview_screen = false;


    asset_t blank_screen_asset = {0};
    LoadAsset(&blank_screen_asset, "assets/blank_screen.png");
    InitializeAssetToRender(&blank_screen_asset, 0, 0, ASPECT_WIDTH, ASPECT_HEIGHT);

    asset_t title_screen_asset = {0};
    LoadAsset(&title_screen_asset, "assets/blank_screen.png");
    InitializeAssetToRender(&title_screen_asset, 0, 0, ASPECT_WIDTH, ASPECT_HEIGHT);
    
    asset_t new_game_command_menu = {0};
    LoadAsset(&new_game_command_menu, "assets/new_game_screen.png");
    InitializeAssetToRender(&new_game_command_menu, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    InitializeCamera();
    i32 CameraX = (int)(-SDLCamera.X);
    i32 CameraY = (int)(-SDLCamera.Y);


    // TODO(ben): Make an array of rooms for future levels and for the camera switch since it's also tied to the room
    i32 room_select = 1;
    asset_t room_asset[2] = {0};
    LoadAsset(&room_asset[0], "assets/map1.png");
    InitializeAssetToRender(&room_asset[0], CameraX, CameraY, room_asset[0].w, room_asset[0].h);

    asset_t down_stairs_asset = {0};
    LoadAsset(&down_stairs_asset, "assets/down_stairs.png");
    InitializeAssetToRender(&down_stairs_asset, 14 * 16, 2 * 24, down_stairs_asset.w, down_stairs_asset.h);

    asset_t forest_scenario_map = IdealLoadAsset("assets/forest_scenario.png");
    asset_t oldman_asset = IdealLoadAsset("assets/sprites/oldman.png");
    oldman_asset.conditions.has_physics = true;

    i32 boulder_count = 0;
    asset_t boulder_asset = IdealLoadAsset("assets/sprites/boulder.png");
    boulder_asset.conditions.has_physics = true;
    boulder_asset.conditions.has_movement = true;
 
    asset_t boulder_wall_asset[5] = {0};
    for (i32 i = 0; i < ArraySize(boulder_wall_asset); ++i)
    {
        boulder_wall_asset[i] = IdealLoadAsset("assets/sprites/boulder.png");
        boulder_wall_asset[i].conditions.has_physics = true;
    }

    asset_t forest_scenario_walls[2] = {0};
    forest_scenario_walls[0].body.x = 0;
    forest_scenario_walls[0].body.y = 240 - (48 + 24);
    forest_scenario_walls[0].body.w = forest_scenario_map.w;
    forest_scenario_walls[0].body.h = 24;
    forest_scenario_walls[0].conditions.has_physics = true;

    forest_scenario_walls[1].body.x = 0;
    forest_scenario_walls[1].body.y = 240 - (48 - 24 - 24 - 24);
    forest_scenario_walls[1].body.w = forest_scenario_map.w;
    forest_scenario_walls[1].body.h = 24;
    forest_scenario_walls[1].conditions.has_physics = true;
  
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
    forest_scenario_finish_line.conditions.has_physics = true;

    asset_t dialogue_box = {0};
    dialogue_box.body.x = 16; //256 + (16 * 8);
    dialogue_box.body.y = 150; //240 - (48);
    dialogue_box.body.w = 224;
    dialogue_box.body.h = 64;

    font_t font = {0};
    Font_Init(&font);

    
    i32 option_index = 0;
    option_t title_screen_options[4] = {
        { "New Game", SET_TEXT_CENTER_X("New Game", 0), SCREEN_CENTER_Y},
        { "Continue Game", SET_TEXT_CENTER_X("Continue Game", 0), SCREEN_CENTER_Y + 16 },
        { "Settings", SET_TEXT_CENTER_X("Settings", 0), SCREEN_CENTER_Y + 32 },
        { "Exit", SET_TEXT_CENTER_X("Exit", 0), SCREEN_CENTER_Y + 48 },
    };

    asset_t new_game_asset = {0};
    LoadAsset(&new_game_asset, "assets/new_game_screen.png");
    InitializeAssetToRender(&new_game_asset, 0, 0, new_game_asset.w, new_game_asset.h); 

    asset_t dialogue_box_asset = {0};
    LoadAsset(&dialogue_box_asset, "assets/ui/dialogue_box.png");
    InitializeAssetToRender(&dialogue_box_asset, 0, 0, dialogue_box_asset.w, dialogue_box_asset.h);
   
    typedef struct
    {
        asset_t model;
    } cursor_t;

#define MAX_CURSOR      4
#define UP_CURSOR       0
#define DOWN_CURSOR     1
#define LEFT_CURSOR     2
#define RIGHT_CURSOR    3

    cursor_t cursor[MAX_CURSOR] = {0};
    cursor[UP_CURSOR].model = IdealLoadAsset("assets/ui/up_cursor.png");
    cursor[DOWN_CURSOR].model = IdealLoadAsset("assets/ui/down_cursor.png");
    cursor[LEFT_CURSOR].model = IdealLoadAsset("assets/ui/left_cursor.png");
    cursor[RIGHT_CURSOR].model = IdealLoadAsset("assets/ui/right_cursor.png");

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
        i32 index;
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
    for (i32 i = 0; i < ArraySize(character_creation_screen.info); ++i)
    {
        LoadAsset(&character_creation_screen.info[i].asset, class_files[i]);
        character_creation_screen.info[i].name = class_names[i];
        character_creation_screen.info[i].description = class_description[i];
        character_creation_screen.info[i].asset.conditions.has_physics = true;
        character_creation_screen.info[i].asset.conditions.has_movement = true;
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
        "A personality test to determine your class's poi32 allocation.",
        "A preset allocation of your selected class.",
        "Manually allocate your class's points.",
    };

    typedef struct
    {
        // overall instance
        i32 index;
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
    for (i32 i = 0; i < ArraySize(character_allocation_select_screen.info); ++i)
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

    confirm_buttons_t confirmation = {0};
    ConfirmationButtons_Init(&confirmation, &font);

    printf("r: %d\n", confirmation.font->color[COLOR_RED].r);
    printf("g: %d\n", confirmation.font->color[COLOR_RED].g);
    printf("b: %d\n", confirmation.font->color[COLOR_RED].b);

    const char *scenario_name = '\0';

    personality_scenario_t scenarios = {0};
    Personality_InitScenarioOptions(&scenarios, village_scenario_options, VILLAGE_INDEX);
    Personality_InitScenarioOptions(&scenarios, monster_scenario_options, MONSTER_INDEX);
    Personality_InitScenarioOptions(&scenarios, cave_scenario_options,    CAVE_INDEX);
    Personality_InitScenarioOptions(&scenarios, desert_scenario_options,  DESERT_INDEX);
    Personality_InitScenarioOptions(&scenarios, tower_scenario_options,   TOWER_INDEX);
    Personality_InitScenarioOptions(&scenarios, theater_scenario_options, THEATER_INDEX);


    //////////////////////////////////////////////////////////////////////////////////////

    sound_settings_t sound_settings = {0};
    Sound_InitSettings(&sound_settings); 

    sound_volume_controller_t volume_controller[3] = {0};
    Sound_InitVolumeBar(&sound_settings, volume_controller, VOLUME_CONTROLLER_COUNT); 

    sound_volume_controller_t test_volume_controller = {0};
    Sound_TestInitVolumeBar(&sound_settings, &test_volume_controller, VOLUME_CONTROLLER_COUNT);


    ////////////// Enter name grid 
    name_entry_t name_entry = {0};
    NameEntry_Init(&name_entry);

    typedef struct
    {
        bool is_compatible;
        struct
        {
                // TODO: Load asset
            i32 id; // do we need?
            i32 defense;
            char name[10];
            char description[32];
        } helm[2];

        struct
        {
            i32 id; // do we need?
            i32 defense;
            char name[10];
            char description[32];
        } chest;

        struct
        {
            i32 id; // do we need?
            i32 attack;
            char name[10];
            char description[32];
        } main_hand;

        struct
        {
            i32 id; // do we need?
            i32 defense;
            char name[10];
            char description[32];
        } off_hand;

        struct
        {
            i32 id; // do we need?
            i32 defense;
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


    class_base_stats_t class_base_stats[4] = {0};
    class_base_stats[KNIGHT_ID] = Class_InitBaseStats(knight_base_stat_data);
    class_base_stats[PALADIN_ID] = Class_InitBaseStats(paladin_base_stat_data);
    class_base_stats[WIZARD_ID] = Class_InitBaseStats(wizard_base_stat_data);
    class_base_stats[ARCHER_ID] = Class_InitBaseStats(archer_base_stat_data);

    printf("Max HP: %d\n", class_base_stats[KNIGHT_ID].max_hp);

       
    class_stat_growth_per_level_t class_stat_growth[4] = {0};
    f32 knight_stat_baseline[5] = {
        15, 15, 15, 15, 15
    };

    f32 knight_growth_per_level[][20] = {
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

    Class_InitStatBaseline(&class_stat_growth[0], knight_stat_baseline);
    Class_InitStatGrowthPerLevel(&class_stat_growth[0], knight_growth_per_level);

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
    for (i32 i = 0; i < ArraySize(personality_stat_growth); ++i)
    {
        for (i32 j = 0; j < 5; ++j)
        {
            personality_stat_growth[i].stat_growth[j] = personality_stats[i][j];
        }
    }

    for (i32 i = 0; i < 5; ++i)
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
         i32 index;
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
    const char *dst_txt = "                           "; // Just to specify we're working with a str of size 27, for now
    char int_buffer[10][28]; 
    class_status_overview_t class_status_overview[12] = {
        // Box 1 - Info
        { {SET_TEXT_CENTER_X(dst_txt, 0),                 SCREEN_CENTER_Y - 96},    "Name:                      " },
        { {SET_TEXT_CENTER_X(dst_txt, 0),                 SCREEN_CENTER_Y - 86},    "Lv:                        " },
        { {SET_TEXT_CENTER_X(dst_txt, 0),                 SCREEN_CENTER_Y - 76},    "Class:                     " },
        { {SET_TEXT_CENTER_X(dst_txt, 0),                 SCREEN_CENTER_Y - 66},    "Personality:               " },

        // Box 2 - Stats
        { {SET_TEXT_CENTER_X(dst_txt, 0),   SCREEN_CENTER_Y - 46},                  "Strength:                 " },
        { {SET_TEXT_CENTER_X(dst_txt, 0),   SCREEN_CENTER_Y - 36},                  "Resillience:              " },
        { {SET_TEXT_CENTER_X(dst_txt, 0),   SCREEN_CENTER_Y - 26},                  "Agility:                  " },
        { {SET_TEXT_CENTER_X(dst_txt, 0),   SCREEN_CENTER_Y - 16},                  "Stamina:                  " },
        { {SET_TEXT_CENTER_X(dst_txt, 0),   SCREEN_CENTER_Y - 6},                   "Wisdom:                   " },
        { {SET_TEXT_CENTER_X(dst_txt, 0),   SCREEN_CENTER_Y + 4},                   "Luck:                     " },
        { {SET_TEXT_CENTER_X(dst_txt, 0),   SCREEN_CENTER_Y + 14},                  "Max HP:                  " },
        { {SET_TEXT_CENTER_X(dst_txt, 0),   SCREEN_CENTER_Y + 24},                  "Max MP:                  " },
    };
   
    bool game_init_stats = false;
    const char *test_txt = "              ";
    size_t test_txt_size = strlen(test_txt);
    printf("test_txt_size: %d\n", test_txt_size); 
    
    char game_int_buffer[10][17];
    class_status_overview_t in_game_class_status_overview[15] = {
        // Box 1 - Info
        { {SET_TEXT_CENTER_X(test_txt, -48),                 SCREEN_CENTER_Y - 94},    "Name:                " }, // name
        { {SET_TEXT_CENTER_X(test_txt, -48),                 SCREEN_CENTER_Y - 84},    "Lv:                  " },
        { {SET_TEXT_CENTER_X(test_txt, -48),                 SCREEN_CENTER_Y - 74},    "Class:               " }, // class
        { {SET_TEXT_CENTER_X(test_txt, -48),                 SCREEN_CENTER_Y - 64},    "Ego:                 " }, // personality
        { {SET_TEXT_CENTER_X(test_txt, -48),                 SCREEN_CENTER_Y - 54},    "Exp_to_next:         " },

        // Box 2 - Stats
        { {SET_TEXT_CENTER_X(test_txt, -48),   SCREEN_CENTER_Y - 34},                  "Str:                 " },
        { {SET_TEXT_CENTER_X(test_txt, -48),   SCREEN_CENTER_Y - 24},                  "Res:                 " },
        { {SET_TEXT_CENTER_X(test_txt, -48),   SCREEN_CENTER_Y - 14},                  "Agi:                 " },
        { {SET_TEXT_CENTER_X(test_txt, -48),   SCREEN_CENTER_Y - 4},                   "Sta:                 " },
        { {SET_TEXT_CENTER_X(test_txt, -48),   SCREEN_CENTER_Y + 6},                   "Wis:                 " },
        { {SET_TEXT_CENTER_X(test_txt, -48),   SCREEN_CENTER_Y + 16},                  "Lck:                 " },
        { {SET_TEXT_CENTER_X(test_txt, -48),   SCREEN_CENTER_Y + 26},                  "Max HP:              " },
        { {SET_TEXT_CENTER_X(test_txt, -48),   SCREEN_CENTER_Y + 36},                  "Max MP:              " },
        { {SET_TEXT_CENTER_X(test_txt, -48),   SCREEN_CENTER_Y + 46},                  "Atk:                 " },
        { {SET_TEXT_CENTER_X(test_txt, -48),   SCREEN_CENTER_Y + 56},                  "Def:                 " },
    };


    typedef struct
    {
        i32 index;
        bool is_active;
        option_t button[1];
    } next_button_t;

    next_button_t next_button = {0};
    next_button.button[0].text = "Next";
    next_button.button[0].x = SET_TEXT_CENTER_X(next_button.button[0].text, 96);
    next_button.button[0].y = SCREEN_CENTER_Y + 96;

    bool boulder_has_reached_end = false;

    // Start event
    Running = true;

    SDL_Color hold_my_color = {0};

    bool personality_results_screen = false;
    
    bool player_talking_to_oldman = false;
    bool first_forest_entrance = false;
    bool hold_dialogue_box = false;
    bool hold_dialogue_box_return_boulder = false;
    bool next_text = false;
    bool display_player_gold_count = false;

    typedef struct
    {
        // personality_screen_t personality_screen;
        // personality_screen.is_active;
        // personality_screen.is_complete;
        bool is_active;
        bool is_complete;

    } personality_screen_t;

    
    i32 dialogue_index = 0;
    const char *forest_scenario_dialogue_intro[32] = {
        {"Whoa-whoa-whoa!\nI see you're lost. Go"},
        {"west. That's to your left.\nKeep walking"},
        {"in that direction and\nyou'll be clear of"},
        {"this forest. If you\nhappen to see a boulder,"},
        {"could you push it back\nto me? I will be sure"},
        {"to repay you."}
    };

    i32 dialogue_index_2 = 0;
    const char *forest_scenario_dialogue_return_the_boulder[32] = {
        {"Oh! You brought the\nrock to me. Thank you!"},
        {"Here's 10 gold coins\nfor your troubles."}
    };

    i32 gold_count_from_old_man = 0;

    printf("%s\n", forest_scenario_dialogue_intro[dialogue_index]);
    printf("%s\n", forest_scenario_dialogue_return_the_boulder[dialogue_index_2]);

    SDL_GameController *controller = NULL;
    for (i32 i = 0; i < SDL_NumJoysticks(); ++i)
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

    SDL_Color stat_overview_color[8] = {0};
  
    // -------- In-game menus --------
    
    // A menu that opens when the player hits ESC, options for sound settings and to exit,
    // save in the future
    typedef struct
    {
        i32 index;
        bool is_active;

        // Two options for sound settings and exit
        option_t options[2];

        // Init with our current volume settings
        sound_settings_t *sound_settings;
    } game_settings_t;


    #define ITEM_STACK_MAX 99
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
        
        // Tracks the number of current items in inventory
        u32 count;
        char count_buffer[2];

        bool in_inventory;
        u32 hp_recovery;
        u32 mp_recovery;
        u32 buy_value;
        u32 sell_value;
        
        char *name;
        char *description;
        asset_t asset;
    } game_item_t;

    typedef struct
    {
        game_item_t data;
        asset_t box;
    } game_item_decription_t;

    game_item_decription_t game_item_description = {0};
    game_item_description.box = IdealLoadAsset("assets/ui/item_description_box.png");

    // Example use of item creation below
    
    typedef enum
    {
        ITEM_NONE,
        ITEM_HP_POTION,
        ITEM_MP_POTION
    } item_type;

#define ITEM_COUNT 2
#define ITEM_HEALTH_POTION 0
#define ITEM_MANA_POTION   1 
    game_item_t game_items[ITEM_COUNT] = {0};

    game_items[ITEM_HEALTH_POTION].id = 0;
    game_items[ITEM_HEALTH_POTION].in_inventory = true;
    game_items[ITEM_HEALTH_POTION].count = 0;
    game_items[ITEM_HEALTH_POTION].hp_recovery = 20;
    game_items[ITEM_HEALTH_POTION].mp_recovery = 0;
    game_items[ITEM_HEALTH_POTION].buy_value = 5;
    game_items[ITEM_HEALTH_POTION].sell_value = 1;
    game_items[ITEM_HEALTH_POTION].name = "Health Potion";
    game_items[ITEM_HEALTH_POTION].description = "Recovers 20 HP";
    game_items[ITEM_HEALTH_POTION].asset = IdealLoadAsset("assets/items/health_potion.png");
    game_items[ITEM_HEALTH_POTION].asset.body.x = 16 * 11;
    game_items[ITEM_HEALTH_POTION].asset.body.y = 24 * 16;
    game_items[ITEM_HEALTH_POTION].asset.conditions.has_physics = true;
    SetAssetAdjacentHitBoxes(&game_items[ITEM_HEALTH_POTION].asset);

    game_items[ITEM_MANA_POTION].id = 1;
    game_items[ITEM_MANA_POTION].in_inventory = true;
    game_items[ITEM_MANA_POTION].count = 0;
    game_items[ITEM_MANA_POTION].hp_recovery = 0;
    game_items[ITEM_MANA_POTION].mp_recovery = 30;
    game_items[ITEM_MANA_POTION].buy_value = 5;
    game_items[ITEM_MANA_POTION].sell_value = 1;
    game_items[ITEM_MANA_POTION].name = "Mana Potion";
    game_items[ITEM_MANA_POTION].description = "Recovers 30 MP";
    game_items[ITEM_MANA_POTION].asset = IdealLoadAsset("assets/items/mana_potion.png");
  
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
        bool in_inventory;

        u32 attack;
        u32 defense;
        u32 buy_value;
        u32 sell_value;

        u32 range;
        char *name;
        char *effect; // may have different effects from its rarity 
        
        // color.white -> common -> 61% 
        // blue -> rare -> 25%
        // purple -> epic -> 8%
        // yellow -> legendary -> 5%
        // red -> mythical -> 1%
        //char *rarity; 
       
        asset_t model;
        game_rarity_roller_t rarity;
    } game_equipment_t;
 
    asset_t slot_default    = IdealLoadAsset("assets/ui/default_slot.png"); // -> Brown
    asset_t slot_common     = IdealLoadAsset("assets/ui/common_slot.png"); // -> White
    asset_t slot_rare       = IdealLoadAsset("assets/ui/rare_slot.png"); // -> Blue
    asset_t slot_epic       = IdealLoadAsset("assets/ui/epic_slot.png"); // -> Purple
    asset_t slot_legendary  = IdealLoadAsset("assets/ui/legendary_slot.png"); // -> Yellow 
    asset_t slot_mythical   = IdealLoadAsset("assets/ui/mythical_slot.png"); // -> Red

    asset_t rarity_types[5] = {0};
    rarity_types[0] = slot_common;
    rarity_types[1] = slot_rare;
    rarity_types[2] = slot_epic;
    rarity_types[3] = slot_legendary;
    rarity_types[4] = slot_mythical;

    // long sword example
    game_equipment_t equipment[1] = {0};
    equipment[0].id = 0;
    equipment[0].in_inventory = true; // temp
    equipment[0].attack = 5;
    equipment[0].buy_value = 10;
    equipment[0].sell_value = 5;
    equipment[0].range = 2; // 2 tiles
     
    equipment[0].name = "Long Sword";
    equipment[0].rarity = Game_RarityRoller(rarity_types); // randomly roll through the rarities and apply name, common default, we may even have it roll as well when the player spawns and not only through chests, just for that extra spice. 
    equipment[0].model = IdealLoadAsset("assets/weapons/long_sword.png");


    typedef struct
    {
        // Properties of each slot:
        //  -> bool to check if it's empty or not, if it's empty a new item can be stored, if it's an existing item
        //  you're picking it then it can be stacked.

        bool occupied;
        game_item_t *item; // Pass created item into slot
        game_item_decription_t description_box;

        // Tracks number of THIS item is stacked, limit -> 99
        u32 count; // only items that share the same ID can stack
        
        asset_t model;
        game_rarity_roller_t rarity;
    } game_command_menu_slots_t;

    #define ITEMS_SLOT_GRID_COLS  5
    #define ITEMS_SLOT_GRID_ROWS  5
    #define COMMAND_MENU_ITEMS_SLOT_COUNT 25     
    #define COMMAND_MENU_EQUIP_SLOT_COUNT 15     
    typedef struct
    {
        u32 index;
        u32 selected_index;
        bool is_active;
        bool has_movement;

        bool is_opened;
        game_command_menu_slots_t slots[COMMAND_MENU_ITEMS_SLOT_COUNT]; 
    } game_command_menu_items_t;

    // For now, not efficient but will work
    vec2_t item_slot_coords[COMMAND_MENU_ITEMS_SLOT_COUNT] = {
        // First row
        {SCREEN_CENTER_X - 90, SCREEN_CENTER_Y - 66},
        {SCREEN_CENTER_X - 68, SCREEN_CENTER_Y - 66},
        {SCREEN_CENTER_X - 46, SCREEN_CENTER_Y - 66},
        {SCREEN_CENTER_X - 24, SCREEN_CENTER_Y - 66},
        {SCREEN_CENTER_X - 2,  SCREEN_CENTER_Y - 66},

        // Second row
        {SCREEN_CENTER_X - 90, SCREEN_CENTER_Y - 44},
        {SCREEN_CENTER_X - 68, SCREEN_CENTER_Y - 44},
        {SCREEN_CENTER_X - 46, SCREEN_CENTER_Y - 44},
        {SCREEN_CENTER_X - 24, SCREEN_CENTER_Y - 44},
        {SCREEN_CENTER_X - 2,  SCREEN_CENTER_Y - 44},

        // Third Row
        {SCREEN_CENTER_X - 90, SCREEN_CENTER_Y - 22},
        {SCREEN_CENTER_X - 68, SCREEN_CENTER_Y - 22},
        {SCREEN_CENTER_X - 46, SCREEN_CENTER_Y - 22},
        {SCREEN_CENTER_X - 24, SCREEN_CENTER_Y - 22},
        {SCREEN_CENTER_X - 2,  SCREEN_CENTER_Y - 22},

        // Fourth Row
        {SCREEN_CENTER_X - 90, SCREEN_CENTER_Y},
        {SCREEN_CENTER_X - 68, SCREEN_CENTER_Y},
        {SCREEN_CENTER_X - 46, SCREEN_CENTER_Y},
        {SCREEN_CENTER_X - 24, SCREEN_CENTER_Y},
        {SCREEN_CENTER_X - 2,  SCREEN_CENTER_Y},

        // Fifth Row
        {SCREEN_CENTER_X - 90, SCREEN_CENTER_Y + 22},
        {SCREEN_CENTER_X - 68, SCREEN_CENTER_Y + 22},
        {SCREEN_CENTER_X - 46, SCREEN_CENTER_Y + 22},
        {SCREEN_CENTER_X - 24, SCREEN_CENTER_Y + 22},
        {SCREEN_CENTER_X - 2,  SCREEN_CENTER_Y + 22},

    };

    typedef struct
    {
        u32 index;
        bool has_movement;
        bool open_options;
    } game_slot_options_t;

    int item_slot_option_index = 0;
    bool item_slot_option = false;
    bool item_slot_option_has_movement = false;
    asset_t item_slot_asset = IdealLoadAsset("assets/ui/slot_options.png");
    asset_t item_menu_description_box = IdealLoadAsset("assets/ui/item_description_box.png");
    option_t item_slot_options[3] = {
        {"Use", SET_TEXT_CENTER_X("Use", 0), SCREEN_CENTER_Y - 16},
        {"Move", SET_TEXT_CENTER_X("Move", 0), SCREEN_CENTER_Y},
        {"Toss", SET_TEXT_CENTER_X("Toss", 0), SCREEN_CENTER_Y + 16},
    };
    
    game_command_menu_items_t game_command_menu_items = {0};
    u32 item_slot_current_row = game_command_menu_items.index / ITEMS_SLOT_GRID_COLS;
    u32 item_slot_current_col = game_command_menu_items.index % ITEMS_SLOT_GRID_ROWS;

    // Example code
    game_items[ITEM_HEALTH_POTION].count = 0;
    game_items[ITEM_MANA_POTION].count = 4;

    game_command_menu_items.slots[0].count = 4;    
    game_command_menu_items.slots[1].count = 0;    

    // Hardcode example, ideally we call a function once that pushes or pops an item in/out of the inventory
    for (int i = 0; i < 2; ++i)
    {
        game_command_menu_items.slots[i].item = &game_items[i];

        if (game_command_menu_items.slots[i].item->in_inventory)
        {
            game_command_menu_items.slots[i].occupied = true;
            

            printf("Item name: %s\n", game_command_menu_items.slots[i].item->name);
        }
        else
        {
            game_command_menu_items.slots[i].occupied = false;
        }

    }

    for (i32 i = 0; i < COMMAND_MENU_ITEMS_SLOT_COUNT; ++i)
    {
        game_command_menu_items.slots[i].model = slot_default;
        game_command_menu_items.slots[i].model.x = item_slot_coords[i].x;
        game_command_menu_items.slots[i].model.y = item_slot_coords[i].y;
    }

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
        
        i32 index;
        cursor_t cursor;

    } game_command_menu_equip_t;
    game_command_menu_equip_t game_command_menu_equip = {0};

    typedef struct
    {
        cursor_t cursor;

    
    } game_command_menu_status_t;
    game_command_menu_status_t game_command_menu_status = {0};

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
        
        //   |        | > | Items     |
        //   |        |   | Equipment |
        //   |        |   | Status    |
        //   |        |   | ...       |
        //   |        |   | ...       |


        /* Items -> Overview of entire inventory, player can use items and only examine equipments
         * Attack ?
         * Spells ?
         * Equipment -> Inventory for only equipments that the player can examine and equip
         * Status -> Overview of character's status; name, level, personality, exp, remaining exp to next level, stats and more
         */

        // Cursor package -> index and bool
        i32 index;
        bool is_active; 
        bool has_movement;
        
        // Menu package
        bool is_opened;
        // Asset for the command menu and respective options (items, equipment ...)
        asset_t box;
        asset_t option_box[3];
        
        option_t options[3]; // items, equipment and status for now
    } game_command_menu_t;

#define COMMAND_MENU_OPTION_COUNT       3
#define COMMAND_MENU_ITEM               0
#define COMMAND_MENU_EQUIP              1
#define COMMAND_MENU_STATUS             2

    game_command_menu_t game_command_menu = {0};
    game_command_menu.box = IdealLoadAsset("assets/ui/command_menu_box.png");
    
    game_command_menu.option_box[0] = IdealLoadAsset("assets/ui/command_menu_options_box.png");
    game_command_menu.option_box[1] = IdealLoadAsset("assets/ui/command_menu_options_box.png");
    game_command_menu.option_box[2] = IdealLoadAsset("assets/ui/command_menu_options_box - Copy.png");

    game_command_menu.options[0].text = "Items",
    game_command_menu.options[0].x = SET_TEXT_CENTER_X("Items", 72),
    game_command_menu.options[0].y = SCREEN_CENTER_Y - 64;
    
    game_command_menu.options[1].text = "Equip",
    game_command_menu.options[1].x = SET_TEXT_CENTER_X("Equip", 72),
    game_command_menu.options[1].y = SCREEN_CENTER_Y - 48;

    game_command_menu.options[2].text = "Status",
    game_command_menu.options[2].x = SET_TEXT_CENTER_X("Status", 72),
    game_command_menu.options[2].y = SCREEN_CENTER_Y - 32;


    typedef struct
    {
        /* Is this tile occupied? If yes, the player can inspect it, otherwise no action
           
           Will not render an asset, but just a rect as it's hitbox. Could come across the same problem with the adjacent hitboxes from a sprite, where because the pixels
           may still be touching standing NEXT to the hitbox and not on, not cutting the size of the hitbox down and instead leaving it as is, could lead to problems detecting 
           invalid tiles.

           Number of tiles must be relative to size of the room, what we can do is use the W and H of the current room's asset, divide it into tiles of 16x24, and render
           them that way. Not concerned if these tiles are rendered over anything that isn't a floor.

            We may also just handle the wall collision here as well. If the wall has collision on it, then it's not considered occupied thus the hitbox for items are not rendered there, prevents rendering useless tiles.
        
        */

        int rows, cols;
        // variables to handle items ...

        //SDL_Rect sub_tiles;

        asset_t collison_tiles[580];
        asset_t item_tiles[580]; 
    } floor_tile_t;

    typedef struct
    {
        asset_t collision_tiles;
        asset_t item_tiles;
    } test_floor_tile_t;

    typedef struct
    {
        test_floor_tile_t floor_tiles[580];
    } rooms_t;


    int main_room_cols = room_asset[0].w / TILE_WIDTH;
    int main_room_rows = room_asset[0].h / TILE_HEIGHT;

    int total_floor_tiles = main_room_rows * main_room_cols;
    printf("total tiles: %d\n", total_floor_tiles);

    rooms_t map_rooms[1] = {0};
    for (int i = 0; i < main_room_rows; ++i)
    {
        for (int j = 0; j < main_room_cols; ++j)
        {
            int index = i * main_room_cols + j;
            map_rooms[0].floor_tiles[index].collision_tiles.body.x = (j * TILE_WIDTH);
            map_rooms[0].floor_tiles[index].collision_tiles.body.y = (i * TILE_HEIGHT);
            map_rooms[0].floor_tiles[index].collision_tiles.body.w = TILE_WIDTH;
            map_rooms[0].floor_tiles[index].collision_tiles.body.h = TILE_HEIGHT;

            map_rooms[0].floor_tiles[index].item_tiles.body.x = (map_rooms[0].floor_tiles[index].collision_tiles.body.x + (TILE_WIDTH - SUB_TILE_WIDTH) / 2);
            map_rooms[0].floor_tiles[index].item_tiles.body.y = (map_rooms[0].floor_tiles[index].collision_tiles.body.y + (TILE_HEIGHT - SUB_TILE_HEIGHT) / 2); 
            map_rooms[0].floor_tiles[index].item_tiles.body.w = SUB_TILE_WIDTH;
            map_rooms[0].floor_tiles[index].item_tiles.body.h = SUB_TILE_HEIGHT;
        }
    }

    // Top Wall
    int col_start_index = 0;
    for (int i = col_start_index; i < main_room_cols; ++i)
    {
        map_rooms[0].floor_tiles[i].collision_tiles.conditions.has_physics = true;
    }
    // Bottom Wall
    int col_end_index = (main_room_rows - 1) * main_room_cols;
    for (int i = col_end_index; i < total_floor_tiles; ++i)
    {
        map_rooms[0].floor_tiles[i].collision_tiles.conditions.has_physics = true;
    }
    // Left Wall
    for (int i = 0; i < main_room_rows; ++i)
    {
        int index = i * main_room_cols;
        map_rooms[0].floor_tiles[index].collision_tiles.conditions.has_physics = true;
    }
    // Right Wall
    for (int i = 0; i < main_room_rows; ++i) 
    {
        int index = i * main_room_cols + main_room_cols - 1;
        map_rooms[0].floor_tiles[index].collision_tiles.conditions.has_physics = true;
    }

    // Main room -> Left Room
    map_rooms[0].floor_tiles[117].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[118].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[119].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[120].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[121].collision_tiles.conditions.has_physics = true;

    map_rooms[0].floor_tiles[123].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[124].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[125].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[126].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[127].collision_tiles.conditions.has_physics = true;

    map_rooms[0].floor_tiles[128].collision_tiles.conditions.has_physics = true;  
    map_rooms[0].floor_tiles[41].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[70].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[99].collision_tiles.conditions.has_physics = true;  

    // Main room -> Right Room
    map_rooms[0].floor_tiles[45].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[74].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[103].collision_tiles.conditions.has_physics = true;  
    map_rooms[0].floor_tiles[132].collision_tiles.conditions.has_physics = true;
    
    map_rooms[0].floor_tiles[133].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[134].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[135].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[136].collision_tiles.conditions.has_physics = true; 
    map_rooms[0].floor_tiles[137].collision_tiles.conditions.has_physics = true; 

    map_rooms[0].floor_tiles[139].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[140].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[141].collision_tiles.conditions.has_physics = true;
    map_rooms[0].floor_tiles[142].collision_tiles.conditions.has_physics = true; 
    map_rooms[0].floor_tiles[143].collision_tiles.conditions.has_physics = true;
    
    asset_t tile_sparkle = IdealLoadAsset("assets/rooms/sparkle.png");
    asset_t loot_box = IdealLoadAsset("assets/chest_box.png");

    // Floor has an item test
    // TODO: Walking on a tile with an item should prompt the user on the bottom left
    // or right of the screen to press the confirm button to pick up item, otherwise
    // it's just something shiny on the ground. It won't be intuitive on first impression.
    bool touch_item_on_floor = false;

    int player_position_on_map = 0;
    map_rooms[0].floor_tiles[443].collision_tiles.conditions.is_occupied = true;
    map_rooms[0].floor_tiles[442].collision_tiles.conditions.is_occupied = true;
    map_rooms[0].floor_tiles[441].collision_tiles.conditions.is_occupied = true;
    printf("floor tile x: %d\n", map_rooms[0].floor_tiles[443].collision_tiles.body.x);
    printf("floor tile y: %d\n", map_rooms[0].floor_tiles[443].collision_tiles.body.y);

    asset_t player_card_mock = IdealLoadAsset("assets/player_card_mockup_3.png");
    asset_t enemy_card_mock = IdealLoadAsset("assets/enemy_card_mockup_3.png");
    
    asset_card_stack_t enemy_card_stack = {0};

    for (int i = 0; i < MAX_CARD_STACK; ++i)
        printf("enemy_card_stack: %d\n", enemy_card_stack.asset[i].x);
    
    // Push and pop asset_t within a stack
    printf("w: %d\n", enemy_card_mock.w);

    // Combat mode - Triggered event by being in the line of sight of an enemy
    // Enemy will have a line of sight that stems from their "front", and where their front is is where that
    // line renders. Then they have a chase radius that will be the same distance as their line of sight. The chase
    // radius is only relevant in combat mode such that if you have not already triggered the enemy, walking into
    // their chase radius does nothing. It's only relevant during combat in which let's say you are running and not
    // aligned with their line of sight, but still in their chase radius, you'll remain in combat mode, where in this
    // case, if you are out of range.
    // Line of sight - Every enemy type will have a different line of sight range.
    // Sprite's Front - The front of a sprite is the direction in which they last turned. 

    vec2_t player_starting_coords = Vec2_SetPosition(16 * 10, 24 * 16);
    vec2_t enemy_starting_coords = Vec2_SetPosition(16 * 14, 24 * 16);
    
    asset_t enemy_los_asset = IdealLoadAsset("assets/sprites/enemy1.png");
    InitializeAsset(&enemy_los_asset, &enemy_starting_coords);

    ////////////////////////////////////////////////////////////////////

    static personality_results_t personality_result = {0};

    typedef struct
    {
        bool first_entrance;
        bool player_is_out_of_bounds;

        i32 boulder_count;
        
    } scenario_forest_t;


    const char *theater_scenario_result[] = {
        {"You are a priest, and dressed for"},
        {"night's stage show. You walk into"},
        {"the theater and a man recognizes you"},
        {"as the town's priest. He immediately"},
        {"begs you to marry the women of life, of"},
        {"life, of which they had only just met"},
        {"and he claims is love at first sight."},
        {"What do you do?"}
    };

    option_t theater_scenario_results[8] = {
        { "You are a priest, and dressed for",          SET_TEXT_CENTER_X("You are a priest, and dressed for", 0),          SCREEN_CENTER_Y - 88 },
        { "night's stage show. You walk into",          SET_TEXT_CENTER_X("night's stage show. You walk into", 0),          SCREEN_CENTER_Y - 80 },
        { "the theater and a man recognizes you",       SET_TEXT_CENTER_X("the theater and a man recognizes you", 0),       SCREEN_CENTER_Y - 72 },
        { "as the town's priest. He immediately",       SET_TEXT_CENTER_X("as the town's priest. He immediately", 0),       SCREEN_CENTER_Y - 64 },
        { "begs you to marry the women of life, of",    SET_TEXT_CENTER_X("begs you to marry the women of life, of", 0),    SCREEN_CENTER_Y - 56 },
        { "life, of which they had only just met",      SET_TEXT_CENTER_X("life, of which they had only just met", 0),      SCREEN_CENTER_Y - 48 },
        { "and he claims is love at first sight.",      SET_TEXT_CENTER_X("and he claims is love at first sight", 0),       SCREEN_CENTER_Y - 40 },
        { "What do you do?",                            SET_TEXT_CENTER_X("What do you do?", 0),                            SCREEN_CENTER_Y - 32 },
    };



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
                                if (scenarios.scenario[FOREST_INDEX].is_active)
                                {
                                    character_data.model.direction.up = true;
                                    boulder_asset.direction.up = true;
                                }

                                if (is_game_running)
                                {
                                    character_data.model.direction.up = true;
                                   
                                    character_data.model.los.front = front_face_zero;
                                    character_data.model.los.front.is_up = true;

                                    printf("up: %d\n", character_data.model.los.front.is_up);
                                    printf("down: %d\n", character_data.model.los.front.is_down);
                                    printf("left: %d\n", character_data.model.los.front.is_left);
                                    printf("right: %d\n", character_data.model.los.front.is_right);

                                }

                                if (boulder_has_reached_end)
                                    boulder_has_reached_end = false;

                                if (is_title_screen)
                                {
                                    option_index--;
                                    if (option_index < 0)
                                        option_index = ArraySize(title_screen_options) - 1;
                                    Sound_PlaySFX(&master_volume.sfx[0]->wav);
                                }

                                if (confirmation.is_active)
                                {
                                    confirmation.index--;
                                    if (confirmation.index < 0)
                                        confirmation.index = ArraySize(confirmation.button) - 1;
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

                                Personality_MoveUpScenario(&scenarios);

                                if (is_game_running && game_command_menu.is_opened && game_command_menu.has_movement)
                                {
                                    game_command_menu.index--;
                                    if (game_command_menu.index < 0)
                                        game_command_menu.index = ArraySize(game_command_menu.options) - 1;
                                    printf("game command index: %d\n", game_command_menu.index); 
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav); // Change the sfx
                                }

                                if (is_game_running && game_command_menu_items.is_opened && game_command_menu_items.has_movement)
                                {
                                    item_slot_current_row = (item_slot_current_row - 1 + ITEMS_SLOT_GRID_ROWS) % ITEMS_SLOT_GRID_ROWS;
                                    game_command_menu_items.index = item_slot_current_row * ITEMS_SLOT_GRID_COLS + item_slot_current_col;
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav); // Change the sfx
                                }

                                if (is_game_running && item_slot_option_has_movement)
                                {
                                    item_slot_option_index--;
                                    if (item_slot_option_index < 0)
                                        item_slot_option_index = ArraySize(item_slot_options) - 1;
                                }



                            } break;
                            case SDLK_s:
                            {
                                if (scenarios.scenario[FOREST_INDEX].is_active)
                                {
                                    character_data.model.direction.down = true;
                                    boulder_asset.direction.down = true;
                                } 
                               
                                if (is_game_running)
                                {
                                    character_data.model.direction.down = true;
                                   
                                    character_data.model.los.front = front_face_zero;
                                    character_data.model.los.front.is_down = true;

                                    printf("up: %d\n", character_data.model.los.front.is_up);
                                    printf("down: %d\n", character_data.model.los.front.is_down);
                                    printf("left: %d\n", character_data.model.los.front.is_left);
                                    printf("right: %d\n", character_data.model.los.front.is_right);
                                }

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

                                if (confirmation.is_active)
                                {
                                    confirmation.index++;
                                    if (confirmation.index >= ArraySize(confirmation.button))
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

                                Personality_MoveDownScenario(&scenarios);

                                if (is_game_running && game_command_menu.is_opened && game_command_menu.has_movement)
                                {
                                    game_command_menu.index++;
                                    if (game_command_menu.index >= ArraySize(game_command_menu.options))
                                        game_command_menu.index = 0;

                                    printf("game command index: %d\n", game_command_menu.index); 
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                }
                               
                                if (is_game_running && game_command_menu_items.is_opened && game_command_menu_items.has_movement)
                                {
                                    item_slot_current_row = (item_slot_current_row + 1) % ITEMS_SLOT_GRID_ROWS;
                                    game_command_menu_items.index = item_slot_current_row * ITEMS_SLOT_GRID_COLS + item_slot_current_col;
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav); // Change the sfx
                                }
                                
                                if (is_game_running && item_slot_option_has_movement)
                                {
                                    item_slot_option_index++;
                                    if (item_slot_option_index >= ArraySize(item_slot_options))
                                        item_slot_option_index = 0;
                                }
                            } break;
                            case SDLK_a:
                            {
                                if (scenarios.scenario[FOREST_INDEX].is_active)
                                {
                                    character_data.model.direction.left = true;
                                    boulder_asset.direction.left = true;
                                }

                                if (is_game_running)
                                {
                                    character_data.model.direction.left = true;
                                    
                                    character_data.model.los.front = front_face_zero;
                                    character_data.model.los.front.is_left = true;

                                    printf("up: %d\n", character_data.model.los.front.is_up);
                                    printf("down: %d\n", character_data.model.los.front.is_down);
                                    printf("left: %d\n", character_data.model.los.front.is_left);
                                    printf("right: %d\n", character_data.model.los.front.is_right);
                                }

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

                                    for (i32 vc = 0; vc < VOLUME_CONTROLLER_COUNT; ++vc) 
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

                                if (is_game_running && game_command_menu_items.is_opened && game_command_menu_items.has_movement)
                                {
                                    item_slot_current_col = (item_slot_current_col - 1 + ITEMS_SLOT_GRID_COLS) % ITEMS_SLOT_GRID_COLS;
                                    game_command_menu_items.index = item_slot_current_row * ITEMS_SLOT_GRID_COLS + item_slot_current_col;
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav); // Change the sfx
                                }
                            } break;
                            case SDLK_d:
                            {
                                if (scenarios.scenario[FOREST_INDEX].is_active)
                                {
                                    character_data.model.direction.right = true;
                                    boulder_asset.direction.right = true;
                                }

                                if (is_game_running)
                                {
                                    character_data.model.direction.right = true;
                                    
                                    character_data.model.los.front = front_face_zero;
                                    character_data.model.los.front.is_right = true;

                                    printf("up: %d\n", character_data.model.los.front.is_up);
                                    printf("down: %d\n", character_data.model.los.front.is_down);
                                    printf("left: %d\n", character_data.model.los.front.is_left);
                                    printf("right: %d\n", character_data.model.los.front.is_right);
                                }

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

                                    for (i32 vc = 0; vc < VOLUME_CONTROLLER_COUNT; ++vc) 
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

                                if (is_game_running && game_command_menu_items.is_opened && game_command_menu_items.has_movement)
                                {
                                    item_slot_current_col = (item_slot_current_col + 1) % ITEMS_SLOT_GRID_COLS;
                                    game_command_menu_items.index = item_slot_current_row * ITEMS_SLOT_GRID_COLS + item_slot_current_col;
                                    Sound_PlaySFX(&master_volume.sfx[1]->wav); // Change the sfx
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
                                if (is_game_running && !game_command_menu.is_opened)
                                {
                                    game_command_menu.is_opened = true;
                                    game_command_menu.is_active = true;
                                    game_command_menu.has_movement = true;

                                    // Player cannot move in this event
                                    character_data.model.conditions.has_movement = false;
                                    printf("menu opened\n");
                                }
                                // If the the command menu is opened, AND only if the other menus are not opened/used, the command
                                // menu will close. Otherwise, pressing the TAB/ESC key will close the menu regardless if you're using 
                                // any of them.
                                else if (is_game_running && game_command_menu.is_opened &&
                                         (!game_command_menu_items.is_opened))
                                {
                                    game_command_menu.is_opened = false;
                                    game_command_menu.is_active = false;
                                    game_command_menu.has_movement = false;
                                   
                                    game_command_menu.index = 0; // Set the cursor back to the top of the list, items

                                    // Player can now move
                                    character_data.model.conditions.has_movement = true;
                                    printf("menu closed\n");
                                }

                                // Deactivate the item menu
                                if (is_game_running && game_command_menu_items.is_active)
                                {
                                    game_command_menu_items.is_opened = false;
                                    game_command_menu.has_movement = true;
                                }

                                if (is_game_running && item_slot_option)
                                {
                                    game_command_menu_items.has_movement = true;
                                    game_command_menu_items.is_active = true;
                                    game_command_menu.is_active = true;
                                    item_slot_option = false;
                                    item_slot_option_has_movement = false;
                                }

                                if (is_game_running && touch_item_on_floor)
                                {
                                    game_command_menu.is_opened = false;
                                    game_command_menu.is_active = false;
                                    game_command_menu.has_movement = false;
                                    character_data.model.conditions.has_movement = true;
                                    
                                    touch_item_on_floor = false;
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
                                    PersonalityTest_BranchQuestions(&personality_test, confirmation.index);
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
                                    if (scenarios.result & SCENARIO_VILLAGE)
                                    {
                                        switch (scenarios.scenario[VILLAGE_INDEX].index)
                                        {
                                            case 0:
                                            {
                                                printf("Show-off\n");
                                                personality_types_state = PERSONALITY_SHOW_OFF;
                                                PushString(scenarios.personality, "Show-Off");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[VILLAGE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_VILLAGE);

                                                personality_results_screen = true;
                                            } break;
                                            case 1:
                                            {
                                                printf("Slippery Devil\n");
                                                personality_types_state = PERSONALITY_SLIPPERY_DEVIL;
                                                PushString(scenarios.personality, "Slippery Devil");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[VILLAGE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_VILLAGE);

                                                personality_results_screen = true;
                                            } break;
                                            case 2:
                                            {
                                                printf("Shrinking Violet\n");
                                                personality_types_state = PERSONALITY_SHRINKING_VIOLET;
                                                PushString(scenarios.personality, "Shrinking Violet");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[VILLAGE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_VILLAGE);

                                                personality_results_screen = true;
                                            } break;
                                        }

                                    }

                                    if (scenarios.result & SCENARIO_MONSTER)
                                    {
                                        switch (scenarios.scenario[MONSTER_INDEX].index)
                                        {
                                            case 0: 
                                            {
                                                printf("Paragon\n");
                                                personality_types_state = PERSONALITY_PARAGON;
                                                PushString(scenarios.personality, "Paragon");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[MONSTER_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_MONSTER);

                                                personality_results_screen = true;
                                            } break;
                                            case 1: 
                                            {
                                                printf("Wimp\n");
                                                personality_types_state = PERSONALITY_WIMP;
                                                PushString(scenarios.personality, "Wimp");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[MONSTER_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_MONSTER);

                                                personality_results_screen = true;
                                            } break;
                                            case 2: 
                                            {
                                                printf("Spoilt Brat\n");
                                                personality_types_state = PERSONALITY_SPOILT_BRAT;
                                                PushString(scenarios.personality, "Spoilt Brat");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[MONSTER_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_MONSTER);

                                                personality_results_screen = true;
                                            } break;
                                            case 3: 
                                            {
                                                printf("Egghead\n");
                                                personality_types_state = PERSONALITY_EGGHEAD;
                                                PushString(scenarios.personality, "Egghead");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[MONSTER_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_MONSTER);

                                                personality_results_screen = true;
                                            } break;
                                            case 4: 
                                            {
                                                printf("Klutz\n");
                                                personality_types_state = PERSONALITY_KLUTZ;
                                                PushString(scenarios.personality, "Klutz");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[MONSTER_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_MONSTER);

                                                personality_results_screen = true;
                                            } break;
                                        } 
                                    }

                                    if (scenarios.result & SCENARIO_CAVE)
                                    {
                                        switch (scenarios.scenario[CAVE_INDEX].index)
                                        {
                                            case 0: 
                                            {
                                                printf("Straight Arrow\n");
                                                personality_types_state = PERSONALITY_STRAIGHT_ARROW;
                                                PushString(scenarios.personality, "Straight Arrow");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[CAVE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_CAVE);

                                                personality_results_screen = true;
                                            } break;
                                            case 1: 
                                            {
                                                printf("Mule\n");
                                                personality_types_state = PERSONALITY_MULE;
                                                PushString(scenarios.personality, "Mule");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[CAVE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_CAVE);

                                                personality_results_screen = true;
                                            } break;
                                            case 2: 
                                            {
                                                printf("Scatterbrain\n");
                                                personality_types_state = PERSONALITY_SCATTER_BRAIN;
                                                PushString(scenarios.personality, "Scatterbrain");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[CAVE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_CAVE);

                                                personality_results_screen = true;
                                            } break;
                                            case 3: 
                                            {
                                                printf("Narcissist\n");
                                                personality_types_state = PERSONALITY_NARCISSIST;
                                                PushString(scenarios.personality, "Narcissist");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[CAVE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_CAVE);

                                                personality_results_screen = true;
                                            } break;
                                            case 4: 
                                            {
                                                printf("Sore Loser\n");
                                                personality_types_state = PERSONALITY_SORE_LOSER;
                                                PushString(scenarios.personality, "Sore Loser");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[CAVE_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_CAVE);

                                                personality_results_screen = true;
                                            } break;
                                        } 
                                    }

                                    if (scenarios.result & SCENARIO_DESERT)
                                    {
                                        switch (scenarios.scenario[DESERT_INDEX].index)
                                        {
                                            case 0: 
                                            {
                                                personality_types_state = PERSONALITY_THUG;
                                                PushString(scenarios.personality, "Thug");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[DESERT_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_DESERT);

                                                personality_results_screen = true;
                                            } break;
                                            case 1: 
                                            {
                                                printf("Daredevil\n");
                                                personality_types_state = PERSONALITY_DAREDEVIL;
                                                PushString(scenarios.personality, "Daredevil");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[DESERT_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_DESERT);

                                                personality_results_screen = true;
                                            } break;
                                            case 2: 
                                            {
                                                printf("Idealist\n");
                                                personality_types_state = PERSONALITY_IDEALIST;
                                                PushString(scenarios.personality, "Idealist");
                                                scenarios.load_results = true;
                                                
                                                scenarios.scenario[DESERT_INDEX].is_active = false;
                                                is_personality_test = false;
                                                scenarios.result &= ~(SCENARIO_DESERT);

                                                personality_results_screen = true;
                                            } break;
                                        } 
                                    }
                                }
                                    
                                if (scenarios.result & SCENARIO_TOWER)
                                {
                                    switch (scenarios.scenario[TOWER_INDEX].index)
                                    {
                                        case 0: 
                                        {
                                            printf("Daydreamer\n");
                                            personality_types_state = PERSONALITY_DAYDREAMER;
                                            PushString(scenarios.personality, "Daydreamer");
                                            scenarios.load_results = true;
                                            
                                            scenarios.scenario[TOWER_INDEX].is_active = false;
                                            is_personality_test = false;
                                            scenarios.result &= ~(SCENARIO_TOWER);

                                            personality_results_screen = true;
                                        } break;
                                        case 1: 
                                        {
                                            printf("Socialite\n");
                                            personality_types_state = PERSONALITY_SOCIALITE;
                                            PushString(scenarios.personality, "Socialite");
                                            scenarios.load_results = true;
                                            
                                            scenarios.scenario[TOWER_INDEX].is_active = false;
                                            is_personality_test = false;
                                            scenarios.result &= ~(SCENARIO_TOWER);

                                            personality_results_screen = true;
                                        } break;
                                    }
                                }
                               
                                if (scenarios.result & SCENARIO_THEATER)
                                {
                                    switch (scenarios.scenario[THEATER_INDEX].index)
                                    {
                                        case 0: 
                                        {
                                            personality_types_state = PERSONALITY_FREE_SPIRIT;
                                            PushString(scenarios.personality, "Free Spirit");
                                            scenarios.load_results = true;
                                            
                                            scenarios.scenario[THEATER_INDEX].is_active = false;
                                            is_personality_test = false;
                                            scenarios.result &= ~(SCENARIO_THEATER);

                                            personality_results_screen = true;
                                        } break;
                                        case 1: 
                                        {
                                            personality_types_state = PERSONALITY_CRYBABY;
                                            PushString(scenarios.personality, "Crybaby");
                                            scenarios.load_results = true;
                                            
                                            scenarios.scenario[THEATER_INDEX].is_active = false;
                                            is_personality_test = false;
                                            scenarios.result &= ~(SCENARIO_THEATER);

                                            personality_results_screen = true;
                                        } break;
                                        case 2: 
                                        {
                                            personality_types_state = PERSONALITY_LONE_WOLF;
                                            PushString(scenarios.personality, "Lone Wolf");
                                            scenarios.load_results = true;
                                            
                                            scenarios.scenario[THEATER_INDEX].is_active = false;
                                            is_personality_test = false;
                                            scenarios.result &= ~(SCENARIO_THEATER);

                                            personality_results_screen = true;
                                        } break;
                                        case 3: 
                                        {
                                            personality_types_state = PERSONALITY_LOUT;
                                            PushString(scenarios.personality, "Lout");
                                            scenarios.load_results = true;
                                            
                                            scenarios.scenario[THEATER_INDEX].is_active = false;
                                            is_personality_test = false;
                                            scenarios.result &= ~(SCENARIO_THEATER);

                                            personality_results_screen = true;
                                        } break;
                                    }
                                }

                                if (scenarios.result & SCENARIO_FOREST)
                                {
                                    // Setup level
                                    /*SetAssetPosition(&character_data.model, 128 + (16 * 16), 240 - 24);
                                    SetAssetPosition(&oldman_asset, 128 + (16 * 18), 240 - 24);
                                    SetAssetPosition(&boulder_asset, 128 - (16 * 4), 240 - 24);
                                    SetAssetPosition(&dialogue_box_asset, SCREEN_CENTER_X, SCREEN_CENTER_Y);
                                    SetAssetPosition(&boulder_wall_asset[0], 128 + (16 * 20), 240);
                                    SetAssetPosition(&boulder_wall_asset[1], 128 + (16 * 20), 240 - 24);
                                    SetAssetPosition(&boulder_wall_asset[2], 128 + (16 * 20), 240 - 48);
                                    SetAssetPosition(&boulder_wall_asset[3], 128 + (16 * 19), 240 - 12);
                                    SetAssetPosition(&boulder_wall_asset[4], 128 + (16 * 19), 240 - 36);

                                    SetAssetAdjacentHitBoxes(&oldman_asset);*/
                                            
                                    //scenarios.result &= ~(SCENARIO_FOREST);
                                }

                                if (scenarios.scenario[FOREST_INDEX].is_active)
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
                                            character_data.model.conditions.has_movement = true;
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
                                            character_data.model.conditions.has_movement = true; 
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
                                                printf("personality next\n");
                                                Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                                personality_results_screen = false;
                                                next_button.is_active = false;
                                                is_name_submission = true;
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
                                                printf("overview next\n");

                                                // Initialize/Reset the player starting here
                                                SetAssetPosition(&character_data.model, 
                                                                 player_starting_coords.x,
                                                                 player_starting_coords.y);


                                                Sound_PlaySFX(&master_volume.sfx[1]->wav);
                                                is_class_overview_screen = false;
                                                next_button.is_active = false;
                                                is_game_running = true;
                                            } break;
                                        }
                                    }
                                }

                                if (is_game_running)
                                {
                                    for (int i = 0; i < total_floor_tiles; ++i)
                                    {
                                        if (map_rooms[0].floor_tiles[i].collision_tiles.conditions.is_occupied)
                                        {
                                            if (AABB_Detection(&character_data.model.body, &map_rooms[0].floor_tiles[i].collision_tiles.body))
                                            {
                                                printf("Item!\n");
                                                touch_item_on_floor = true;
                                            }
                                        }
                                    }

                                    

                                    if (game_command_menu_items.is_opened)
                                    {
                                        if (game_command_menu_items.slots[game_command_menu_items.index].occupied)
                                        {
                                            printf("slot index: %d\n", game_command_menu_items.index);
                                            for (int i = 0; i < COMMAND_MENU_ITEMS_SLOT_COUNT; ++i)
                                            {
                                                if (i == game_command_menu_items.index)
                                                {
                                                    item_slot_option = true;
                                                    item_slot_option_has_movement = true;

                                                    game_command_menu_items.has_movement = false;
                                                    game_command_menu_items.is_active = false;
                                                    game_command_menu.is_active = false;
                                                    printf("opening slot\n");
                                                    break;
                                                }
                                            }
                                   
                                            if (item_slot_option)
                                            {
                                                switch (item_slot_option_index)
                                                {
                                                    case 0:
                                                    {
                                                        // Apply item then close item option
                                                        /*for (int i = 0; i < 2; ++i)
                                                        {
                                                            if (game_items[i].count > 0 &&
                                                                game_items[i].type == ITEM_TYPE)
                                                            {
                                                                game_counti].count--;
                                                            }
                                                        }*/


                                                        printf("Use\n");
                                                    } break;
                                                    case 1:
                                                    {
                                                        // Toss item then close item option
                                                        game_command_menu.is_active = true;
                                                        game_command_menu_items.has_movement = true;
                                                        game_command_menu_items.is_active = true;

                                                        item_slot_option_has_movement = false;
                                                        item_slot_option = false;
                                                    
                                                        printf("Move\n");
                                                    } break;
                                                    case 2:
                                                    {
                                                        if (game_command_menu_items.slots[game_command_menu_items.index].occupied)
                                                        {
                                                            if (game_items[ITEM_HEALTH_POTION].count > 0)
                                                                game_items[ITEM_HEALTH_POTION].count--;
                                                            
                                                            if (game_items[ITEM_MANA_POTION].count > 0)
                                                                game_items[ITEM_MANA_POTION].count--;

                                                            printf("Toss\n");
                                                        }
                                                    } break;
                                                    default:
                                                    {

                                                    } break;
                                                }
                                            }
                                        }
                                    }

                                    if (game_command_menu.is_opened && game_command_menu.is_active)
                                    {
                                        switch (game_command_menu.index)
                                        {
                                            case 0:
                                            {
                                                game_command_menu_items.is_opened = true;
                                                game_command_menu_items.has_movement = true;
                                                game_command_menu.has_movement = false;
                                            } break;
                                            case 1:
                                            {
                                                printf("EQUIP\n");
                                                game_command_menu.has_movement = false;
                                            } break;
                                            case 2:
                                            {
                                                printf("STATUS\n");
                                            } break;
                                            default:
                                            {

                                            } break;
                                        }
                                    }

                                }
                            } break;
                            case SDLK_r:
                            {
                                //if (game_command_menu.is_opened && game_command_menu.is_active)
                                {
                                    if (game_command_menu_items.slots[game_command_menu_items.index].occupied)
                                    {
                                        // Test to add health potions
                                        if (game_items[game_command_menu_items.index].count < ITEM_STACK_MAX)
                                            game_items[game_command_menu_items.index].count++;
                                        printf("health potion count: %d\n", game_items[game_command_menu_items.index].count);

                                    }
                                }
                                
                            } break;
                            case SDLK_t:
                            {
                                if (is_game_running)
                                {
                                    if (enemy_card_stack.index < MAX_CARD_STACK)
                                        AssetCard_Push(&enemy_card_stack, enemy_card_mock);
                                }
                            } break;
                            case SDLK_y:
                            {
                                if (is_game_running)
                                {
                                    if (enemy_card_stack.index > 0)
                                        AssetCard_Pop(&enemy_card_stack);
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
            GUI_UpdateCursor(&cursor[RIGHT_CURSOR].model,
                             title_screen_options[option_index].x,
                             title_screen_options[option_index].y,
                             -4, -1);
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

            SDL_RenderClear(SDLWindow.Renderer);
            SDL_RenderCopy(SDLWindow.Renderer, title_screen_asset.texture, NULL, &title_screen_asset.body);
            GUI_RenderCursor(&cursor[RIGHT_CURSOR].model);
            for (i32 i = 0; i < ArraySize(title_screen_options); ++i)
            {
                RenderText(SDLWindow.Renderer, font.atlas, 
                           title_screen_options[i].x, 
                           title_screen_options[i].y,
                           title_screen_options[i].text, 
                           color.white);
            }
        }
    
        if (is_settings) 
        {
            SDL_RenderClear(SDLWindow.Renderer);
            SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
            CursorForItems(&sound_settings.options[sound_settings.index], &cursor[RIGHT_CURSOR].model, 6, 1);
            RenderAssetInWorldSpace(&cursor[RIGHT_CURSOR].model);
           
            RenderText(SDLWindow.Renderer, font.atlas,
                           SET_TEXT_CENTER_X("Sound Settings", 0), 
                           SCREEN_CENTER_Y - 64,
                           "Sound Settings", 
                           color.white);


            for (i32 i = 0; i < ArraySize(sound_settings.options); ++i) // Ignore rendering the apply button 
            {
                RenderText(SDLWindow.Renderer, font.atlas,
                           sound_settings.options[i].x, 
                           sound_settings.options[i].y,
                           sound_settings.options[i].text, 
                           color.white);
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
                        for (i32 i = 0; i < MUSIC_INDEX; ++i)
                        {
                            master_volume.music[MASTER_INDEX]->wav.volume = test_volume_controller.info[MASTER_INDEX].volume;
                        }
                          
                        for (i32 i = 0; i < SFX_INDEX; ++i)
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
                                for (i32 i = 0; i < MUSIC_FILE_COUNT; ++i)
                                {
                                    master_volume.music[i].wav.volume = volume_controller[0].volume;
                                    printf("master_music: %d\n", music_volume[i].wav.volume);

                                    if (music_volume[i].wav.volume > master_volume.music[i].wav.volume)
                                    {
                                        master_volume.music[i].wav.volume = master_volume.music[i].wav.volume;
                                    }
                                }

                                for (i32 i = 0; i < SFX_FILE_COUNT; ++i)
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
                SDL_RenderClear(SDLWindow.Renderer);
                SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);

                RenderText(SDLWindow.Renderer, font.atlas, 
                           SET_TEXT_CENTER_X("Back", -94),
                           SCREEN_CENTER_Y - 96,
                           "Back",
                           color.white);

                RenderText(SDLWindow.Renderer, font.atlas, 
                           SET_TEXT_CENTER_X("[ESC]", -64),
                           SCREEN_CENTER_Y - 96,
                           "[ESC]",
                           color.orange);

                for (i32 i = 0; i < ArraySize(character_creation_screen.info); ++i)
                {
                    RenderText(SDLWindow.Renderer, font.atlas, character_creation_screen.info[i].asset.x - 8, character_creation_screen.info[i].asset.y - 16, 
                               character_creation_screen.info[i].name, color.white);
                    RenderWrappedText(SDLWindow.Renderer, font.atlas, 
                                              character_creation_screen.info[character_creation_screen.index].description, color.white, 
                                              character_creation_screen.description_box.x + 2,
                                              character_creation_screen.description_box.y + 8, // containerX, containerY
                                              character_creation_screen.description_box.w, 
                                              character_creation_screen.description_box.h,    // containerW, containerH
                                              4);          // lineSpacing

                    RenderAssetInWorldSpace(&character_creation_screen.info[i].asset);
                }
                
                CursorForAssets(&character_creation_screen.info[character_creation_screen.index].asset, &cursor[UP_CURSOR].model, 4, 16);
                RenderAssetInWorldSpace(&cursor[UP_CURSOR].model);

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
                            character_data.model.conditions.has_physics = true;
                            character_data.model.conditions.has_movement = true;
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
                    GUI_UpdateCursor(&cursor[RIGHT_CURSOR].model, 
                                  confirmation.button[confirmation.index].pos.x,
                                  confirmation.button[confirmation.index].pos.y, 
                                  -4, -1);
                    ConfirmationButtons_RenderBox(&confirmation);
                    ConfirmationButtons_RenderText(&confirmation);
                    GUI_RenderCursor(&cursor[RIGHT_CURSOR].model);
                }

            }

            if (character_allocation_select_screen.is_active)
            {
                SDL_RenderClear(SDLWindow.Renderer);
                SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                          
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

                RenderText(SDLWindow.Renderer, font.atlas, 
                           SET_TEXT_CENTER_X("Back", -94),
                           SCREEN_CENTER_Y - 96,
                           "Back",
                           color.white);
                RenderText(SDLWindow.Renderer, font.atlas, 
                           SET_TEXT_CENTER_X("[ESC]", -64),
                           SCREEN_CENTER_Y - 96,
                           "[ESC]",
                           color.orange);

                RenderText(SDLWindow.Renderer, font.atlas,
                           SET_TEXT_CENTER_X("How will you allocate your", 0),
                           SCREEN_CENTER_Y - 48,
                           "How will you allocate your",
                           color.white);
                RenderText(SDLWindow.Renderer, font.atlas,
                           SET_TEXT_CENTER_X("points for your character?", 0),
                           SCREEN_CENTER_Y - 40,
                           "points for your character?",
                           color.white);
    
                for (i32 i = 0; i < ArraySize(character_allocation_select_screen.info); ++i)
                {
                    RenderAssetInWorldSpace(&character_allocation_select_screen.info[i].asset);
                    RenderTextWithNewlines(SDLWindow.Renderer, font.atlas, 
                               character_allocation_select_screen.info[i].asset.x, 
                               character_allocation_select_screen.info[i].asset.y, 
                               character_allocation_select_screen.info[i].name, 
                               color.white, 
                               2);
    
                    RenderWrappedText(SDLWindow.Renderer, font.atlas, 
                                      character_allocation_select_screen.info[character_allocation_select_screen.index].description, color.white, 
                                      character_allocation_select_screen.description_box.x + 2,
                                      character_allocation_select_screen.description_box.y + 8, // containerX, containerY
                                      character_allocation_select_screen.description_box.w, 
                                      character_allocation_select_screen.description_box.h,    // containerW, containerH
                                      4);          // lineSpacing

                }
                  
                CursorForAssets(&character_allocation_select_screen.info[character_allocation_select_screen.index].asset, &cursor[UP_CURSOR].model, 24, 20);
                RenderAssetInWorldSpace(&cursor[UP_CURSOR].model);
                
                SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(SDLWindow.Renderer, &character_allocation_select_screen.description_box);

                if (confirmation.is_active)
                {
                
                    GUI_UpdateCursor(&cursor[RIGHT_CURSOR].model, 
                                  confirmation.button[confirmation.index].pos.x,
                                  confirmation.button[confirmation.index].pos.y, 
                                  -4, -1);
                    ConfirmationButtons_RenderBox(&confirmation);
                    ConfirmationButtons_RenderText(&confirmation);
                    GUI_RenderCursor(&cursor[RIGHT_CURSOR].model);

                    for (i32 i = 0; i < ArraySize(confirmation.button); ++i) 
                    {
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
                        
                        RenderText(SDLWindow.Renderer, font.atlas,
                                   confirmation.button[i].pos.x, 
                                   confirmation.button[i].pos.y,
                                   confirmation.button[i].text, 
                                   color.white);
                    }
                }
            }

        
            // Character name is the last screen where input will be entering it in letter by letter like FF8 or pokemon   
            if (is_personality_test)
            {
                SDL_RenderClear(SDLWindow.Renderer);
                SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);

                if (personality_test.is_active)
                {
                    switch (personality_scenario_result_state)
                    {
                        case PERSONALITY_SCENARIO_RESULT_NONE:
                        {
                            confirmation.is_active = true;
                        } break;
                        case PERSONALITY_SCENARIO_RESULT_VILLAGE:
                        {
                            scenario_name = "Village Scenario";
                            
                            scenarios.scenario[VILLAGE_INDEX].is_active = true;
                            scenarios.result |= SCENARIO_VILLAGE;
                            personality_test.is_active = false;
                        } break;
                        case PERSONALITY_SCENARIO_RESULT_MONSTER:
                        {
                            scenario_name = "Monster Scenario";
                           
                            scenarios.scenario[MONSTER_INDEX].is_active = true;
                            scenarios.result |= SCENARIO_MONSTER;
                            personality_test.is_active = false;
                        } break;
                        case PERSONALITY_SCENARIO_RESULT_FOREST:
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
                                    
                            SetAssetAdjacentHitBoxes(&oldman_asset);
                           
                            scenarios.scenario[FOREST_INDEX].is_active = true;
                            personality_test.is_active = false;
                        } break;
                        case PERSONALITY_SCENARIO_RESULT_CAVE:
                        {
                            scenario_name = "Cave Scenario";
                           
                            scenarios.scenario[CAVE_INDEX].is_active = true;
                            scenarios.result |= SCENARIO_CAVE;
                            personality_test.is_active = false;
                        } break;
                        case PERSONALITY_SCENARIO_RESULT_DESERT:
                        {
                            scenario_name = "Desert Scenario";
                           
                            scenarios.scenario[DESERT_INDEX].is_active = true;
                            scenarios.result |= SCENARIO_DESERT;
                            personality_test.is_active = false;
                        } break;
                        case PERSONALITY_SCENARIO_RESULT_TOWER:
                        {
                            scenario_name = "Tower Scenario";
                          
                            scenarios.scenario[TOWER_INDEX].is_active = true;
                            scenarios.result |= SCENARIO_TOWER;
                            personality_test.is_active = false;
                        } break;
                        case PERSONALITY_SCENARIO_RESULT_THEATER:
                        {
                            scenario_name = "Theater Scenario";
                           
                            scenarios.scenario[THEATER_INDEX].is_active = true;
                            scenarios.result |= SCENARIO_THEATER;
                            personality_test.is_active = false;
                        } break;
                        default:
                        {
                            personality_scenario_result_state = PERSONALITY_SCENARIO_RESULT_NONE;
                        } break;
                    }
                }

                Personality_BeginQuestions(&personality_test); 
                RenderWrappedTextCentered(SDLWindow.Renderer, font.atlas, 
                                      personality_test.table[personality_test.index], color.white, 
                                      0, -32,        // containerX, containerY
                                      256, 240,    // containerW, containerH
                                      2);          // lineSpacing


                if (confirmation.is_active)
                {
                    GUI_UpdateCursor(&cursor[RIGHT_CURSOR].model, 
                                  confirmation.button[confirmation.index].pos.x,
                                  confirmation.button[confirmation.index].pos.y, 
                                  -4, -1);
                    ConfirmationButtons_RenderBox(&confirmation);
                    ConfirmationButtons_RenderText(&confirmation);
                    GUI_RenderCursor(&cursor[RIGHT_CURSOR].model);
                }

                if (scenarios.scenario[VILLAGE_INDEX].is_active)
                {
                    SDL_RenderClear(SDLWindow.Renderer);
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   color.white);

                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("Silver coins drop from the hanging bag", 0), 
                               SCREEN_CENTER_Y - 88,
                               "Silver coins drop from the hanging bag", 
                               color.white); 
                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X(" of an elderly man's pocket as he walks", 0), 
                               SCREEN_CENTER_Y - 80,
                               " of an elderly man's pocket as he walks", 
                               color.white);

                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X(" through the market. What do you do?", 0), 
                               SCREEN_CENTER_Y - 72,
                               " through the market. What do you do?", 
                               color.white);

                    CursorForItems(&scenarios.scenario[VILLAGE_INDEX].options[scenarios.scenario[VILLAGE_INDEX].index], &cursor[RIGHT_CURSOR].model, 4, 1);
                    RenderAssetInWorldSpace(&cursor[RIGHT_CURSOR].model);

                    for (i32 i = 0; i < VILLAGE_OPTION_COUNT; ++i)
                    {
                        RenderTextWithNewlines(SDLWindow.Renderer, font.atlas,
                                               scenarios.scenario[VILLAGE_INDEX].options[i].x,
                                               scenarios.scenario[VILLAGE_INDEX].options[i].y,
                                               scenarios.scenario[VILLAGE_INDEX].options[i].text,
                                               color.white,
                                               2);
                    }
                     

                }

                if (scenarios.scenario[MONSTER_INDEX].is_active)
                {
                    SDL_RenderClear(SDLWindow.Renderer);
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   color.white);

                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X("You are a man by day and a beast by", 0), 
                                   SCREEN_CENTER_Y - 88,
                                   "You are a man by day and a beast by", 
                                   color.white);                   

                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X("night. You prey off human flesh and blood", 0), 
                                   SCREEN_CENTER_Y - 80,
                                   "night. You prey off human flesh and blood", 
                                   color.white);  

                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X(" to survive. You come across a small and", 0), 
                                   SCREEN_CENTER_Y - 72,
                                   " to survive. You come across a small and", 
                                   color.white);

                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X(" quiet village. What do you do?", 0), 
                                   SCREEN_CENTER_Y - 64,
                                   " quiet village. What do you do?", 
                                   color.white);
 
                    scenarios.scenario[MONSTER_INDEX].box.x = SCREEN_CENTER_X - 124;
                    scenarios.scenario[MONSTER_INDEX].box.y = SCREEN_CENTER_Y - 96;
                    scenarios.scenario[MONSTER_INDEX].box.w = 240;
                    scenarios.scenario[MONSTER_INDEX].box.h = 64;

                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(SDLWindow.Renderer, &scenarios.scenario[MONSTER_INDEX].box);



                    CursorForItems(&scenarios.scenario[MONSTER_INDEX].options[scenarios.scenario[MONSTER_INDEX].index], &cursor[RIGHT_CURSOR].model, 4, 1);
                    RenderAssetInWorldSpace(&cursor[RIGHT_CURSOR].model);

                    for (i32 i = 0; i < MONSTER_OPTION_COUNT; ++i)
                    {
                        RenderTextWithNewlines(SDLWindow.Renderer, font.atlas,
                                               scenarios.scenario[MONSTER_INDEX].options[i].x,
                                               scenarios.scenario[MONSTER_INDEX].options[i].y,
                                               scenarios.scenario[MONSTER_INDEX].options[i].text,
                                               color.white,
                                               2);
                    }

                }

                if (scenarios.scenario[FOREST_INDEX].is_active)
                {
                    if (!first_forest_entrance)
                    {
                        hold_dialogue_box = true;
                        first_forest_entrance = true;
                    }

                    if (UpdateAssetMovement(&character_data.model, &sfx_move))
                    {
                        UpdatePushableAsset(&boulder_asset, &character_data.model); 
                    }

                    for (i32 i = 0; i < ArraySize(forest_scenario_walls); ++i)
                    {
                        AABB_Resolution(&character_data.model, &forest_scenario_walls[i]);
                        AABB_Resolution(&boulder_asset, &forest_scenario_walls[i]);
                    }
                    
                    for (i32 i = 0; i < ArraySize(boulder_wall_asset); ++i)
                    {
                        AABB_Resolution(&character_data.model, &boulder_wall_asset[i]);
                        AABB_Resolution(&boulder_asset, &boulder_wall_asset[i]);
                    }

                    AABB_Resolution(&character_data.model, &boulder_asset);
                    AABB_Resolution(&character_data.model, &oldman_asset);

                    // Boulder going out of bounds counts as ending the scene too
                    if (!player_is_out_of_bounds)
                    {
                        for (i32 i = 0; i < ArraySize(forest_out_of_bounds); ++i)
                        {
                            if (AABB_Detection(&character_data.model.body, &forest_out_of_bounds[i].body) ||
                                AABB_Detection(&boulder_asset.body, &forest_out_of_bounds[i].body))
                            {
                                printf("out of bounds!\n");
                                
                                if (boulder_count <= 1)
                                {
                                    personality_types_state = PERSONALITY_LAZYBONES;
                                    PushString(scenarios.personality, "Lazybones");
                                    scenarios.load_results = true;
                                    
                                    // Set character's x,y coords for when the game loop starts, player
                                    // is left at the position when leaving the forest scenario
                                    character_data.model.body.x = 10 * 16;
                                    character_data.model.body.y = 16 * 24;

                                    scenarios.scenario[FOREST_INDEX].is_active = false;
                                    is_personality_test = false;
                                    personality_results_screen = true;
                                }

                                if (boulder_count <= 5 && boulder_count >= 2)
                                {
                                    personality_types_state = PERSONALITY_SHOW_OFF;
                                    PushString(scenarios.personality, "Show-Off");
                                    scenarios.load_results = true;
                                    
                                    character_data.model.body.x = 10 * 16;
                                    character_data.model.body.y = 16 * 24;
                                    
                                    scenarios.scenario[FOREST_INDEX].is_active = false;
                                    is_personality_test = false;
                                    personality_results_screen = true;
                                }
                                
                                if (boulder_count <= 19 && boulder_count >= 6)
                                {
                                    personality_types_state = PERSONALITY_PLUGGER;
                                    PushString(scenarios.personality, "Plugger");
                                    scenarios.load_results = true;
                                    
                                    character_data.model.body.x = 10 * 16;
                                    character_data.model.body.y = 16 * 24;

                                    scenarios.scenario[FOREST_INDEX].is_active = false;
                                    is_personality_test = false;
                                    personality_results_screen = true;
                                }

                                if (boulder_count <= 39 && boulder_count >= 20)
                                {
                                    personality_types_state = PERSONALITY_DRUDGE;
                                    PushString(scenarios.personality, "Drudge");
                                    scenarios.load_results = true;
                                    
                                    character_data.model.body.x = 10 * 16;
                                    character_data.model.body.y = 16 * 24;

                                    scenarios.scenario[FOREST_INDEX].is_active = false;
                                    is_personality_test = false;
                                    personality_results_screen = true;
                                }

                                if (boulder_count >= 40)
                                {
                                    personality_types_state = PERSONALITY_TOUGH_COOKIE;
                                    PushString(scenarios.personality, "Tough-Cookie");
                                    scenarios.load_results = true;
                                    
                                    character_data.model.body.x = 10 * 16;
                                    character_data.model.body.y = 16 * 24;

                                    scenarios.scenario[FOREST_INDEX].is_active = false;
                                    is_personality_test = false;
                                    personality_results_screen = true;
                                }

                                player_is_out_of_bounds = true;
                            }
                        }
                    }

                    if (!player_talking_to_oldman)
                    {
                        for (i32 i = 0; i < ArraySize(oldman_asset.adjacent_hitboxes); ++i)
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
                    
                    for (i32 i = 0; i < ArraySize(forest_scenario_walls); ++i)
                        RenderAssetInWorldSpace(&forest_scenario_walls[i]);
                    
                    for (i32 i = 0; i < ArraySize(forest_out_of_bounds); ++i)
                        RenderAssetInWorldSpace(&forest_out_of_bounds[i]);

                    RenderAssetInWorldSpace(&forest_scenario_map); 
                    RenderAssetInWorldSpace(&forest_scenario_finish_line);
  
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
                                             SCREEN_CENTER_X + 88, SCREEN_CENTER_Y - 105); 

                        RenderText(SDLWindow.Renderer, font.atlas, 
                               SET_TEXT_CENTER_X(gold_coins, 104), SCREEN_CENTER_Y - 95,
                               gold_coins,
                               color.green);
                    }
                    

                    if (hold_dialogue_box)
                    {
                        character_data.model.conditions.has_movement = false; 
                        RenderAssetInCameraSpace(&dialogue_box_asset, 
                                             SCREEN_CENTER_X - (128 - 32), SCREEN_CENTER_Y + 32); 
                        RenderTextWithNewlines(SDLWindow.Renderer, font.atlas,
                                   SCREEN_CENTER_X - (128 - 56), 
                                   SCREEN_CENTER_Y + 56,
                                   forest_scenario_dialogue_intro[dialogue_index], 
                                   color.green, 
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
                        character_data.model.conditions.has_movement = false; 
                        RenderAssetInCameraSpace(&dialogue_box_asset, 
                                             SCREEN_CENTER_X - (128 - 32), SCREEN_CENTER_Y + 32); 
                            
                        RenderTextWithNewlines(SDLWindow.Renderer, font.atlas,
                                   SCREEN_CENTER_X - (128 - 56), 
                                   SCREEN_CENTER_Y + 56,
                                   forest_scenario_dialogue_return_the_boulder[dialogue_index_2], 
                                   color.green, 
                                   2);
                    }

                    RenderAssetInWorldSpace(&character_data.model);
                    RenderAssetInWorldSpace(&oldman_asset);
                    RenderAssetInWorldSpace(&boulder_asset);

                    for (i32 i = 0; i < ArraySize(boulder_wall_asset); ++i)
                        RenderAssetInWorldSpace(&boulder_wall_asset[i]);

                }


                if (scenarios.scenario[CAVE_INDEX].is_active)
                {
                    SDL_RenderClear(SDLWindow.Renderer);
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   color.white);

                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X("You are near the end of your quest in", 0), 
                                   SCREEN_CENTER_Y - 88,
                                   "You are near the end of your quest in ", 
                                   color.white); 
                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X("saving the princess, where the entrance", 0), 
                                   SCREEN_CENTER_Y - 80,
                                   "saving the princess, where the entrance ", 
                                   color.white); 
                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X("to her prison is in front of you. But two", 0), 
                                   SCREEN_CENTER_Y - 72,
                                   "to her prison is in front of you. But two doors", 
                                   color.white); 
                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X("doors fork to the left and right, a", 0), 
                                   SCREEN_CENTER_Y - 64,
                                   "doors fork to the left and right, a", 
                                   color.white); 
                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X("door to take you deeper in and a door to", 0), 
                                   SCREEN_CENTER_Y - 56,
                                   "door to take you deeper in and a door to", 
                                   color.white); 
                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X("a room of treasures. What do you do?", 0), 
                                   SCREEN_CENTER_Y - 48,
                                   "a room of treasures. What do you do?", 
                                   color.white);

   
                    scenarios.scenario[CAVE_INDEX].box.x = SCREEN_CENTER_X - 126;
                    scenarios.scenario[CAVE_INDEX].box.y = SCREEN_CENTER_Y - 98;
                    scenarios.scenario[CAVE_INDEX].box.w = 250;
                    scenarios.scenario[CAVE_INDEX].box.h = 64;

                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(SDLWindow.Renderer, &scenarios.scenario[CAVE_INDEX].box);


                    CursorForItems(&scenarios.scenario[CAVE_INDEX].options[scenarios.scenario[CAVE_INDEX].index], &cursor[RIGHT_CURSOR].model, 4, 1);
                    RenderAssetInWorldSpace(&cursor[RIGHT_CURSOR].model);

                    for (i32 i = 0; i < CAVE_OPTION_COUNT; ++i)
                    {
                        RenderTextWithNewlines(SDLWindow.Renderer, font.atlas,
                                               scenarios.scenario[CAVE_INDEX].options[i].x,
                                               scenarios.scenario[CAVE_INDEX].options[i].y,
                                               scenarios.scenario[CAVE_INDEX].options[i].text,
                                               color.white,
                                               2);
                    }
                }

                if (scenarios.scenario[DESERT_INDEX].is_active)
                {
                    SDL_RenderClear(SDLWindow.Renderer);
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   color.white);

                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("You carry with yourself a canteen of", 0), 
                               SCREEN_CENTER_Y - 88,
                               "You carry with yourself a canteen of", 
                               color.white);           
                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("water with only a few sips worth left.", 0), 
                               SCREEN_CENTER_Y - 80,
                               "water with only a few sips worth left.", 
                               color.white);
                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("In the harsh and unforgiving desert,", 0), 
                               SCREEN_CENTER_Y - 72,
                               "In the harsh and unforgiving desert", 
                               color.white);           
                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("you come across two men stranded, where", 0), 
                               SCREEN_CENTER_Y - 64,
                               "you come across two men stranded, where", 
                               color.white);
                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("one is near death from thirst.", 0), 
                               SCREEN_CENTER_Y - 56,
                               "one is near death from thirst.", 
                               color.white);
                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("What do you do?", 0), 
                               SCREEN_CENTER_Y - 48,
                               "What do you do?", 
                               color.white);

                    scenarios.scenario[DESERT_INDEX].box.x = SCREEN_CENTER_X - 126;
                    scenarios.scenario[DESERT_INDEX].box.y = SCREEN_CENTER_Y - 98;
                    scenarios.scenario[DESERT_INDEX].box.w = 250;
                    scenarios.scenario[DESERT_INDEX].box.h = 64;
                    
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(SDLWindow.Renderer, &scenarios.scenario[DESERT_INDEX].box);

                    CursorForItems(&scenarios.scenario[DESERT_INDEX].options[scenarios.scenario[DESERT_INDEX].index], &cursor[RIGHT_CURSOR].model, 4, 1);
                    RenderAssetInWorldSpace(&cursor[RIGHT_CURSOR].model);

                    for (i32 i = 0; i < DESERT_OPTION_COUNT; ++i)
                    {
                        RenderTextWithNewlines(SDLWindow.Renderer, font.atlas,
                                               scenarios.scenario[DESERT_INDEX].options[i].x,
                                               scenarios.scenario[DESERT_INDEX].options[i].y,
                                               scenarios.scenario[DESERT_INDEX].options[i].text,
                                               color.white,
                                               2);
                    }
                }

                if (scenarios.scenario[TOWER_INDEX].is_active)
                {
                    SDL_RenderClear(SDLWindow.Renderer);
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   color.white);

                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("You are disoriented and awake at", 0), 
                               SCREEN_CENTER_Y - 88,
                               "You are disoriented and awake at", 
                               color.white);           
                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("the top of a seemingly endless tower.", 0), 
                               SCREEN_CENTER_Y - 80,
                               "the top of a seemingly endless tower.", 
                               color.white);  
                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("You see a staircase besides you that leads", 0), 
                               SCREEN_CENTER_Y - 72,
                               "You see a staircase besides you that leads", 
                               color.white);
                    RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("down to the unknown. What do you do?", 0), 
                               SCREEN_CENTER_Y - 64,
                               "down to the unknown. What do you do?", 
                               color.white);
                                        
                    scenarios.scenario[TOWER_INDEX].box.x = SCREEN_CENTER_X - 126;
                    scenarios.scenario[TOWER_INDEX].box.y = SCREEN_CENTER_Y - 98;
                    scenarios.scenario[TOWER_INDEX].box.w = 250;
                    scenarios.scenario[TOWER_INDEX].box.h = 64;
                    
                    SDL_SetRenderDrawColor(SDLWindow.Renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(SDLWindow.Renderer, &scenarios.scenario[TOWER_INDEX].box);
                    
                    CursorForItems(&scenarios.scenario[TOWER_INDEX].options[scenarios.scenario[TOWER_INDEX].index], &cursor[RIGHT_CURSOR].model, 4, 1);
                    RenderAssetInWorldSpace(&cursor[RIGHT_CURSOR].model);

                    for (i32 i = 0; i < TOWER_OPTION_COUNT; ++i)
                    {
                        RenderTextWithNewlines(SDLWindow.Renderer, font.atlas,
                                               scenarios.scenario[TOWER_INDEX].options[i].x,
                                               scenarios.scenario[TOWER_INDEX].options[i].y,
                                               scenarios.scenario[TOWER_INDEX].options[i].text,
                                               color.white,
                                               2);
                    }
                }

                if (scenarios.scenario[THEATER_INDEX].is_active)
                {
                    SDL_RenderClear(SDLWindow.Renderer);
                    SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                    RenderText(SDLWindow.Renderer, font.atlas,
                                   SET_TEXT_CENTER_X(scenario_name, 0), 
                                   SCREEN_CENTER_Y - 108,
                                   scenario_name, 
                                   color.white);
                    
                    for (int i = 0; i < 8; ++i)
                    {
                        RenderText(SDLWindow.Renderer, font.atlas,
                                   theater_scenario_results[i].x, 
                                   theater_scenario_results[i].y,
                                   theater_scenario_results[i].text, 
                                   color.white);
                    }

                    CursorForItems(&scenarios.scenario[THEATER_INDEX].options[scenarios.scenario[THEATER_INDEX].index], &cursor[RIGHT_CURSOR].model, 4, 1);
                    RenderAssetInWorldSpace(&cursor[RIGHT_CURSOR].model);

                    for (i32 i = 0; i < THEATER_OPTION_COUNT; ++i)
                    {
                        RenderTextWithNewlines(SDLWindow.Renderer, font.atlas,
                                               scenarios.scenario[THEATER_INDEX].options[i].x,
                                               scenarios.scenario[THEATER_INDEX].options[i].y,
                                               scenarios.scenario[THEATER_INDEX].options[i].text,
                                               color.white,
                                               2);
                    }

                }

            }
    
            if (personality_results_screen)
            {
                SDL_RenderClear(SDLWindow.Renderer);
                SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("Personality:", 0), 
                               SCREEN_CENTER_Y - 88,
                               "Personality:", 
                               color.white);


                personality_result = Personality_GetScenarioResults(&scenarios, personality_types_state);    

                // Resets the camera so it doesn't affect assets to render AFTER it's been used in a condition where it's then 
                // turned off relative to it still.
                SDLCamera.X = 0;
                SDLCamera.Y = 0;

                Personality_RenderScenarioResults(&personality_result, &font);

                CursorForItems(&next_button.button[next_button.index], &cursor[RIGHT_CURSOR].model, 4, 1);
                RenderAssetInWorldSpace(&cursor[RIGHT_CURSOR].model);
                for (i32 i = 0; i < ArraySize(next_button.button); ++i)
                {
                    RenderText(SDLWindow.Renderer, font.atlas,
                               next_button.button[i].x,
                               next_button.button[i].y,
                               next_button.button[i].text,
                               color.white);
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
                            is_name_submission = false;
                            is_class_overview_screen = true;
                        } break;
                    }
                }

                NameEntry_EnterGlyph(&name_entry, name_entry_bar, ascii_to_glyph_grid);
                NameEntry_DeleteGlyph(&name_entry, name_entry_bar);
                NameEntry_ConfirmName(&name_entry, name_entry_bar);

                SDL_RenderClear(SDLWindow.Renderer);
                SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
                
                NameEntry_RenderNameUnderline(name_entry_bar, SDLWindow.Renderer, font.atlas, color.white);
                NameEntry_RenderName(name_entry_bar, SDLWindow.Renderer, font.atlas, color.white);
            
                for (i32 i = 0; i < ArraySize(glyph_grid); ++i)
                {
                    RenderText(SDLWindow.Renderer, font.atlas,
                            glyph_grid[i].pos.x, 
                            glyph_grid[i].pos.y,
                            glyph_grid[i].glyph, 
                            color.white);

                }    
                
                RenderText(SDLWindow.Renderer, font.atlas,
                           SET_TEXT_CENTER_X("Enter your name", 0), 
                           SCREEN_CENTER_Y - 96,
                           "Enter your name", 
                           color.white);
                
                
                // Render after the glyph grid so it renders over rather than behind, looks decent but still considering a change
                Cursor(&cursor[RIGHT_CURSOR].model, &glyph_grid[name_entry.index].pos, -4, -1);
                RenderAssetInWorldSpace(&cursor[RIGHT_CURSOR].model);
                RenderAsset(&character_creation_screen.info[character_creation_screen.index].asset, 
                            SCREEN_CENTER_X - (16/2), 52,
                            16, 24);
            }
        }

        if (is_class_overview_screen)
        {
          
            static char full_text[10][28];
            static i32 full_len;
            static i32 color_count;
            static i32 non_colored_len;
            static i32 base_x_arr[10];
            static i32 base_y_arr[10];
            static i32 offset_x_arr[10];

            if (init_name)
            {
                char buffer[28];
                char *name = NameEntry_GetName(&name_entry);
                char *lvl = "1";
                char *class_name = character_data.class.name;
                char *class_personality = scenarios.personality;
                StatOverview_Init(buffer, class_status_overview[0].text, name, 27);
                StatOverview_Init(buffer, class_status_overview[1].text, lvl, 27);
                StatOverview_Init(buffer, class_status_overview[2].text, class_name, 27);
                StatOverview_Init(buffer, class_status_overview[3].text, class_personality, 27);

                sprintf(int_buffer[0], "%d", character_data.base_stats.strength);
                sprintf(int_buffer[1], "%d", character_data.base_stats.resilience);
                sprintf(int_buffer[2], "%d", character_data.base_stats.agility);
                sprintf(int_buffer[3], "%d", character_data.base_stats.stamina);
                sprintf(int_buffer[4], "%d", character_data.base_stats.wisdom);
                sprintf(int_buffer[5], "%d", character_data.base_stats.luck);
                sprintf(int_buffer[6], "%d", character_data.base_stats.max_hp);
                sprintf(int_buffer[7], "%d", character_data.base_stats.max_mp);
                
                StatOverview_Init(buffer, class_status_overview[4].text, int_buffer[0], 27);
                StatOverview_Init(buffer, class_status_overview[5].text, int_buffer[1], 27);
                StatOverview_Init(buffer, class_status_overview[6].text, int_buffer[2], 27);
                StatOverview_Init(buffer, class_status_overview[7].text, int_buffer[3], 27);
                StatOverview_Init(buffer, class_status_overview[8].text, int_buffer[4], 27);
                StatOverview_Init(buffer, class_status_overview[9].text, int_buffer[5], 27);
                StatOverview_Init(buffer, class_status_overview[10].text, int_buffer[6], 27);
                StatOverview_Init(buffer, class_status_overview[11].text, int_buffer[7], 27);
     
                full_len = strlen(class_status_overview[4].text);
                color_count = 3;
                non_colored_len = full_len - color_count;
                if (non_colored_len < 0)
                    non_colored_len = 0;     

                for (i32 i = 0, j = 4; i < 8 && j < 14; ++i, ++j)
                {
                    PushString(full_text[i], class_status_overview[j].text);

                    base_x_arr[i] = class_status_overview[j].pos.x;
                    base_y_arr[i] = class_status_overview[j].pos.y;

                    offset_x_arr[i] = base_x_arr[i] + non_colored_len * GLYPH_WIDTH;
                }

                init_name = false;
            }

            SDL_RenderClear(SDLWindow.Renderer);
            SDL_RenderCopy(SDLWindow.Renderer, blank_screen_asset.texture, NULL, &blank_screen_asset.body);
            CursorForItems(&next_button.button[next_button.index], &cursor[RIGHT_CURSOR].model, 4, 1);
            RenderAssetInWorldSpace(&cursor[RIGHT_CURSOR].model);
            for (i32 i = 0; i < ArraySize(next_button.button); ++i)
            {
                RenderText(SDLWindow.Renderer, font.atlas,
                           next_button.button[i].x,
                           next_button.button[i].y,
                           next_button.button[i].text,
                           color.white);
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
            
            for (i32 i = 0; i < ArraySize(class_status_overview); ++i)
            {
                RenderText(SDLWindow.Renderer, font.atlas,
                           class_status_overview[i].pos.x,
                           class_status_overview[i].pos.y,
                           class_status_overview[i].text,
                           color.white);
            }

            for (i32 i = 0; i < 10; ++i)
            {
                RenderText(SDLWindow.Renderer, font.atlas,
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
            UpdateAssetProperties(&character_data.model);
            UpdateAssetProperties(&enemy_los_asset);

            if (UpdateAssetMovement(&character_data.model, &sfx_move))
            {
                UpdateAssetNPCMovement(&enemy_los_asset, &sfx_move); 
                


            }

            AABB_AdjHitboxResolution(&character_data.model, &enemy_los_asset);
            AABB_LOSResolution(&character_data.model, &enemy_los_asset, &font);
            AABB_ChaseRadiusResolution(&enemy_los_asset, &character_data.model, &font);
            for (int i = 0; i < total_floor_tiles; ++i)
            {
               AABB_Resolution(&character_data.model, &map_rooms[0].floor_tiles[i].collision_tiles);
               AABB_Resolution(&enemy_los_asset, &map_rooms[0].floor_tiles[i].collision_tiles);
            } 


            GUI_UpdateCursor(&cursor[RIGHT_CURSOR].model, 
                             enemy_card_stack.asset[enemy_card_stack.index - 1].x,
                             enemy_card_stack.asset[enemy_card_stack.index - 1].y,
                             -4, -1);
            
            AttachCameraToPlayer(&character_data.model, &room_asset[0]);
            SDL_SetRenderTarget(SDLWindow.Renderer, SDLCamera.TargetTexture);
            SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
            SDL_RenderClear(SDLWindow.Renderer);

            RenderAssetInWorldSpace(&room_asset[0]);
            RenderAssetInWorldSpace(&down_stairs_asset);
         
            ///////////////////////////////////////////
            
            RenderAssetInCameraSpace(&player_card_mock, 
                                     SCREEN_CENTER_X - 120, 
                                     SCREEN_CENTER_Y - 112);
    

            // Only render together
            AssetCard_Render(&enemy_card_stack);
            GUI_RenderCursor(&cursor[RIGHT_CURSOR].model);


            for (int i = 0; i < total_floor_tiles; ++i)
            {
                if (map_rooms[0].floor_tiles[i].collision_tiles.conditions.is_occupied)
                {
                    RenderAssetInWorldSpaceWithCoords(&tile_sparkle, 
                                                      map_rooms[0].floor_tiles[i].collision_tiles.body.x,
                                                      map_rooms[0].floor_tiles[i].collision_tiles.body.y);
                }
            }

            RenderAssetInWorldSpace(&enemy_los_asset);
            RenderAssetInWorldSpace(&character_data.model);

            if (touch_item_on_floor)
            {
                RenderAssetInCameraSpace(&loot_box, 
                                         SCREEN_CENTER_X - (loot_box.w / 2), 
                                         SCREEN_CENTER_Y - (loot_box.h / 2));


            }

            if (game_command_menu.is_opened)
            {
                RenderAssetInCameraSpace(&game_command_menu.box, 
                                         SCREEN_CENTER_X + 32, (SCREEN_CENTER_Y - (game_command_menu.box.h / 2) - 16));

                // Option box rendered is relative to the option hovered by using the index
                RenderAssetInCameraSpace(&game_command_menu.option_box[game_command_menu.index], 
                                         SCREEN_CENTER_X - 112, (SCREEN_CENTER_Y - (game_command_menu.option_box[game_command_menu.index].h / 2) - 16));

/*
                if (game_command_menu_items.slots[0].occupied)
                {
                    game_command_menu_items.slots[0].rarity.slot = equipment[0].rarity.slot;
                }
                else
                {
                    game_command_menu_items.slots[0].rarity.slot = slot_default;
                }
*/

                // Items, Equip and Status
                switch (game_command_menu.index)
                {
                    case 0: // Items
                    {
                        static int inventory_count = 0;
                        int counter = 0;
                        for (int i = 0; i < COMMAND_MENU_ITEMS_SLOT_COUNT; ++i)
                        {
                            if (game_command_menu_items.slots[i].occupied)
                            {
                                counter++;
                            }
                        }

                        inventory_count = counter;

                        for (int i = 0; i < COMMAND_MENU_ITEMS_SLOT_COUNT; ++i)
                        {
                            RenderAssetInCameraSpace(&game_command_menu_items.slots[i].model, 
                                                     game_command_menu_items.slots[i].model.x, 
                                                     game_command_menu_items.slots[i].model.y);
                        }

                        // Render asset(s) inside slot
                        // TODO: Iterate over a current count of items in inventory that updates on add or removal
                        // of an item on a slot, stacking does not count towards it
                        for (int i = 0; i < inventory_count; ++i)
                        {
                            RenderAssetInCameraSpace(&game_items[i].asset,
                                                 game_command_menu_items.slots[i].model.x + 2,
                                                 game_command_menu_items.slots[i].model.y + 1);
                        }

                        for (int i = 0; i < COMMAND_MENU_ITEMS_SLOT_COUNT; ++i)
                        {
                            if (game_command_menu_items.slots[i].occupied)
                            {
                                sprintf(game_items[i].count_buffer, "%d", game_items[i].count); 
                                RenderText(SDLWindow.Renderer, font.atlas,
                                           game_command_menu_items.slots[i].model.x + 10,
                                           game_command_menu_items.slots[i].model.y + 8, 
                                           game_items[i].count_buffer,
                                           color.white);
                            }
                        }

                    } break;
                    case 1: // Equip
                    {
                        for (int i = 0; i < COMMAND_MENU_EQUIP_SLOT_COUNT; ++i)
                        {
                            RenderAssetInCameraSpace(&game_command_menu_items.slots[i].model, 
                                                     game_command_menu_items.slots[i].model.x, 
                                                     game_command_menu_items.slots[i].model.y);
                        }
                    } break;
                    case 2: // Status
                    {
                                    
                        static char full_text[10][28];
                        static i32 full_len;
                        static i32 color_count;
                        static i32 non_colored_len;
                        static i32 base_x_arr[10];
                        static i32 base_y_arr[10];
                        static i32 offset_x_arr[10];

                        if (!game_init_stats)
                        {
                            char buffer[15];
                            char *name = NameEntry_GetName(&name_entry);
                            char *lvl = "1";
                            char *class_name = character_data.class.name;
                            char *class_personality = scenarios.personality;
                            char *exp = "91";

                            StatOverview_Init(buffer, in_game_class_status_overview[0].text, name, 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[1].text, lvl, 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[2].text, class_name, 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[3].text, class_personality, 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[4].text, exp, 17);

                            sprintf(game_int_buffer[0], "%d", character_data.base_stats.strength);
                            sprintf(game_int_buffer[1], "%d", character_data.base_stats.resilience);
                            sprintf(game_int_buffer[2], "%d", character_data.base_stats.agility);
                            sprintf(game_int_buffer[3], "%d", character_data.base_stats.stamina);
                            sprintf(game_int_buffer[4], "%d", character_data.base_stats.wisdom);
                            sprintf(game_int_buffer[5], "%d", character_data.base_stats.luck);
                            sprintf(game_int_buffer[6], "%d", character_data.base_stats.max_hp);
                            sprintf(game_int_buffer[7], "%d", character_data.base_stats.max_mp);
                            sprintf(game_int_buffer[8], "%d", character_data.base_stats.attack);
                            sprintf(game_int_buffer[9], "%d", character_data.base_stats.defense);
                            
                            StatOverview_Init(buffer, in_game_class_status_overview[5].text, game_int_buffer[0], 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[6].text, game_int_buffer[1], 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[7].text, game_int_buffer[2], 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[8].text, game_int_buffer[3], 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[9].text, game_int_buffer[4], 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[10].text, game_int_buffer[5], 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[11].text, game_int_buffer[6], 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[12].text, game_int_buffer[7], 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[13].text, game_int_buffer[8], 17);
                            StatOverview_Init(buffer, in_game_class_status_overview[14].text, game_int_buffer[9], 17);

                                            
                            full_len = strlen(in_game_class_status_overview[0].text);
                            color_count = 3;
                            non_colored_len = full_len - color_count;
                            if (non_colored_len < 0)
                                non_colored_len = 0;     

                            for (i32 i = 0, j = 5; i < 10 && j < 15; ++i, ++j)
                            {
                                PushString(full_text[i], in_game_class_status_overview[j].text);

                                base_x_arr[i] = in_game_class_status_overview[j].pos.x;
                                base_y_arr[i] = in_game_class_status_overview[j].pos.y;

                                offset_x_arr[i] = base_x_arr[i] + non_colored_len * GLYPH_WIDTH;
                            }

                            game_init_stats = true;
                        }

                        for (i32 i = 0; i < ArraySize(in_game_class_status_overview); ++i)
                        {
                            RenderText(SDLWindow.Renderer, font.atlas,
                                       in_game_class_status_overview[i].pos.x,
                                       in_game_class_status_overview[i].pos.y,
                                       in_game_class_status_overview[i].text,
                                       color.white);
                        }

                        for (i32 i = 0; i < 13; ++i)
                        {
                            RenderText(SDLWindow.Renderer, font.atlas,
                                       offset_x_arr[i],
                                       base_y_arr[i],
                                       full_text[i] + non_colored_len,
                                       stat_overview_color[i]);
                        }
                    } break;
                    default:
                    {

                    } break;
                }

                CursorForItems(&game_command_menu.options[game_command_menu.index], &cursor[RIGHT_CURSOR].model, 4, 1);
                RenderAssetInCameraSpace(&cursor[RIGHT_CURSOR].model,
                                         game_command_menu.options[game_command_menu.index].x - 12,     
                                         game_command_menu.options[game_command_menu.index].y - 2);     
                for (i32 i = 0; i < ArraySize(game_command_menu.options); ++i)
                {
                    RenderText(SDLWindow.Renderer, font.atlas,
                               game_command_menu.options[i].x,
                               game_command_menu.options[i].y, 
                               game_command_menu.options[i].text,
                               color.white);
                }


                if (game_command_menu_items.is_opened)
                {
                    // Render the cursor that iterates over the slots in items
                    CursorForAssets(&game_command_menu_items.slots[game_command_menu_items.index].model, &cursor[RIGHT_CURSOR].model, 0, 0);
                    RenderAssetInCameraSpace(&cursor[RIGHT_CURSOR].model, 
                                             game_command_menu_items.slots[game_command_menu_items.index].model.x - 10, 
                                             game_command_menu_items.slots[game_command_menu_items.index].model.y + 1);

                    // Render only if slot is occupied
                    if (game_command_menu_items.slots[game_command_menu_items.index].occupied)
                    {
                        // Rough example of rendering item description on hover
                        RenderAssetInCameraSpace(&game_item_description.box,
                                                 SCREEN_CENTER_X - 112, SCREEN_CENTER_Y + 48);

                        /*RenderAssetInCameraSpaceDIMENSION(&game_items[0].asset, 
                                                          SCREEN_CENTER_X - 96, SCREEN_CENTER_Y + 80,
                                                          (game_items[0].asset.w*2), (game_items[0].asset.h*2));*/

                        RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X(game_items[game_command_menu_items.index].name, -56),
                               SCREEN_CENTER_Y + 68, 
                               game_items[game_command_menu_items.index].name,
                               color.white);

                        RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X(game_items[game_command_menu_items.index].description, -50),
                               SCREEN_CENTER_Y + 80, 
                               game_items[game_command_menu_items.index].description,
                               color.white);

                        RenderText(SDLWindow.Renderer, font.atlas,
                               SET_TEXT_CENTER_X("Sell Value: 1", 56),
                               SCREEN_CENTER_Y + 92, 
                               "Sell Value: 1",
                               color.white);


                        if (item_slot_option)
                        {
                            RenderAssetInCameraSpace(&item_slot_asset, 
                                                     game_command_menu_items.slots[game_command_menu_items.index].model.x + 20, 
                                                     game_command_menu_items.slots[game_command_menu_items.index].model.y - 3);
                            
                            RenderText(SDLWindow.Renderer, font.atlas,
                                           game_command_menu_items.slots[game_command_menu_items.index].model.x + 36,
                                           game_command_menu_items.slots[game_command_menu_items.index].model.y + 4, 
                                           item_slot_options[0].text,
                                           color.white);

                            RenderText(SDLWindow.Renderer, font.atlas,
                                           game_command_menu_items.slots[game_command_menu_items.index].model.x + 33,
                                           game_command_menu_items.slots[game_command_menu_items.index].model.y + 16, 
                                           item_slot_options[1].text,
                                           color.white);

                            RenderText(SDLWindow.Renderer, font.atlas,
                                           game_command_menu_items.slots[game_command_menu_items.index].model.x + 33,
                                           game_command_menu_items.slots[game_command_menu_items.index].model.y + 28, 
                                           item_slot_options[2].text,
                                           color.white);

                            CursorForItems(&item_slot_options[item_slot_option_index], &cursor[RIGHT_CURSOR].model, 0, 0);
                            RenderAssetInCameraSpace(&cursor[RIGHT_CURSOR].model, 
                                                     item_slot_options[item_slot_option_index].x, 
                                                     item_slot_options[item_slot_option_index].y);
                        }
                    }
                }
            }

            // Set render target to camera
            SDL_SetRenderTarget(SDLWindow.Renderer, NULL);
            SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 0, 0, 255);
            SDL_RenderClear(SDLWindow.Renderer);
            
            // Render to camera
            SDL_RenderCopy(SDLWindow.Renderer, SDLCamera.TargetTexture, NULL, NULL);
        }

        SDL_RenderPresent(SDLWindow.Renderer);
    }

    // TODO: Store objecst in their own respecitive block of mem then free at once
    DestroyCamera(); 
    DestroyAssets(&room_asset[0]);
    DestroyAssets(&cursor[RIGHT_CURSOR].model);
    SDL_DestroyTexture(font.atlas);


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


