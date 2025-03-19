#include "sound.h"

sound_wav_audio_type audio_type_state = AUDIO_TYPE_NONE;
sound_volume_settings_type volume_settings_state = VOL_SETTINGS_NONE;
sound_volume_level_type volume_level_state = VOL_LEVEL_NONE;

const char *music_files[MUSIC_FILE_COUNT] = {
    {"assets/music/5. Smooth As Glass.wav"},
    {"assets/music/4. Church of Order.wav"}
};

const char *sfx_files[SFX_FILE_COUNT] = {
    {"assets/sfx/select.wav"},
    {"assets/sfx/confirm.wav"},
    {"assets/sfx/fire_a.wav"},
    {"assets/sfx/fire_b.wav"}
};

void Sound_InitMusic(sound_music_t *music, const char *file_name[])
{
    for (int i = 0; i < MUSIC_FILE_COUNT; ++i)
    {
        Sound_LoadWavFile(&music[i].wav, file_name[i], AUDIO_TYPE_MUSIC);
    }
}

void Sound_InitSFX(sound_sfx_t *sfx, const char *file_name[])
{
    for (int i = 0; i < SFX_FILE_COUNT; ++i)
    {
        Sound_LoadWavFile(&sfx[i].wav, file_name[i], AUDIO_TYPE_SFX);
    }
}

void Sound_InitMaster(sound_master_volume_t* master, sound_music_t *music, sound_sfx_t *sfx)
{
    for (int i = 0; i < MUSIC_FILE_COUNT; ++i)
    {
        master->music[i] = &music[i];
    }
    printf("master->music length: %d\n", master->music[0]->wav.length);
    printf("master->music volume: %d\n", master->music[0]->wav.volume);

    for (int i = 0; i < SFX_FILE_COUNT; ++i)
    {
        master->sfx[i] = &sfx[i];
    }
}

void Sound_PlayMusic(sound_wav_t *sound)
{
    SDL_PauseAudioDevice(sound->device_id, 0);
}

void Sound_PlaySFX(sound_wav_t *sound)
{
    SDL_PauseAudioDevice(sound->device_id, 0);

    // Lock to reset the audio from loop and to instead reset if button is played again, then unlock
    SDL_LockAudioDevice(sound->device_id);
    sound->reset_audio = true;
    SDL_UnlockAudioDevice(sound->device_id);
}

void Sound_PauseMusic(sound_wav_t *sound)
{
    SDL_PauseAudioDevice(sound->device_id, 1);
}

void Sound_SFXCallback(void *user_data, u8 *stream, int length)
{
    sound_wav_t *context = (sound_wav_t *)user_data;

    // Clear the output buffer.
    SDL_memset(stream, 0, length);

    if (context->reset_audio) {
        context->offset = 0;
        context->reset_audio = false;
    }

    // Calculate how many bytes remain in the buffer.
    u32 remaining = context->length - context->offset;
    if ((u32)length > remaining) {
        // Mix only the remainder of the audio buffer.
        SDL_MixAudioFormat(stream, context->buffer + context->offset,
                           context->spec.format, remaining, context->volume);
        // Set offset to the end, so we don't loop.
        context->offset = context->length;
    } else {
        // Mix normally, with no wrap-around.
        SDL_MixAudioFormat(stream, context->buffer + context->offset,
                           context->spec.format, length, context->volume);
        context->offset += length;
        if (context->offset >= context->length) {
            context->offset = context->length;
        }
    }
}

