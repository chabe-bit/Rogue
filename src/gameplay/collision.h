#ifndef COLLISION_H
#define COLLISION_H

#include "asset.h"

extern bool combat_mode_initiated;
extern bool chase_initiated;

bool AABB_Detection(SDL_Rect *A, SDL_Rect *B);
void AABB_Resolution(asset_t *A, asset_t *B);
void AABB_AdjHitboxResolution(asset_t *A, asset_t *B);
void AABB_LOSResolution(asset_t *A, asset_t *B);
void AABB_ChaseRadiusResolution(asset_t *A, asset_t *B);

#endif
