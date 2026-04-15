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

/* Define keysyms locally to avoid header dependency */
#define XK_BackSpace 0xff08
#define XK_Tab       0xff09
#define XK_Return    0xff0d
#define XK_Escape    0xff1b
#define XK_Delete    0xffff
#define XK_Home      0xff50
#define XK_Left      0xff51
#define XK_Up        0xff52
#define XK_Right     0xff53
#define XK_Down      0xff54
#define XK_Page_Up   0xff55
#define XK_Page_Down 0xff56
#define XK_End       0xff57
#define XK_Insert    0xff63
#define XK_F1        0xffbe
#define XK_F2        0xffbf
#define XK_F3        0xffc0
#define XK_F4        0xffc1
#define XK_F5        0xffc2
#define XK_F6        0xffc3
#define XK_F7        0xffc4
#define XK_F8        0xffc5
#define XK_F9        0xffc6
#define XK_F10       0xffc7
#define XK_F11       0xffc8
#define XK_F12       0xffc9
#define XK_Shift_L   0xffe1
#define XK_Shift_R   0xffe2
#define XK_Control_L 0xffe3
#define XK_Control_R 0xffe4
#define XK_Caps_Lock 0xffe5
#define XK_Alt_L     0xffe9
#define XK_Alt_R     0xffea

#define XK_KP_0      0xffb0
#define XK_KP_1      0xffb1
#define XK_KP_2      0xffb2
#define XK_KP_3      0xffb3
#define XK_KP_4      0xffb4
#define XK_KP_5      0xffb5
#define XK_KP_6      0xffb6
#define XK_KP_7      0xffb7
#define XK_KP_8      0xffb8
#define XK_KP_9      0xffb9
#define XK_KP_Enter  0xff8d

#define MSX_AUTOFIREA 0x01000000
#define MSX_AUTOFIREB 0x02000000

extern int MasterSwitch;
extern int MasterVolume;
extern int ExitNow;
extern unsigned char Verbose;

static SDL_Window *SDLWindow = NULL;
static SDL_Renderer *Renderer = NULL;
static SDL_Texture *Texture   = NULL;
static SDL_GameController *Controllers[2] = { NULL, NULL };

static Uint64 NextFrameTime = 0;
static Uint64 FrameDuration = 0;
static unsigned int JoyState = 0;
static unsigned int LastKey = 0;

static int Effects = EFF_SCALE | EFF_SAVECPU;

static char FPSString[32] = "";
static Uint32 LastFPSTime = 0;
static int FrameCount = 0;

extern int SyncFreq;

int ARGC = 0;
char **ARGV = NULL;

static void SigHandler(int Signum) {
  ExitNow = 1;
}