void Sound_MusicCallback(void *user_data, u8 *stream, int length)
{
    sound_wav_t *context = (sound_wav_t *)user_data;

    // Clear the output buffer.
    SDL_memset(stream, 0, length);

    // Calculate how many bytes remain in the buffer before wrap-around.
    u32 remaining = context->length - context->offset;
    if ((u32)length > remaining) {
        // Mix the remainder of the audio buffer.
        SDL_MixAudioFormat(stream, context->buffer + context->offset,
                           context->spec.format, remaining, context->volume);
        // Then wrap around and mix the beginning of the buffer.
        SDL_MixAudioFormat(stream + remaining, context->buffer,
                           context->spec.format, length - remaining, SDL_MIX_MAXVOLUME);
        context->offset = length - remaining;
    } else {
        // No wrap-around needed.
        SDL_MixAudioFormat(stream, context->buffer + context->offset,
                           context->spec.format, length, context->volume);
        context->offset += length;
        if (context->offset >= context->length) {
            context->offset = 0;
        }
    }
}

void Sound_LoadWavFile(sound_wav_t *wav, const char *filename, sound_wav_audio_type audio_type)
{
    // Must convert the raw audio data to match the system's sample rate 
    SDL_AudioSpec loadedSpec;
    u8 *loadedBuffer = NULL;
    u32 loadedLength = 0;

    if (SDL_LoadWAV(filename, &loadedSpec,
                &loadedBuffer, &loadedLength) == NULL)
    {
        SDL_Log("Failed to load wav %s\n", SDL_GetError());
        return;
    }
    
    SDL_AudioSpec desiredSpec;
    SDL_zero(desiredSpec);
    desiredSpec.freq = 48000;             // desired frequency
    desiredSpec.format = AUDIO_S16LSB;      // desired sample format
    desiredSpec.channels = 2;             // desired number of channels
    desiredSpec.samples = 512;            // desired buffer size
           
    switch (audio_type)
    {
        case AUDIO_TYPE_NONE:
        {       
            fprintf(stderr, "Invalid audio type: %d\n", audio_type);
            return;
        } break;
        case AUDIO_TYPE_MUSIC:
        {       
            desiredSpec.callback = Sound_MusicCallback;  
        } break;
        case AUDIO_TYPE_SFX:
        {       
            desiredSpec.callback = Sound_SFXCallback;  
        } break;
        default:
        {
            audio_type = AUDIO_TYPE_NONE;
        } break;
    }

    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt,
                          loadedSpec.format, loadedSpec.channels, loadedSpec.freq,
                          desiredSpec.format, desiredSpec.channels, desiredSpec.freq) < 0) {
        SDL_Log("SDL_BuildAudioCVT failed: %s\n", SDL_GetError());
        SDL_FreeWAV(loadedBuffer);
        SDL_Quit();
        return;
    }
   
    // Allocate a buffer for the converted audio data.
    cvt.len = loadedLength;
    cvt.buf = (Uint8 *)malloc(cvt.len * cvt.len_mult);
    if (!cvt.buf) {
        SDL_Log("Failed to allocate conversion buffer\n");
        SDL_FreeWAV(loadedBuffer);
        SDL_Quit();
        return;
    }
    
    // Copy the loaded data into the conversion buffer.
    memcpy(cvt.buf, loadedBuffer, loadedLength);
    SDL_FreeWAV(loadedBuffer); // free the original loaded buffer

    // Convert the audio data.
    if (SDL_ConvertAudio(&cvt) < 0) {
        SDL_Log("SDL_ConvertAudio failed: %s\n", SDL_GetError());
        free(cvt.buf);
        SDL_Quit();
        return;
    }
   
    u32 convertedLength = cvt.len_cvt;
    printf("Converted audio length: %u bytes\n", convertedLength);

    wav->volume = 16;
    wav->offset = 0;   
    wav->length = convertedLength;   
    wav->buffer = cvt.buf;   
    wav->spec = desiredSpec;  
    wav->spec.userdata = wav;  

    SDL_AudioSpec obtained;
    wav->device_id = SDL_OpenAudioDevice(NULL, 0, &wav->spec, &obtained, 0);
    if (wav->device_id == 0)
    {
        SDL_Log("Failed to open audio: %s\n", SDL_GetError());
        return;
    }
}

