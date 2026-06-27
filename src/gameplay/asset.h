#ifndef ASSET_H
#define ASSET_H

#include "common.h"
#include "sound.h"

typedef struct asset_brain_t
{
    int unused;
} asset_brain_t;

typedef struct asset_front_face_t
{
    bool is_up;
    bool is_down;
    bool is_left;
    bool is_right;
} asset_front_face_t;

typedef struct asset_line_of_sight_t
{
    bool is_front;
    asset_front_face_t front;
    SDL_Rect range[4];
} asset_line_of_sight_t;

typedef struct test_asset_line_of_sight_t
{
    bool is_front;
    asset_front_face_t front;
    SDL_Rect range;
} test_asset_line_of_sight_t;

typedef struct asset_chase_radius_t
{
    SDL_Rect range;
} asset_chase_radius_t;

typedef struct asset_direction_t
{
    bool up, down, left, right;
} asset_direction_t;

typedef struct asset_conditions_t
{
    bool has_movement;
    bool has_movement_priority;
    bool has_renderer;
    bool has_physics;
    bool has_collided;
    bool is_occupied;
} asset_conditions_t;

typedef struct asset_t
{
    i32 x, y;
    i32 w, h;

    bool spawn_card;
    bool first_combat_encounter;

    asset_direction_t direction;
    asset_conditions_t conditions;
    asset_line_of_sight_t los;
    test_asset_line_of_sight_t test_los[4];
    asset_chase_radius_t chase_radius;

    SDL_Rect body;
    SDL_Rect adjacent_hitboxes[4];
    SDL_Texture *texture;
} asset_t;

#define MAX_CARD_STACK 4
typedef struct asset_card_stack_t
{
    i32 index;
    asset_t asset[MAX_CARD_STACK];
} asset_card_stack_t;

extern asset_front_face_t front_face_zero;

void LoadAsset(asset_t *asset, const char *filename);
asset_t IdealLoadAsset(const char *filename);

void UpdateAssetLOS(asset_t *asset);
void UpdateAssetAdjHitboxes(asset_t *asset);
void UpdateAssetChaseRadius(asset_t *asset);
void UpdateAssetProperties(asset_t *asset);
bool UpdateAssetMovement(asset_t *asset, sound_wav_t *sound);
void TestUpdateAssetNPCMovement(asset_t *asset, sound_wav_t *sound);
void UpdateAssetNPCMovement(asset_t *asset, sound_wav_t *sound);
void UpdatePushableAsset(asset_t *asset, asset_t *player);

void InitializeAssetToRender(asset_t *asset, i32 x, int y, int w, int h);
void InitializeAssetConditions(asset_t *asset);

void RenderAssetLOS(asset_t *asset);
void RenderAssetAdjHitboxes(asset_t *asset);
void RenderAssetChaseRadius(asset_t *asset);
void RenderAssetInWorldSpace(asset_t *asset);
void Asset_SetPosition(asset_t *asset, vec2_t *coords);
void RenderAssetInWorldSpaceWithCoords(asset_t *asset, int x, int y);
void RenderAsset(asset_t *asset, i32 x, int y, int w, int h);
void RenderAssetInCameraSpace(asset_t *asset, i32 x, int y);
void RenderAssetInCameraSpaceDIMENSION(asset_t asset, i32 x, int y, int w, int h);

void SetAssetAdjacentHitBoxes(asset_t *asset);
void SetAssetLOS(asset_t *asset);
void SetAssetChaseRadius(asset_t *asset);
void SetAssetConditionAllOn(asset_t *asset);
void SetAssetConditionAllOff(asset_t *asset);
void SetAssetConditionOn(asset_t *asset, i32 index);
void SetAssetConditionOff(asset_t *asset, i32 index);
vec2_t Vec2_SetPosition(i32 x, i32 y);
void SetAssetPosition(asset_t *asset, i32 x, int y);
void InitializeAsset(asset_t *asset, vec2_t *pos);

asset_t Asset_Zero(void);
void AssetCard_Push(asset_card_stack_t *stack, asset_t card);
void AssetCard_Pop(asset_card_stack_t *stack);
void AssetCard_Render(asset_card_stack_t *stack);

void DestroyAssets(asset_t *assets);

#endif

