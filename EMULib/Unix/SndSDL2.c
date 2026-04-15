/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        SndSDL2.c                        **/
/**                                                         **/
/** This file contains SDL2-dependent sound implementation. **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2021                 **/
/**               Leandro Pereira 2024                      **/
/*************************************************************/
#include "EMULib.h"
#include "Sound.h"
#include <SDL2/SDL.h>

static SDL_AudioDeviceID AudioDevice = 0;
static SDL_AudioSpec AudioSpec;

unsigned int InitAudio(unsigned int Rate, unsigned int Latency) {
    SDL_AudioSpec Desired;

    if (AudioDevice) SDL_CloseAudioDevice(AudioDevice);

    SDL_memset(&Desired, 0, sizeof(Desired));
    Desired.freq = Rate;
#ifdef BPS16
    Desired.format = AUDIO_S16SYS;
#else
    Desired.format = AUDIO_S8;
#endif
    Desired.channels = 1;

    /* Force buffer size to be a power of 2 for better stability */
    unsigned int Samples = 1;
    while(Samples < (Rate * Latency / 1000)) Samples <<= 1;
    Desired.samples = Samples;

    /* Open audio device, allowing only frequency changes if necessary */
    AudioDevice = SDL_OpenAudioDevice(NULL, 0, &Desired, &AudioSpec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (!AudioDevice) {
        return 0;
    }

    SDL_PauseAudioDevice(AudioDevice, 0);
    
    /* Return the actual frequency obtained from SDL */
    return AudioSpec.freq;
}

void TrashAudio(void) {
    if (AudioDevice) {
        SDL_CloseAudioDevice(AudioDevice);
        AudioDevice = 0;
    }
}

unsigned int WriteAudio(sample *Data, unsigned int Length) {
    if (!AudioDevice) return 0;
    
    /* If there's more than 500ms of audio queued, clear it to eliminate lag */
    if (SDL_GetQueuedAudioSize(AudioDevice) > (AudioSpec.freq * sizeof(sample) / 2)) {
        SDL_ClearQueuedAudio(AudioDevice);
    }

    if (SDL_QueueAudio(AudioDevice, Data, Length * sizeof(sample)) < 0) return 0;
    return Length;
}

unsigned int GetFreeAudio(void) {
    if (!AudioDevice) return 0;
    
    unsigned int Queued = SDL_GetQueuedAudioSize(AudioDevice);
    unsigned int Max    = AudioSpec.samples * sizeof(sample) * 4; /* Allow some headroom */
    
    if (Queued >= Max) return 0;
    return (Max - Queued) / sizeof(sample);
}

int PauseAudio(int Switch) {
    if (!AudioDevice) return 0;
    SDL_PauseAudioDevice(AudioDevice, Switch);
    return 1;
}