void Sound_InitSettings(sound_settings_t *sound_settings)
{
    sound_settings->options[0].text = "Master";
    sound_settings->options[0].x = SET_TEXT_CENTER_X(sound_settings->options[0].text, 0);
    sound_settings->options[0].y = SCREEN_CENTER_Y + 0;

    sound_settings->options[1].text = "Music";
    sound_settings->options[1].x = SET_TEXT_CENTER_X(sound_settings->options[1].text, 0);
    sound_settings->options[1].y = SCREEN_CENTER_Y + 24;

    sound_settings->options[2].text = "SFX";
    sound_settings->options[2].x = SET_TEXT_CENTER_X(sound_settings->options[2].text, 0);
    sound_settings->options[2].y = SCREEN_CENTER_Y + 48;

    sound_settings->options[3].text = "Apply";
    sound_settings->options[3].x = SET_TEXT_CENTER_X(sound_settings->options[3].text, 96);
    sound_settings->options[3].y = SCREEN_CENTER_Y + 96;

    sound_settings->options[4].text = "Back";
    sound_settings->options[4].x = SET_TEXT_CENTER_X(sound_settings->options[4].text, -96);
    sound_settings->options[4].y = SCREEN_CENTER_Y + 96;
    
    sound_settings->volume_body[0].x = SCREEN_CENTER_X - ((96/2));
    sound_settings->volume_body[0].y = sound_settings->options[0].y + 10; // Master volume's Y position added
    sound_settings->volume_body[0].w = 96; 
    sound_settings->volume_body[0].h = GLYPH_HEIGHT + 2;

    sound_settings->volume_body[1].x = SCREEN_CENTER_X - ((96/2));
    sound_settings->volume_body[1].y = sound_settings->options[1].y + 10;
    sound_settings->volume_body[1].w = 96; 
    sound_settings->volume_body[1].h = GLYPH_HEIGHT + 2;

    sound_settings->volume_body[2].x = SCREEN_CENTER_X - ((96/2));
    sound_settings->volume_body[2].y = sound_settings->options[2].y + 10;
    sound_settings->volume_body[2].w = 96; 
    sound_settings->volume_body[2].h = GLYPH_HEIGHT + 2;
}

void Sound_MoveUpSettings(sound_settings_t *sound_settings)
{
    sound_settings->index--;
    if (sound_settings->index < 0)
        sound_settings->index = ArraySize(sound_settings->options) - 1;

    printf("sound setting index: %d\n", sound_settings->index);
    
}

void Sound_MoveDownSettings(sound_settings_t *sound_settings)
{
    sound_settings->index++;
    if (sound_settings->index >= ArraySize(sound_settings->options))
        sound_settings->index = 0;
    
    printf("sound setting index: %d\n", sound_settings->index);
}

void Sound_MoveRightSettings(sound_settings_t *sound_settings)
{
    switch (sound_settings->index)
    {
        case 3:
        {
            sound_settings->index = 4;
            if (sound_settings->index >= ArraySize(sound_settings->options))
                sound_settings->index = 3;
        } break;
        case 4:
        {
            sound_settings->index = 3;
            if (sound_settings->index >= ArraySize(sound_settings->options))
                sound_settings->index = 3;

        } break;
    }
}

void Sound_MoveLeftSettings(sound_settings_t *sound_settings)
{
    switch (sound_settings->index)
    {
        case 3:
        {
            sound_settings->index = 4;
            if (sound_settings->index < 0)
                sound_settings->index = 3;
        } break;
        case 4:
        {
            sound_settings->index = 3;
            if (sound_settings->index < 0)
                sound_settings->index = 3;

        } break;
    }
}