static unsigned int SDLToKeysym(SDL_Keycode Key) {
    switch (Key) {
        case SDLK_BACKSPACE: return CON_BS;
        case SDLK_TAB:       return CON_TAB;
        case SDLK_RETURN:    return CON_OK;
        case SDLK_ESCAPE:    return CON_EXIT;
        case SDLK_DELETE:    return CON_DELETE;
        case SDLK_UP:        return CON_UP;
        case SDLK_DOWN:      return CON_DOWN;
        case SDLK_LEFT:      return CON_LEFT;
        case SDLK_RIGHT:     return CON_RIGHT;
        case SDLK_F1:        return CON_F1;
        case SDLK_F2:        return CON_F2;
        case SDLK_F3:        return CON_F3;
        case SDLK_F4:        return CON_F4;
        case SDLK_F5:        return CON_F5;
        case SDLK_F6:        return CON_F6;
        case SDLK_F7:        return CON_F7;
        case SDLK_F8:        return CON_F8;
        case SDLK_F9:        return CON_F9;
        case SDLK_F10:       return CON_F10;
        case SDLK_F11:       return CON_F11;
        case SDLK_F12:       return CON_F12;
        case SDLK_LSHIFT:    return XK_Shift_L;
        case SDLK_RSHIFT:    return XK_Shift_R;
        case SDLK_LCTRL:     return XK_Control_L;
        case SDLK_RCTRL:     return XK_Control_R;
        case SDLK_LALT:      return XK_Alt_L;
        case SDLK_RALT:      return XK_Alt_R;
        case SDLK_CAPSLOCK:  return XK_Caps_Lock;
        case SDLK_INSERT:    return CON_INSERT;
        case SDLK_HOME:      return CON_HOME;
        case SDLK_END:       return CON_END;
        case SDLK_PAGEUP:    return CON_PAGEUP;
        case SDLK_PAGEDOWN:  return CON_PAGEDOWN;
        case SDLK_KP_0:      return XK_KP_0;
        case SDLK_KP_1:      return XK_KP_1;
        case SDLK_KP_2:      return XK_KP_2;
        case SDLK_KP_3:      return XK_KP_3;
        case SDLK_KP_4:      return XK_KP_4;
        case SDLK_KP_5:      return XK_KP_5;
        case SDLK_KP_6:      return XK_KP_6;
        case SDLK_KP_7:      return XK_KP_7;
        case SDLK_KP_8:      return XK_KP_8;
        case SDLK_KP_9:      return XK_KP_9;
        case SDLK_KP_ENTER:  return CON_OK;
        
        /* Map common symbols explicitly */
        case SDLK_QUOTE:     return '\'';
        case SDLK_QUOTEDBL:  return '"';
        case SDLK_BACKQUOTE: return '`';
        case SDLK_HASH:      return '#';
        case SDLK_PERCENT:   return '%';
        case SDLK_AMPERSAND: return '&';
        case SDLK_LEFTPAREN: return '(';
        case SDLK_RIGHTPAREN: return ')';
        case SDLK_ASTERISK:  return '*';
        case SDLK_PLUS:      return '+';
        case SDLK_COMMA:     return ',';
        case SDLK_MINUS:     return '-';
        case SDLK_PERIOD:    return '.';
        case SDLK_SLASH:     return '/';
        case SDLK_COLON:     return ':';
        case SDLK_SEMICOLON: return ';';
        case SDLK_LESS:      return '<';
        case SDLK_EQUALS:    return '=';
        case SDLK_GREATER:   return '>';
        case SDLK_QUESTION:  return '?';
        case SDLK_AT:        return '@';
        case SDLK_LEFTBRACKET: return '[';
        case SDLK_BACKSLASH: return '\\';
        case SDLK_RIGHTBRACKET: return ']';
        case SDLK_CARET:     return '^';
        case SDLK_UNDERSCORE: return '_';
    }
    if (Key < 128) return (unsigned int)Key;
    return 0;
}

int InitUnix(const char *Title, int Width, int Height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        return 0;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    SDLWindow = SDL_CreateWindow(Title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Width, Height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!SDLWindow) return 0;

    Renderer = SDL_CreateRenderer(SDLWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!Renderer) Renderer = SDL_CreateRenderer(SDLWindow, -1, 0);
    if (!Renderer) Renderer = SDL_CreateRenderer(SDLWindow, -1, SDL_RENDERER_SOFTWARE);
    if (!Renderer) return 0;

    SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
    SDL_RenderClear(Renderer);
    SDL_RenderPresent(Renderer);

    /* Open available game controllers or joysticks */
    int ControllerIdx = 0;
    int NumJoysticks = SDL_NumJoysticks();
    if (NumJoysticks > 0) printf("SDL: Found %d joystick(s)\n", NumJoysticks);

    for (int i = 0; i < NumJoysticks && ControllerIdx < 2; ++i) {
        if (SDL_IsGameController(i)) {
            Controllers[ControllerIdx] = SDL_GameControllerOpen(i);
            if (Controllers[ControllerIdx]) {
                printf("SDL: Opened GameController %d: %s\n", ControllerIdx, SDL_GameControllerName(Controllers[ControllerIdx]));
                ControllerIdx++;
            }
        } else {
            /* Fallback to generic joystick if no GameController mapping exists */
            SDL_Joystick *Joy = SDL_JoystickOpen(i);
            if (Joy) {
                printf("SDL: Opened generic Joystick %d: %s\n", ControllerIdx, SDL_JoystickName(Joy));
                /* We still want to use it, but generic joysticks need different handling. 
                   For now, we just log it. In a full implementation, we'd add SDL_JOYBUTTONDOWN etc. */
                SDL_JoystickClose(Joy);
                /* Try to force it as a GameController anyway if it has enough buttons */
                Controllers[ControllerIdx] = SDL_GameControllerOpen(i);
                if (Controllers[ControllerIdx]) ControllerIdx++;
            }
        }
    }

    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);

    return 1;
}

