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
#include <signal.h>

extern int MasterSwitch;
extern int MasterVolume;
extern int ExitNow;

static SDL_Window *SDLWindow = NULL;
static SDL_Renderer *Renderer = NULL;
static SDL_Texture *Texture   = NULL;

static int TimerReady = 0;
static unsigned int JoyState = 0;
static unsigned int LastKey = 0;

static int Effects = EFF_SCALE | EFF_SAVECPU;

int ARGC = 0;
char **ARGV = NULL;

static void SigHandler(int Signum) {
  ExitNow = 1;
}

static Uint32 TimerCallback(Uint32 interval, void *param) {
    TimerReady = 1;
    return interval;
}

int InitUnix(const char *Title, int Width, int Height) {
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

    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);

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
                ExitNow = 1;
                return 0;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                {
                    unsigned int Key = Event.key.keysym.sym;
                    if (Event.type == SDL_KEYUP) Key |= CON_RELEASE;
                    LastKey = Key;
                    if (KeyHandler) KeyHandler(Key);
                }
                break;
        }
    }
    return !ExitNow;
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

    int TW = 0, TH = 0;
    if (Texture) SDL_QueryTexture(Texture, NULL, NULL, &TW, &TH);

    if (!Texture || VideoImg->W != TW || VideoImg->H != TH) {
        if (Texture) SDL_DestroyTexture(Texture);
        
        Uint32 Format;
        switch (VideoImg->D) {
            case 32: Format = SDL_PIXELFORMAT_ARGB8888; break;
            case 16: Format = SDL_PIXELFORMAT_RGB565; break;
            case 8:  Format = SDL_PIXELFORMAT_RGB332; break;
            default: Format = SDL_PIXELFORMAT_UNKNOWN; break;
        }

        Texture = SDL_CreateTexture(Renderer, Format, SDL_TEXTUREACCESS_STREAMING, VideoImg->W, VideoImg->H);
    }

    if (!Texture) return 0;

    SDL_UpdateTexture(Texture, NULL, VideoImg->Data, VideoImg->L * (VideoImg->D >> 3));
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
    while (!TimerReady && !ExitNow) {
        SDL_Delay(1);
        ProcessEvents(0);
    }
    TimerReady = 0;
    return !ExitNow;
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
    while (!LastKey && !ExitNow) ProcessEvents(1);
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
    while (!(J = GetMouse() & MSE_BUTTONS) && !LastKey && !ExitNow) ProcessEvents(1);
    return J? GetMouse():GetKey();
}

unsigned int X11GetColor(unsigned char R, unsigned char G, unsigned char B) {
    /* Use the same formulas as EMULib/Unix/LibUnix.h for consistency */
#if defined(BPP32) || !defined(PIXEL)
    return (unsigned int)(((int)R<<16)|((int)G<<8)|B);
#elif defined(BPP16)
    return (unsigned int)(((31*(R)/255)<<11)|((63*(G)/255)<<5)|(31*(B)/255));
#elif defined(BPP8)
    return (unsigned int)(((7*(R)/255)<<5)|((7*(G)/255)<<2)|(3*(B)/255));
#else
    return (unsigned int)(((int)R<<16)|((int)G<<8)|B);
#endif
}
