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

static SDL_AudioSpec AudioSpec;
static int AudioOpened = 0;

unsigned int InitAudio(unsigned int Rate, unsigned int Latency) {
    if (AudioOpened) SDL_CloseAudio();

    SDL_memset(&AudioSpec, 0, sizeof(AudioSpec));
    AudioSpec.freq = Rate;
#ifdef BPS16
    AudioSpec.format = AUDIO_S16SYS;
#else
    AudioSpec.format = AUDIO_S8;
#endif
    AudioSpec.channels = 1;

    /* Force buffer size to be a power of 2 for better stability */
    unsigned int Samples = 1;
    while(Samples < (Rate * Latency / 1000)) Samples <<= 1;
    AudioSpec.samples = Samples;

    if (SDL_OpenAudio(&AudioSpec, NULL) < 0) {
        return 0;
    }

    SDL_PauseAudio(0);
    AudioOpened = 1;
    return Rate;
}

void TrashAudio(void) {
    if (AudioOpened) {
        SDL_CloseAudio();
        AudioOpened = 0;
    }
}

unsigned int WriteAudio(sample *Data, unsigned int Length) {
    if (!AudioOpened) return 0;
    
    /* If there's more than 500ms of audio queued, clear it to eliminate lag */
    if (SDL_GetQueuedAudioSize(1) > (AudioSpec.freq * sizeof(sample) / 2)) {
        SDL_ClearQueuedAudio(1);
    }

    if (SDL_QueueAudio(1, Data, Length * sizeof(sample)) < 0) return 0;
    return Length;
}

unsigned int GetFreeAudio(void) {
    if (!AudioOpened) return 0;
    
    unsigned int Queued = SDL_GetQueuedAudioSize(1);
    unsigned int Max    = AudioSpec.samples * sizeof(sample);
    
    /* If the queue is already filled with 2x the buffer size, no more space */
    if (Queued >= (Max * 2)) return 0;
    
    /* Return remaining space in samples (not bytes) */
    return ((Max * 2) - Queued) / sizeof(sample);
}

int PauseAudio(int Switch) {
    if (!AudioOpened) return 0;
    SDL_PauseAudio(Switch);
    return 1;
}