void TrashUnix(void) {
    for (int i = 0; i < 2; ++i) {
        if (Controllers[i]) {
            SDL_GameControllerClose(Controllers[i]);
            Controllers[i] = NULL;
        }
    }
    if (Texture) SDL_DestroyTexture(Texture);
    if (Renderer) SDL_DestroyRenderer(Renderer);
    if (SDLWindow) SDL_DestroyWindow(SDLWindow);
    SDL_Quit();
}

static void HandleSDLEvent(SDL_Event *Event) {
    switch (Event->type) {
        case SDL_QUIT:
            ExitNow = 1;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            {
                unsigned int Key = SDLToKeysym(Event->key.keysym.sym);
                if (Key) {
                    if (Event->type == SDL_KEYUP) Key |= CON_RELEASE;
                    
                    SDL_Keymod Mod = SDL_GetModState();
                    if (Mod & KMOD_SHIFT) Key |= CON_SHIFT;
                    if (Mod & KMOD_CTRL)  Key |= CON_CONTROL;
                    if (Mod & KMOD_ALT)   Key |= CON_ALT;
                    if (Mod & KMOD_CAPS)  Key |= CON_CAPS;

                    if (Verbose & 64) {
                        printf("SDL Key: %s (0x%X) -> EMULib: 0x%X [%s]\n",
                            SDL_GetKeyName(Event->key.keysym.sym),
                            (unsigned int)Event->key.keysym.sym,
                            Key,
                            (Key & CON_RELEASE) ? "RELEASE" : "PRESS");
                    }

                    if (!(Key & CON_RELEASE)) {
                        LastKey = Key;
                        switch (Event->key.keysym.sym) {
                            case SDLK_UP:        JoyState |= BTN_UP; break;
                            case SDLK_DOWN:      JoyState |= BTN_DOWN; break;
                            case SDLK_LEFT:      JoyState |= BTN_LEFT; break;
                            case SDLK_RIGHT:     JoyState |= BTN_RIGHT; break;
                            case SDLK_LSHIFT:
                            case SDLK_RSHIFT:    JoyState |= BTN_SHIFT; break;
                            case SDLK_LCTRL:
                            case SDLK_RCTRL:     JoyState |= BTN_CONTROL; break;
                            case SDLK_LALT:
                            case SDLK_RALT:      JoyState |= BTN_ALT; break;
                            case SDLK_SPACE:
                            case SDLK_a: case SDLK_s: case SDLK_d: case SDLK_f:
                            case SDLK_g: case SDLK_h: case SDLK_j: case SDLK_k:
                            case SDLK_l:         JoyState |= BTN_FIREA; break;
                            case SDLK_z: case SDLK_x: case SDLK_c: case SDLK_v:
                            case SDLK_b: case SDLK_n: case SDLK_m:
                                                 JoyState |= BTN_FIREB; break;
                            case SDLK_q: case SDLK_e: case SDLK_t:
                            case SDLK_u: case SDLK_o:
                                                 JoyState |= BTN_FIREL; break;
                            case SDLK_w: case SDLK_r: case SDLK_y:
                            case SDLK_i: case SDLK_p:
                                                 JoyState |= BTN_FIRER; break;
                            case SDLK_RETURN:    JoyState |= BTN_START; break;
                            case SDLK_TAB:       JoyState |= BTN_SELECT; break;
                            case SDLK_ESCAPE:    JoyState |= BTN_EXIT; break;
                        }
                    } else {
                        switch (Event->key.keysym.sym) {
                            case SDLK_UP:        JoyState &= ~BTN_UP; break;
                            case SDLK_DOWN:      JoyState &= ~BTN_DOWN; break;
                            case SDLK_LEFT:      JoyState &= ~BTN_LEFT; break;
                            case SDLK_RIGHT:     JoyState &= ~BTN_RIGHT; break;
                            case SDLK_LSHIFT:
                            case SDLK_RSHIFT:    JoyState &= ~BTN_SHIFT; break;
                            case SDLK_LCTRL:
                            case SDLK_RCTRL:     JoyState &= ~BTN_CONTROL; break;
                            case SDLK_LALT:
                            case SDLK_RALT:      JoyState &= ~BTN_ALT; break;
                            case SDLK_SPACE:
                            case SDLK_a: case SDLK_s: case SDLK_d: case SDLK_f:
                            case SDLK_g: case SDLK_h: case SDLK_j: case SDLK_k:
                            case SDLK_l:         JoyState &= ~BTN_FIREA; break;
                            case SDLK_z: case SDLK_x: case SDLK_c: case SDLK_v:
                            case SDLK_b: case SDLK_n: case SDLK_m:
                                                 JoyState &= ~BTN_FIREB; break;
                            case SDLK_q: case SDLK_e: case SDLK_t:
                            case SDLK_u: case SDLK_o:
                                                 JoyState &= ~BTN_FIREL; break;
                            case SDLK_w: case SDLK_r: case SDLK_y:
                            case SDLK_i: case SDLK_p:
                                                 JoyState &= ~BTN_FIRER; break;
                            case SDLK_RETURN:    JoyState &= ~BTN_START; break;
                            case SDLK_TAB:       JoyState &= ~BTN_SELECT; break;
                            case SDLK_ESCAPE:    JoyState &= ~BTN_EXIT; break;
                        }
                    }
                    if (KeyHandler) KeyHandler(Key);
                }
            }
            break;
        case SDL_WINDOWEVENT:
            if (Event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                Event->window.event == SDL_WINDOWEVENT_EXPOSED ||
                Event->window.event == SDL_WINDOWEVENT_RESIZED) {
                ShowVideo();
            }
            break;

        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            {
                int Pressed = (Event->type == SDL_CONTROLLERBUTTONDOWN);
                unsigned int Mask = 0;
                unsigned int Key = 0;

                switch (Event->cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:    Mask = BTN_UP; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  Mask = BTN_DOWN; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  Mask = BTN_LEFT; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: Mask = BTN_RIGHT; break;
                    
                    /* A (Right) = Fire 1, B (Bottom) = Fire 2 */
                    case SDL_CONTROLLER_BUTTON_A:          Mask = BTN_FIREA; break;
                    case SDL_CONTROLLER_BUTTON_B:          Mask = BTN_FIREB; break;
                    
                    /* X (Top) = F4, Y (Left) = F3 */
                    case SDL_CONTROLLER_BUTTON_X:          Key = CON_F4; break;
                    case SDL_CONTROLLER_BUTTON_Y:          Key = CON_F3; break;
                    
                    /* Shoulder buttons for 6-button support (L/R and X/Y bits) */
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  Mask = BTN_FIREL; break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: Mask = BTN_FIRER; break;
                    case SDL_CONTROLLER_BUTTON_LEFTSTICK:     Mask = BTN_FIREX; break; /* Mapping L2/R2 equivalent */
                    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    Mask = BTN_FIREY; break;
                    
                    /* Start = F1, Select = F2 */
                    case SDL_CONTROLLER_BUTTON_START:      Key = CON_F1; break;
                    case SDL_CONTROLLER_BUTTON_BACK:       Key = CON_F2; break;
                }
                
                if (Mask) {
                    if (Event->cbutton.which > 0) Mask <<= 16;
                    if (Pressed) JoyState |= Mask; else JoyState &= ~Mask;
                }
                
                if (Key && KeyHandler) {
                    if (!Pressed) Key |= CON_RELEASE;
                    KeyHandler(Key);
                }
            }
            break;

        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            {
                int Pressed = (Event->type == SDL_JOYBUTTONDOWN);
                unsigned int Mask = 0;
                /* Generic joystick button mapping (very basic fallback) */
                switch (Event->jbutton.button) {
                    case 0: Mask = BTN_FIREA; break;
                    case 1: Mask = BTN_FIREB; break;
                    case 2: Mask = BTN_FIREL; break;
                    case 3: Mask = BTN_FIRER; break;
                    case 4: Mask = BTN_SELECT; break;
                    case 5: Mask = BTN_START; break;
                }
                if (Mask) {
                    if (Event->jbutton.which > 0) Mask <<= 16;
                    if (Pressed) JoyState |= Mask; else JoyState &= ~Mask;
                }
            }
            break;

        case SDL_CONTROLLERAXISMOTION:
            {
                int Val = Event->caxis.value;
                int Threshold = 16384;
                
                if (Event->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                    if (Val > Threshold) JoyState |= (Event->caxis.which > 0 ? (BTN_RIGHT << 16) : BTN_RIGHT);
                    else JoyState &= ~(Event->caxis.which > 0 ? (BTN_RIGHT << 16) : BTN_RIGHT);
                    
                    if (Val < -Threshold) JoyState |= (Event->caxis.which > 0 ? (BTN_LEFT << 16) : BTN_LEFT);
                    else JoyState &= ~(Event->caxis.which > 0 ? (BTN_LEFT << 16) : BTN_LEFT);
                } else if (Event->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                    if (Val > Threshold) JoyState |= (Event->caxis.which > 0 ? (BTN_DOWN << 16) : BTN_DOWN);
                    else JoyState &= ~(Event->caxis.which > 0 ? (BTN_DOWN << 16) : BTN_DOWN);
                    
                    if (Val < -Threshold) JoyState |= (Event->caxis.which > 0 ? (BTN_UP << 16) : BTN_UP);
                    else JoyState &= ~(Event->caxis.which > 0 ? (BTN_UP << 16) : BTN_UP);
                }
            }
            break;

        case SDL_JOYAXISMOTION:
            {
                int Val = Event->jaxis.value;
                int Threshold = 16384;
                if (Event->jaxis.axis == 0) { /* X Axis */
                    if (Val > Threshold) JoyState |= (Event->jaxis.which > 0 ? (BTN_RIGHT << 16) : BTN_RIGHT);
                    else JoyState &= ~(Event->jaxis.which > 0 ? (BTN_RIGHT << 16) : BTN_RIGHT);
                    if (Val < -Threshold) JoyState |= (Event->jaxis.which > 0 ? (BTN_LEFT << 16) : BTN_LEFT);
                    else JoyState &= ~(Event->jaxis.which > 0 ? (BTN_LEFT << 16) : BTN_LEFT);
                } else if (Event->jaxis.axis == 1) { /* Y Axis */
                    if (Val > Threshold) JoyState |= (Event->jaxis.which > 0 ? (BTN_DOWN << 16) : BTN_DOWN);
                    else JoyState &= ~(Event->jaxis.which > 0 ? (BTN_DOWN << 16) : BTN_DOWN);
                    if (Val < -Threshold) JoyState |= (Event->jaxis.which > 0 ? (BTN_UP << 16) : BTN_UP);
                    else JoyState &= ~(Event->jaxis.which > 0 ? (BTN_UP << 16) : BTN_UP);
                }
            }
            break;

        case SDL_CONTROLLERDEVICEADDED:
            {
                int i = Event->cdevice.which;
                if (SDL_IsGameController(i)) {
                    for (int j = 0; j < 2; ++j) {
                        if (!Controllers[j]) {
                            Controllers[j] = SDL_GameControllerOpen(i);
                            if (Verbose) printf("SDL: Controller %d connected: %s\n", j, SDL_GameControllerName(Controllers[j]));
                            break;
                        }
                    }
                }
            }
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            {
                SDL_GameController *C = SDL_GameControllerFromInstanceID(Event->cdevice.which);
                if (C) {
                    for (int j = 0; j < 2; ++j) {
                        if (Controllers[j] == C) {
                            if (Verbose) printf("SDL: Controller %d disconnected\n", j);
                            SDL_GameControllerClose(C);
                            Controllers[j] = NULL;
                            break;
                        }
                    }
                }
            }
            break;
    }
}

