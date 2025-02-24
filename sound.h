#ifndef SOUND_H
#define SOUND_H

#include "common.h"

typedef struct
{
    bool reset_audio;
    int volume;
    u32 offset;
    u32 length;
    u8 *buffer;
    SDL_AudioSpec spec;
    SDL_AudioDeviceID device_id;
} sound_wav_t;

typedef struct
{
    int volume;
    sound_wav_t music; 
} sound_music_t;

typedef struct
{
    int volume;
    sound_wav_t sfx; 
} sound_sfx_t;

typedef struct 
{
    // any audio output will only go as high as the master volume,
    // if the sfx audio was maxed but master volume was set to 10%, it'll
    // only be 10% as strong though it's maxed 
    
    int volume;

    // array size will stay to be hardcoded in, why tf malloc?
    sound_music_t music[2]; 
    sound_sfx_t sfx[2];
} sound_master_volume_t;


#endif
