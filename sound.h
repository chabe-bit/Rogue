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

typedef struct
{
    bool mute;
    bool one;
    bool two;
    bool three;
    bool max;
    bool apply;

    int index;
    int volume_size_bars;
    int volume;
    color_t colors;
    SDL_Rect blocks[5];
} sound_volume_controller_t;

typedef struct
{
    int index;
    bool is_active;

    SDL_Rect volume_body[3];
    menu_item_t options[5];
} sound_settings_t;


typedef enum
{
    AUDIO_TYPE_NONE,
    AUDIO_TYPE_MUSIC,
    AUDIO_TYPE_SFX
} sound_wav_audio_type;
extern sound_wav_audio_type audio_type_state; 

typedef enum
{
    VOL_SETTINGS_NONE,
    VOL_SETTINGS_MASTER,
    VOL_SETTINGS_MUSIC,
    VOL_SETTINGS_SFX,
    VOL_SETTINGS_APPLY,
    VOL_SETTINGS_BACK
} sound_volume_settings_type;
extern sound_volume_settings_type volume_settings_state;

typedef enum
{
    VOL_LEVEL_NONE,
    VOL_LEVEL_GREEN,
    VOL_LEVEL_YELLOW,
    VOL_LEVEL_ORANGE,
    VOL_LEVEL_RED
} sound_volume_level_type;
extern sound_volume_level_type volume_level_state;



#define VOLUME_CONTROLLER_COUNT 3


void Sound_PlayMusic(sound_wav_t *sound);
void Sound_PlaySFX(sound_wav_t *sound);
void Sound_PauseMusic(sound_wav_t *sound);

void Sound_SFXCallback(void *user_data, u8 *stream, int length);
void Sound_MusicCallback(void *user_data, u8 *stream, int length);
void Sound_LoadWavFile(sound_wav_t *wav, const char *filename, sound_wav_audio_type audio_type);

void Sound_InitSettings(sound_settings_t *sound_settings);
void Sound_MoveUpSettings(sound_settings_t *sound_settings);
void Sound_MoveDownSettings(sound_settings_t *sound_settings);
void Sound_MoveRightSettings(sound_settings_t *sound_settings);
void Sound_MoveLeftSettings(sound_settings_t *sound_settings);


void Sound_IncreaseVolume(sound_volume_controller_t *volume_controller); 
void Sound_DecreaseVolume(sound_volume_controller_t *volume_controller);

void Sound_InitVolumeBar(sound_settings_t *sound_settings, sound_volume_controller_t *volume_controller, int volume_controller_count);
void Sound_UpdateVolumeBars(sound_volume_controller_t *volume_controller);
void Sound_RenderVolumeBars(SDL_Renderer *renderer, sound_volume_controller_t *volume_controller, int volume_controller_count);

#endif
