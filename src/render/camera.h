#ifndef CAMERA_H
#define CAMERA_H

#include "asset.h"

typedef struct camera_t
{
    SDL_Texture *TargetTexture;
    i32 TargetWidth, TargetHeight;
    i32 X, Y, W, H;
} camera_t;

extern camera_t SDLCamera;

void InitializeCamera(void);
void AttachCameraToPlayer(asset_t *player, asset_t *room);
void DestroyCamera(void);

#endif
