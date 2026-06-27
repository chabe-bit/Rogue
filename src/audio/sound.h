#ifndef SOUND_H
#define SOUND_H

#include "common.h"

#define MASTER_INDEX    0
#define MUSIC_INDEX     1
#define SFX_INDEX       4

#define MUSIC_FILE_COUNT 2
#define SFX_FILE_COUNT 2
#define VOLUME_CONTROLLER_COUNT 3

extern const char *music_files[MUSIC_FILE_COUNT];
extern const char *sfx_files[SFX_FILE_COUNT];

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
    sound_wav_t wav; 
    int volume;
} sound_music_t;

typedef struct
{
    sound_wav_t wav; 
    int volume;
} sound_sfx_t;

typedef struct 
{
    sound_music_t *music[MUSIC_FILE_COUNT]; 
    sound_sfx_t *sfx[SFX_FILE_COUNT];
    int volume;
} sound_master_volume_t;

typedef struct
{
    bool mute;
    bool one;
    bool two;
    bool three;
    bool max;
    bool apply;
    
    // Ideal way of creating 3 instances of volume controllers rather than this entire struct itself because then from above, we have three instances of each, where we only need one. 
    struct
    {
        int index;
        int volume_size_bars;
        int volume;
        rgba_t colors;
        SDL_Rect blocks[5];
    
        bool touched;
        bool mute;
        bool one;
        bool two;
        bool three;
        bool max;
    } info[3];
    
    bool touched;
    
    int index;
    int volume_size_bars;
    int volume;
    rgba_t colors;
    SDL_Rect blocks[5];
} sound_volume_controller_t;

typedef struct
{
    int index;
    SDL_Rect volume_body[3];
    option_t options[5];
    bool is_active;
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

void Sound_InitMusic(sound_music_t *music, const char *file_name[]);
void Sound_InitSFX(sound_sfx_t *sfx, const char *file_name[]);
void Sound_InitMaster(sound_master_volume_t* master, sound_music_t *music, sound_sfx_t *sfx);

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


void Sound_TestInitVolumeBar(sound_settings_t *sound_settings, sound_volume_controller_t *volume_controller, int volume_controller_count);
void Sound_TestIncreaseVolume(sound_volume_controller_t *volume_controller, int INDEX); 
void Sound_TestDecreaseVolume(sound_volume_controller_t *volume_controller, int INDEX);
void Sound_TestUpdateVolumeBars(sound_volume_controller_t *volume_controller, int INDEX);
void Sound_TestRenderVolumeBars(SDL_Renderer *renderer, sound_volume_controller_t *volume_controller, int volume_controller_count);



#endif