void Sound_InitVolumeBar(sound_settings_t *sound_settings, sound_volume_controller_t *volume_controller, int volume_controller_count)
{
    for (int vc = 0; vc < volume_controller_count; ++vc)
    {
        for (int i = 0; i < ArraySize(volume_controller[vc].blocks); ++i)
        {
            volume_controller[vc].blocks[i].x = sound_settings->volume_body[vc].x + volume_controller[vc].volume_size_bars;
            volume_controller[vc].blocks[i].y = sound_settings->volume_body[vc].y;
            volume_controller[vc].blocks[i].w = sound_settings->volume_body[vc].w / 4;
            volume_controller[vc].blocks[i].h = sound_settings->volume_body[vc].h;
            volume_controller[vc].volume_size_bars += (sound_settings->volume_body[vc].w / 4);
        }
    }
    
    printf("volume_controller: %d\n", volume_controller[0].blocks[0].x);
}

void Sound_IncreaseVolume(sound_volume_controller_t *volume_controller) 
{
    volume_controller->index++;
    if (volume_controller->index >= ArraySize(volume_controller->blocks))
        volume_controller->index = ArraySize(volume_controller->blocks) - 1;
                  
}

void Sound_DecreaseVolume(sound_volume_controller_t *volume_controller)
{
    volume_controller->index--;
    if (volume_controller->index < 0)
        volume_controller->index = 0; // Stay at the start rather than wrapping around 
}

void Sound_UpdateVolumeBars(sound_volume_controller_t *volume_controller)
{
    switch (volume_controller->index)
    {
        case 0:
        {
            if (volume_controller->mute)
            {
                volume_controller->volume = 0;
                volume_controller->mute = false;
            }
        } break;
        case 1:
        {
            volume_controller->colors.r = 0;
            volume_controller->colors.g = 255;
            volume_controller->colors.b = 0;
            volume_controller->colors.a = 255;
            
            if (volume_controller->one)
            {
                volume_controller->volume = 32;
                volume_controller->one = false;
            }
        } break;
        case 2:
        {
            volume_controller->colors.r = 255;
            volume_controller->colors.g = 255;
            volume_controller->colors.b = 0;
            volume_controller->colors.a = 255;

            if (volume_controller->two)
            {
                volume_controller->volume = 64;
                volume_controller->two = false;
            }
        } break;
        case 3:
        {
            volume_controller->colors.r = 255;
            volume_controller->colors.g = 165;
            volume_controller->colors.b = 0;
            volume_controller->colors.a = 255;

            if (volume_controller->three)
            {
                volume_controller->volume = 96;
                volume_controller->three = false;
            }
        } break;
        case 4:
        {
            volume_controller->colors.r = 255;
            volume_controller->colors.g = 0;
            volume_controller->colors.b = 0;
            volume_controller->colors.a = 255;
            
            if (volume_controller->max)
            {
                volume_controller->volume = SDL_MIX_MAXVOLUME;
                volume_controller->max = false;
            }
        } break;
    }
}

void Sound_RenderVolumeBars(SDL_Renderer *renderer, sound_volume_controller_t *volume_controller, int volume_controller_count)
{
    for (int vc = 0; vc < volume_controller_count; ++vc)
    {
        for (int i = 0; i < volume_controller[vc].index; ++i)
        {
            SDL_SetRenderDrawColor(renderer, volume_controller[vc].colors.r, volume_controller[vc].colors.g, volume_controller[vc].colors.b, volume_controller[vc].colors.a);
            SDL_RenderFillRect(renderer, &volume_controller[vc].blocks[i]);
        }
    }
}



void Sound_TestInitVolumeBar(sound_settings_t *sound_settings, sound_volume_controller_t *volume_controller, int volume_controller_count)
{
    for (int vc = 0; vc < volume_controller_count; ++vc)
    {
        for (int i = 0; i < ArraySize(volume_controller->info[vc].blocks); ++i)
        {
            volume_controller->info[vc].blocks[i].x = sound_settings->volume_body[vc].x + volume_controller->info[vc].volume_size_bars;
            volume_controller->info[vc].blocks[i].y = sound_settings->volume_body[vc].y;
            volume_controller->info[vc].blocks[i].w = sound_settings->volume_body[vc].w / 4;
            volume_controller->info[vc].blocks[i].h = sound_settings->volume_body[vc].h;
            volume_controller->info[vc].volume_size_bars += (sound_settings->volume_body[vc].w / 4);
        }
    }
    
}

