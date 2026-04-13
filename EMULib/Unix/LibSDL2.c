/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        LibSDL2.c                        **/
/**                                                         **/
/** This file contains SDL2-dependent implementation        **/
/** parts of the emulation library.                         **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2021                 **/
/**               Leandro Pereira 2024                      **/
/*************************************************************/
#include "EMULib.h"
#include "Sound.h"
#include "Console.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern int MasterSwitch;
extern int MasterVolume;

static SDL_Window *SDLWindow = NULL;
static SDL_Renderer *Renderer = NULL;
static SDL_Texture *Texture   = NULL;

static int TimerReady = 0;
static unsigned int JoyState = 0;
static unsigned int LastKey = 0;

static int Effects = EFF_SCALE | EFF_SAVECPU;
static int XSize, YSize;

int ARGC = 0;
char **ARGV = NULL;

static Uint32 TimerCallback(Uint32 interval, void *param) {
    TimerReady = 1;
    return interval;
}

int InitUnix(const char *Title, int Width, int Height) {
    XSize = Width;
    YSize = Height;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        return 0;
    }

    SDLWindow = SDL_CreateWindow(Title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Width, Height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!SDLWindow) return 0;

    Renderer = SDL_CreateRenderer(SDLWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!Renderer) Renderer = SDL_CreateRenderer(SDLWindow, -1, 0);
    if (!Renderer) return 0;

    SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
    SDL_RenderClear(Renderer);
    SDL_RenderPresent(Renderer);

    return 1;
}

void TrashUnix(void) {
    if (Texture) SDL_DestroyTexture(Texture);
    if (Renderer) SDL_DestroyRenderer(Renderer);
    if (SDLWindow) SDL_DestroyWindow(SDLWindow);
    SDL_Quit();
}

int ProcessEvents(int Wait) {
    SDL_Event Event;

    if (Wait) SDL_WaitEvent(&Event);

    while (SDL_PollEvent(&Event)) {
        switch (Event.type) {
            case SDL_QUIT:
                return 0;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                {
                    unsigned int Key = Event.key.keysym.sym;
                    /* Map SDL keys to fMSX/EMULib expected keys if necessary */
                    /* For now, just pass the sym. */
                    if (Event.type == SDL_KEYUP) Key |= CON_RELEASE;
                    LastKey = Key;
                    if (KeyHandler) KeyHandler(Key);
                }
                break;
        }
    }
    return 1;
}

void SetEffects(unsigned int NewEffects) {
    Effects = NewEffects;
}

unsigned int ParseEffects(char *Args[], unsigned int Effects) {
  /* Stub for now */
  return Effects;
}

int ShowVideo(void) {
    if (!VideoImg || !VideoImg->Data || !Renderer) return 0;

    if (!Texture || VideoImg->W != XSize || VideoImg->H != YSize) {
        if (Texture) SDL_DestroyTexture(Texture);
        Texture = SDL_CreateTexture(Renderer, 
#if defined(BPP32)
            SDL_PIXELFORMAT_ARGB8888,
#elif defined(BPP16)
            SDL_PIXELFORMAT_RGB565,
#else
            SDL_PIXELFORMAT_RGB332,
#endif
            SDL_TEXTUREACCESS_STREAMING, VideoImg->W, VideoImg->H);
    }

    SDL_UpdateTexture(Texture, NULL, VideoImg->Data, VideoImg->W * (VideoImg->D >> 3));
    SDL_RenderClear(Renderer);
    SDL_RenderCopy(Renderer, Texture, NULL, NULL);
    SDL_RenderPresent(Renderer);

    return 1;
}

int SetSyncTimer(int Hz) {
    if (Hz <= 0) return 0;
    SDL_AddTimer(1000 / Hz, TimerCallback, NULL);
    return 1;
}

int WaitSyncTimer(void) {
    while (!TimerReady) {
        SDL_Delay(1);
        ProcessEvents(0);
    }
    TimerReady = 0;
    return 1;
}

unsigned int GetJoystick(void) {
    return JoyState;
}

unsigned int GetKey(void) {
    unsigned int J = LastKey;
    LastKey = 0;
    return J;
}

unsigned int WaitKey(void) {
    while (!LastKey) ProcessEvents(1);
    return GetKey();
}

unsigned int GetMouse(void) {
    int X, Y;
    Uint32 Buttons = SDL_GetMouseState(&X, &Y);
    unsigned int J = 0;

    if (Buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) J |= MSE_LEFT;
    if (Buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) J |= MSE_RIGHT;

    return J | (X & 0xFFFF) | ((Y & 0x3FFF) << 16);
}

unsigned int WaitKeyOrMouse(void) {
    unsigned int J;
    while (!(J = GetMouse() & MSE_BUTTONS) && !LastKey) ProcessEvents(1);
    return J? GetMouse():GetKey();
}

unsigned int X11GetColor(unsigned char R, unsigned char G, unsigned char B) {
#if defined(BPP32)
    return (R << 16) | (G << 8) | B;
#elif defined(BPP16)
    return (((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3));
#else
    return ((R & 0xE0) | ((G >> 3) & 0x1C) | (B >> 6));
#endif
}