int ProcessEvents(int Wait) {
    SDL_Event Event;

    if (Wait) {
        if (SDL_WaitEvent(&Event)) HandleSDLEvent(&Event);
    }

    while (SDL_PollEvent(&Event)) {
        HandleSDLEvent(&Event);
    }
    return !ExitNow;
}

void SetEffects(unsigned int NewEffects) {
    Effects = NewEffects;
}

unsigned int ParseEffects(char *Args[], unsigned int Effects) {
  int J;
  for (J = 1; Args[J]; ++J) {
    if (!strcmp(Args[J], "-soften")) Effects |= EFF_SOFTEN;
    else if (!strcmp(Args[J], "-nosoften")) Effects &= ~EFF_SOFTEN;
    else if (!strcmp(Args[J], "-scanlines")) Effects |= EFF_TVLINES;
    else if (!strcmp(Args[J], "-noscanlines")) Effects &= ~EFF_TVLINES;
    else if (!strcmp(Args[J], "-sync")) Effects |= EFF_SYNC;
    else if (!strcmp(Args[J], "-nosync")) Effects &= ~EFF_SYNC;
    else if (!strcmp(Args[J], "-4x3")) Effects |= EFF_4X3;
  }
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
            case 32: Format = SDL_PIXELFORMAT_XRGB8888; break;
            case 24: Format = SDL_PIXELFORMAT_RGB24; break;
            case 16: Format = SDL_PIXELFORMAT_RGB565; break;
            case 8:  Format = SDL_PIXELFORMAT_RGB332; break;
            default: Format = SDL_PIXELFORMAT_UNKNOWN; break;
        }

        Texture = SDL_CreateTexture(Renderer, Format, SDL_TEXTUREACCESS_STREAMING, VideoImg->W, VideoImg->H);
    }

    if (!Texture) return 0;

    /* If showing FPS, calculate and print it */
    if (Effects & EFF_SHOWFPS) {
        Uint32 Now = SDL_GetTicks();
        FrameCount++;
        if (Now - LastFPSTime >= 1000) {
            sprintf(FPSString, "%d/%d FPS", (int)(FrameCount * 1000.0 / (Now - LastFPSTime)), SyncFreq);
            LastFPSTime = Now;
            FrameCount = 0;
        }
        if (FPSString[0]) {
            ShadowPrintXY(VideoImg, FPSString, VideoX + VideoW - strlen(FPSString) * 8 - 8, VideoY + VideoH - 16, PIXEL(255, 255, 255), PIXEL(0, 0, 0));
        }
    }

    SDL_UpdateTexture(Texture, NULL, VideoImg->Data, VideoImg->L * (VideoImg->D >> 3));
    
    SDL_Rect SrcRect = { VideoX, VideoY, VideoW, VideoH };
    SDL_Rect DstRect = { 0, 0, 0, 0 };
    int WW, WH;

    SDL_GetRendererOutputSize(Renderer, &WW, &WH);
    
    if (Effects & EFF_4X3) {
        float Aspect = 4.0f / 3.0f;
        if ((float)WW / WH > Aspect) {
            DstRect.h = WH;
            DstRect.w = (int)(WH * Aspect);
            DstRect.x = (WW - DstRect.w) / 2;
            DstRect.y = 0;
        } else {
            DstRect.w = WW;
            DstRect.h = (int)(WW / Aspect);
            DstRect.x = 0;
            DstRect.y = (WH - DstRect.h) / 2;
        }
    } else {
        DstRect.w = WW;
        DstRect.h = WH;
        DstRect.x = 0;
        DstRect.y = 0;
    }
    
    SDL_RenderSetViewport(Renderer, NULL);
    SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
    SDL_RenderClear(Renderer);
    SDL_RenderCopy(Renderer, Texture, &SrcRect, &DstRect);

    /* Draw scanlines over the rendered image if enabled */
    if (Effects & EFF_TVLINES) {
        SDL_SetRenderDrawBlendMode(Renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 48); /* ~20% transparency */
        for (int y = DstRect.y; y < DstRect.y + DstRect.h; y += 2) {
            SDL_RenderDrawLine(Renderer, DstRect.x, y, DstRect.x + DstRect.w, y);
        }
    }

    SDL_RenderPresent(Renderer);

    /* Wait for sync timer if requested */
    if (Effects & EFF_SYNC) WaitSyncTimer();

    return 1;
}

