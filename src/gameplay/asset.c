#include "asset.h"
#include "camera.h"
#include "collision.h"
#include "sdl_platform.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

asset_front_face_t front_face_zero = { 0 };

void LoadAsset(asset_t *asset, const char *filename)
{
    i32 channels;
    unsigned char *data = stbi_load(filename, &asset->w, &asset->h, &channels, STBI_default);
    if (data == NULL) {
        fprintf(stderr, "Failed to load files: %s\n", filename);
        return;
    }

    i32 fmt = channels == 2 ? SDL_PIXELFORMAT_RGBA8888 : SDL_PIXELFORMAT_RGBA32;
    i32 pitch = asset->w * channels;

    // Free later!
    asset->texture = SDL_CreateTexture(SDLWindow.Renderer, fmt, SDL_TEXTUREACCESS_STATIC, asset->w, asset->h);
    if (asset->texture == NULL) {
        fprintf(stderr, "Failed to create texture for sprites: %s\n", SDL_GetError());
        return;
    }

    if (SDL_UpdateTexture(asset->texture, NULL, (const void *)data, pitch) < 0) {
        fprintf(stderr, "Failed to update texture for sprites: %s\n", SDL_GetError());
        return;
    }

    stbi_image_free(data);
}

// Probably the ideal way of loading an asset
asset_t IdealLoadAsset(const char *filename)
{
    asset_t asset = { 0 };

    i32 channels;
    unsigned char *data = stbi_load(filename, &asset.w, &asset.h, &channels, STBI_default);
    if (data == NULL) {
        fprintf(stderr, "Failed to load files: %s\n", filename);
        return asset;
    }

    i32 fmt = channels == 2 ? SDL_PIXELFORMAT_RGBA8888 : SDL_PIXELFORMAT_RGBA32;
    i32 pitch = asset.w * channels;

    // Free later!
    asset.texture = SDL_CreateTexture(SDLWindow.Renderer, fmt, SDL_TEXTUREACCESS_STATIC, asset.w, asset.h);
    if (asset.texture == NULL) {
        fprintf(stderr, "Failed to create texture for sprites: %s\n", SDL_GetError());
        return asset;
    }

    if (SDL_UpdateTexture(asset.texture, NULL, (const void *)data, pitch) < 0) {
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
    asset->los.range[ASSET_TOP].y = (asset->body.y - (24 * 3));
    asset->los.range[ASSET_TOP].w = 16;
    asset->los.range[ASSET_TOP].h = (24 * 3);

    asset->los.range[ASSET_BOT].x = asset->body.x;
    asset->los.range[ASSET_BOT].y = (asset->body.y + 24);
    asset->los.range[ASSET_BOT].w = 16;
    asset->los.range[ASSET_BOT].h = (24 * 3);

    asset->los.range[ASSET_LEFT].x = (asset->body.x - (16 * 3));
    asset->los.range[ASSET_LEFT].y = asset->body.y;
    asset->los.range[ASSET_LEFT].w = (16 * 3);
    asset->los.range[ASSET_LEFT].h = 24;

    asset->los.range[ASSET_RIGHT].x = (asset->body.x + 16);
    asset->los.range[ASSET_RIGHT].y = asset->body.y;
    asset->los.range[ASSET_RIGHT].w = (16 * 3);
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

    asset->adjacent_hitboxes[ASSET_LEFT].x = (asset->body.x - 16);
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
    asset->x = asset->body.x;
    asset->y = asset->body.y;

    if (asset->conditions.has_movement) {
        if (asset->direction.up) {
            asset->body.y -= asset->body.h;

            Sound_PlaySFX(sound);
            asset->direction.up = false;

            return true;
        }

        if (asset->direction.down) {
            asset->body.y += asset->body.h;

            Sound_PlaySFX(sound);
            asset->direction.down = false;

            return true;
        }

        if (asset->direction.left) {
            asset->body.x -= asset->body.w;

            Sound_PlaySFX(sound);
            asset->direction.left = false;

            return true;
        }

        if (asset->direction.right) {
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
    ASSET_NPC_IDLE,
    ASSET_NPC_UP,
    ASSET_NPC_DOWN,
    ASSET_NPC_LEFT,
    ASSET_NPC_RIGHT,
} asset_npc_movement_type;

void TestUpdateAssetNPCMovement(asset_t *asset, sound_wav_t *sound)
{
    asset->x = asset->body.x;
    asset->y = asset->body.y;

    i32 direction = 3; // rand() % 5;

    asset_npc_movement_type npc = ASSET_NPC_IDLE;
    npc = direction;

    if (asset->conditions.has_movement) {
        switch (npc) {
        case ASSET_NPC_IDLE:
        {
            // Do nothing
        } break;
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
        default:
        {
            npc = ASSET_NPC_IDLE;
        } break;
        }
    }
}

void UpdateAssetNPCMovement(asset_t *asset, sound_wav_t *sound)
{
    asset->x = asset->body.x;
    asset->y = asset->body.y;

    i32 direction = 3; // rand() % 5;

    asset_npc_movement_type npc = ASSET_NPC_IDLE;
    npc = direction;

    if (asset->conditions.has_movement) {
        switch (npc) {
        case ASSET_NPC_IDLE:
        {
            // Do nothing
        } break;
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
        default:
        {
            npc = ASSET_NPC_IDLE;
        } break;
        }
    }
}

void UpdatePushableAsset(asset_t *asset, asset_t *player)
{
    asset->x = asset->body.x;
    asset->y = asset->body.y;

    if (asset->conditions.has_movement) {
        if (asset->direction.up) {
            if (AABB_Detection(&asset->body, &player->body) &&
                asset->conditions.has_physics) {
                asset->body.y -= asset->body.h;
            }

            asset->direction.up = false;
        }

        if (asset->direction.down) {
            if (AABB_Detection(&asset->body, &player->body) &&
                asset->conditions.has_physics) {
                asset->body.y += asset->body.h;
            }

            asset->direction.down = false;
        }

        if (asset->direction.left) {
            if (AABB_Detection(&asset->body, &player->body) &&
                asset->conditions.has_physics) {
                asset->body.x -= asset->body.w;
            }

            asset->direction.left = false;
        }

        if (asset->direction.right) {
            if (AABB_Detection(&asset->body, &player->body) &&
                asset->conditions.has_physics) {
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
    asset->los.range[ASSET_TOP].y = (asset->body.y - (24 * 3)) - SDLCamera.Y;

    asset->los.range[ASSET_BOT].x = asset->body.x - SDLCamera.X;
    asset->los.range[ASSET_BOT].y = (asset->body.y + 24) - SDLCamera.Y;

    asset->los.range[ASSET_LEFT].x = (asset->body.x - (16 * 3)) - SDLCamera.X;
    asset->los.range[ASSET_LEFT].y = asset->body.y - SDLCamera.Y;

    asset->los.range[ASSET_RIGHT].x = (asset->body.x + 16) - SDLCamera.X;
    asset->los.range[ASSET_RIGHT].y = asset->body.y - SDLCamera.Y;

    SDL_SetRenderDrawColor(SDLWindow.Renderer, 0, 255, 0, 255);
    for (int i = 0; i < 4; ++i) {
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
    for (int i = 0; i < 4; ++i) {
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

    if (asset->texture) {
        SDL_SetTextureBlendMode(asset->texture, SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(SDLWindow.Renderer, asset->texture, NULL, &asset->body) < 0) {
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

    if (asset->texture) {
        SDL_SetTextureBlendMode(asset->texture, SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(SDLWindow.Renderer, asset->texture, NULL, &asset->body) < 0) {
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

    if (asset->texture) {
        SDL_SetTextureBlendMode(asset->texture, SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(SDLWindow.Renderer, asset->texture, NULL, &asset->body) < 0) {
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

    if (asset->texture) {
        SDL_SetTextureBlendMode(asset->texture, SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(SDLWindow.Renderer, asset->texture, NULL, &asset->body) < 0) {
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

    if (asset.texture) {
        SDL_SetTextureBlendMode(asset.texture, SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(SDLWindow.Renderer, asset.texture, NULL, &asset.body) < 0) {
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
    asset->los.range[ASSET_TOP].y = (asset->body.y - (24 * 3));
    asset->los.range[ASSET_TOP].w = 16;
    asset->los.range[ASSET_TOP].h = (24 * 3);

    asset->los.range[ASSET_BOT].x = asset->body.x;
    asset->los.range[ASSET_BOT].y = (asset->body.y + 24);
    asset->los.range[ASSET_BOT].w = 16;
    asset->los.range[ASSET_BOT].h = (24 * 3);

    asset->los.range[ASSET_LEFT].x = (asset->body.x - (16 * 3));
    asset->los.range[ASSET_LEFT].y = asset->body.y;
    asset->los.range[ASSET_LEFT].w = (16 * 3);
    asset->los.range[ASSET_LEFT].h = 24;

    asset->los.range[ASSET_RIGHT].x = (asset->body.x + 16);
    asset->los.range[ASSET_RIGHT].y = asset->body.y;
    asset->los.range[ASSET_RIGHT].w = (16 * 3);
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
    switch (index) {
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
    switch (index) {
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
    vec2_t pos = { 0 };

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



asset_t Asset_Zero()
{
    asset_t zero = { 0 };
    return zero;
}

void AssetCard_Push(asset_card_stack_t *stack, asset_t card)
{
    if (stack->index < MAX_CARD_STACK) {
        stack->asset[stack->index] = card;

        // Update the asset's position based on index
        switch (stack->index) {
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
    for (int i = 0; i < stack->index; ++i) {
        RenderAssetInCameraSpace(&stack->asset[i],
                                 stack->asset[i].x,
                                 stack->asset[i].y);
    }
}

void DestroyAssets(asset_t *assets)
{
    SDL_DestroyTexture(assets->texture);
}