void Sound_TestRenderVolumeBars(SDL_Renderer *renderer, sound_volume_controller_t *volume_controller, int volume_controller_count)
{
    for (int vc = 0; vc < volume_controller_count; ++vc)
    {
        for (int i = 0; i < volume_controller->info[vc].index; ++i)
        {
            SDL_SetRenderDrawColor(renderer, volume_controller->info[vc].colors.r, volume_controller->info[vc].colors.g, volume_controller->info[vc].colors.b, volume_controller[vc].colors.a);
            SDL_RenderFillRect(renderer, &volume_controller->info[vc].blocks[i]);
        }
    }
}


void Sound_TestIncreaseVolume(sound_volume_controller_t *volume_controller, int INDEX) 
{
    volume_controller->info[INDEX].index++;
    if (volume_controller->info[INDEX].index >= ArraySize(volume_controller->info[INDEX].blocks))
        volume_controller->info[INDEX].index = ArraySize(volume_controller->info[INDEX].blocks) - 1;
                  
}

void Sound_TestDecreaseVolume(sound_volume_controller_t *volume_controller, int INDEX)
{
    volume_controller->info[INDEX].index--;
    if (volume_controller->info[INDEX].index < 0)
        volume_controller->info[INDEX].index = 0;
}

void Sound_TestUpdateVolumeBars(sound_volume_controller_t *volume_controller, int INDEX)
{
    switch (volume_controller->info[INDEX].index)
    {
        case 0:
        {
            if (volume_controller->info[INDEX].mute)
            {
                volume_controller->info[INDEX].volume = 0;
                printf("mute: %d\n", volume_controller->info[INDEX].volume);
                volume_controller->info[INDEX].mute = false;
    
            }
        } break;
        case 1:
        {
            volume_controller->info[INDEX].colors.r = 0;
            volume_controller->info[INDEX].colors.g = 255;
            volume_controller->info[INDEX].colors.b = 0;
            volume_controller->info[INDEX].colors.a = 255;
            
            if (volume_controller->info[INDEX].one)
            {
                volume_controller->info[INDEX].volume = 32;
                printf("mute: %d\n", volume_controller->info[INDEX].volume);
                volume_controller->info[INDEX].one = false;
            }
        } break;
        case 2:
        {
            volume_controller->info[INDEX].colors.r = 255;
            volume_controller->info[INDEX].colors.g = 255;
            volume_controller->info[INDEX].colors.b = 0;
            volume_controller->info[INDEX].colors.a = 255;

            if (volume_controller->info[INDEX].two)
            {
                volume_controller->info[INDEX].volume = 64;
                printf("two: %d\n", volume_controller->info[INDEX].volume);
                volume_controller->info[INDEX].two = false;
            }
        } break;
        case 3:
        {
            volume_controller->info[INDEX].colors.r = 255;
            volume_controller->info[INDEX].colors.g = 165;
            volume_controller->info[INDEX].colors.b = 0;
            volume_controller->info[INDEX].colors.a = 255;

            if (volume_controller->info[INDEX].three)
            {
                volume_controller->info[INDEX].volume = 96;
                printf("three: %d\n", volume_controller->info[INDEX].volume);
                volume_controller->info[INDEX].three = false;
            }
        } break;
        case 4:
        {
            volume_controller->info[INDEX].colors.r = 255;
            volume_controller->info[INDEX].colors.g = 0;
            volume_controller->info[INDEX].colors.b = 0;
            volume_controller->info[INDEX].colors.a = 255;
            
            if (volume_controller->info[INDEX].max)
            {
                volume_controller->info[INDEX].volume = SDL_MIX_MAXVOLUME;
                printf("max: %d\n", volume_controller->info[INDEX].volume);
                volume_controller->info[INDEX].max = false;
            }
        } break;
    }

}