int SetSyncTimer(int Hz) {
    if (Hz <= 0) {
        FrameDuration = 0;
        return 0;
    }
    FrameDuration = SDL_GetPerformanceFrequency() / Hz;
    NextFrameTime = SDL_GetPerformanceCounter() + FrameDuration;
    return 1;
}

int WaitSyncTimer(void) {
    if (FrameDuration == 0) return 1;

    Uint64 Now = SDL_GetPerformanceCounter();
    if (Now < NextFrameTime) {
        /* coarse wait */
        Uint32 DelayMs = (Uint32)((NextFrameTime - Now) * 1000 / SDL_GetPerformanceFrequency());
        if (DelayMs > 1) SDL_Delay(DelayMs - 1);
        /* fine busy-wait */
        while (SDL_GetPerformanceCounter() < NextFrameTime);
    }

    /* Advance to next expected frame time */
    NextFrameTime += FrameDuration;

    /* If we are way behind (more than 5 frames), reset to current time */
    if (NextFrameTime + (FrameDuration * 5) < SDL_GetPerformanceCounter()) {
        NextFrameTime = SDL_GetPerformanceCounter() + FrameDuration;
    }

    return !ExitNow;
}

unsigned int GetJoystick(void) {
    ProcessEvents(0);
    return JoyState;
}

int GetJoystickCount(void) {
    int Count = 0;
    for (int i = 0; i < 2; ++i) {
        if (Controllers[i]) Count++;
    }
    return Count;
}

unsigned int GetKey(void) {
    ProcessEvents(0);
    unsigned int J = LastKey;
    LastKey = 0;
    return J;
}

void ClearKey(void) {
    while (GetKey()) ;
    LastKey = 0;
}

unsigned int WaitKey(void) {
    while (!LastKey && !ExitNow) ProcessEvents(1);
    return GetKey();
}

unsigned int GetMouse(void) {
    int X, Y, WW, WH;
    Uint32 Buttons = SDL_GetMouseState(&X, &Y);
    SDL_GetWindowSize(SDLWindow, &WW, &WH);
    
    if (WW > 0 && WH > 0) {
        X = X * VideoW / WW;
        Y = Y * VideoH / WH;
    }
    
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

pixel *NewImage(Image *Img,int Width,int Height) {
  return GenericNewImage(Img,Width,Height);
}

void FreeImage(Image *Img) {
  GenericFreeImage(Img);
}

void SetVideo(Image *Img,int X,int Y,int W,int H) {
  GenericSetVideo(Img,X,Y,W,H);
}
