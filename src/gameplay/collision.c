#include "collision.h"

bool AABB_Detection(SDL_Rect *A, SDL_Rect *B)
{
    if (A->y + A->h <= B->y)
        return false;

    if (A->y >= B->y + B->h)
        return false;

    if (A->x + A->w <= B->x)
        return false;

    if (A->x >= B->x + B->w)
        return false;

    return true;
}

void AABB_Resolution(asset_t *A, asset_t *B)
{
    if (AABB_Detection(&A->body, &B->body) &&
        (A->conditions.has_physics && B->conditions.has_physics)) {
        A->body.x = A->x;
        A->body.y = A->y;

        if (B->conditions.has_movement)
            B->conditions.has_movement = false;

    } else {
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
         (AABB_Detection(&A->body, &B->body))) {
        // Push back collision 
        B->body.x = B->x;
        B->body.y = B->y;

        // Asset A's collision with Asset B is handled here
        if (AABB_Detection(&A->body, &B->body)) {
            A->body.x = A->x;
            A->body.y = A->y;
        }

        // Toggles Asset B's movement off
        if (B->conditions.has_movement) {
            B->conditions.has_movement = false;
        }

    } else {
        // Releases Asset B if out of Asset A's adjacent hitbox
        B->conditions.has_movement = true;
    }
}

//bool first_combat_encounter = false;
//bool enemy_los_asset.spawn_card = false;
bool combat_mode_initiated = false;
bool chase_initiated = false;

void AABB_LOSResolution(asset_t *A, asset_t *B)
{
    if ((AABB_Detection(&A->los.range[0], &B->body)) ||
     (AABB_Detection(&A->los.range[1], &B->body)) ||
     (AABB_Detection(&A->los.range[2], &B->body)) ||
     (AABB_Detection(&A->los.range[3], &B->body)) ||
     (AABB_Detection(&A->body, &B->body))) {
        printf("in los range!\n");

        // Initiate combat mode!
        combat_mode_initiated = true;

        // Spawn that enemy card and only once
        if (!A->first_combat_encounter) {
            //enemy_los_asset.spawn_card = true;
            A->spawn_card = true;
            A->first_combat_encounter = true;
        }

    }
}


// Each asset should have their own instance of this condition because let's say two
// enemies are encountered at the same time, this one instance would probably lag behind
// by 1 or cause some sort of collision. If these were seperate we might not need to worry
// about that.
void AABB_ChaseRadiusResolution(asset_t *A, asset_t *B)
{
    if (AABB_Detection(&A->chase_radius.range, &B->body)) {
        // If in combat mode and in range, chase
        if (combat_mode_initiated)
            chase_initiated = true;
    } else {
        // End combat and chase, if out of chase range 
        combat_mode_initiated = false;
        chase_initiated = false;
    }
}
