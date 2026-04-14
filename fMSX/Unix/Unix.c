/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                         Unix.c                          **/
/**                                                         **/
/** This file contains Unix-dependent subroutines and       **/
/** drivers. It includes screen drivers via CommonMux.h.    **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2021                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "MSX.h"
#include "Console.h"
#include "EMULib.h"
#include "NetPlay.h"
#include "Sound.h"
#include "Record.h"

#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

static struct termios oldt;
static int term_initialized = 0;
static unsigned int LastStdinKey = 0;
static int StdinShiftPressed = 0;

#ifndef SDL2
#include "LibUnix.h"
#else
#include <ctype.h>
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
#define XK_Control_L 0xffe3
#define XK_Control_R 0xffe4
#define XK_Caps_Lock 0xffe5
#define XK_Alt_L     0xffe9
#define XK_Alt_R     0xffea
#endif

#define WIDTH       272                   /* Buffer width    */
#define HEIGHT      228                   /* Buffer height   */

/* Press/Release keys in the background KeyState */
#define XKBD_SET(K) XKeyState[Keys[K][0]]&=~Keys[K][1]
#define XKBD_RES(K) XKeyState[Keys[K][0]]|=Keys[K][1]

/* Combination of EFF_* bits */
int UseEffects  = EFF_SCALE|EFF_SAVECPU|EFF_MITSHM|EFF_VARBPP|EFF_SYNC;

int InMenu;                /* 1: In MenuMSX(), ignore keys   */
int UseZoom     = 2;       /* Zoom factor (1=no zoom)        */
int UseSound    = 22050;   /* Audio sampling frequency (Hz)  */
int SyncFreq    = 60;      /* Sync frequency (0=sync off)    */
int FastForward;           /* Fast-forwarded UPeriod backup  */
int SndSwitch;             /* Mask of enabled sound channels */
int SndVolume;             /* Master volume for audio        */
int OldScrMode;            /* fMSX "ScrMode" variable storage*/

const char *Title     = "fMSX 6.0";       /* Program version */

Image NormScreen;          /* Main screen image              */
Image WideScreen;          /* Wide screen image              */
static pixel *WBuf;        /* From Wide.h                    */
static pixel *XBuf;        /* From Common.h                  */
static unsigned int XPal[80];
static unsigned int BPal[256];
static unsigned int XPal0;

const char *Disks[2][MAXDISKS+1];         /* Disk names      */
volatile byte XKeyState[20]; /* Temporary KeyState array     */

void HandleKeys(unsigned int Key);
void PutImage(void);

/** CommonMux.h **********************************************/
/** Display drivers for all possible screen depths.         **/
/*************************************************************/
#include "CommonMux.h"

/** StdinUpdate() ********************************************/
/** Read characters from stdin and update keyboard matrix.  **/
/*************************************************************/
static void StdinUpdate(void)
{
  unsigned char C[16];
  int n;
  unsigned int MSXKey = 0;
  int NeedShift = 0;
  static int KeyTimer = 0;

  /* Keep key pressed for at least 2 frames to ensure MSX detects it */
  if(KeyTimer > 0)
  {
    KeyTimer--;
    return;
  }

  /* Release previous key */
  if(LastStdinKey)
  {
    HandleKeys(LastStdinKey | CON_RELEASE);
    LastStdinKey = 0;
  }
  if(StdinShiftPressed)
  {
    HandleKeys(KBD_SHIFT | CON_RELEASE);
    StdinShiftPressed = 0;
  }

  /* Read input from stdin */
  n = read(STDIN_FILENO, C, sizeof(C) - 1);
  if(n > 0)
  {
    C[n] = 0; /* Null-terminate for sscanf */
    if(C[0] == 27) /* Escape sequence */
    {
      if(n == 1) MSXKey = XK_Escape;
      else if(n >= 3 && C[1] == '[')
      {
        switch(C[2])
        {
          case 'A': MSXKey = XK_Up; break;
          case 'B': MSXKey = XK_Down; break;
          case 'C': MSXKey = XK_Right; break;
          case 'D': MSXKey = XK_Left; break;
          default:
            if(C[n-1] == '~')
            {
              int Code = 0;
              if(sscanf((char *)&C[2], "%d", &Code) == 1)
              {
                switch(Code)
                {
                  case 11: MSXKey = XK_F1; break;
                  case 12: MSXKey = XK_F2; break;
                  case 13: MSXKey = XK_F3; break;
                  case 14: MSXKey = XK_F4; break;
                  case 15: MSXKey = XK_F5; break;
                  case 17: MSXKey = XK_F6; break;
                  case 18: MSXKey = XK_F7; break;
                  case 19: MSXKey = XK_F8; break;
                  case 20: MSXKey = XK_F9; break;
                  case 21: MSXKey = XK_F10; break;
                  case 23: MSXKey = XK_F11; break;
                  case 24: MSXKey = XK_F12; break;
                  case 2:  MSXKey = XK_Insert; break;
                  case 3:  MSXKey = XK_Delete; break;
                  case 5:  MSXKey = XK_Page_Up; break;
                  case 6:  MSXKey = XK_Page_Down; break;
                }
              }
            }
            break;
        }
      }
      else if(n >= 3 && C[1] == 'O')
      {
        switch(C[2])
        {
          case 'P': MSXKey = XK_F1; break;
          case 'Q': MSXKey = XK_F2; break;
          case 'R': MSXKey = XK_F3; break;
          case 'S': MSXKey = XK_F4; break;
          case 'H': MSXKey = XK_Home; break;
          case 'F': MSXKey = XK_End; break;
        }
      }
      /* Fallback: ESC followed by '=' acts as F12 */
      else if(n == 2 && C[1] == '=') MSXKey = XK_F12;
    }
    else
    {
      /* Map ASCII to MSX keys */
      unsigned char val = C[0];
      if(val == '\n' || val == '\r') MSXKey = XK_Return;
      else if(val == 8 || val == 127) MSXKey = XK_BackSpace;
      else if(val == '\t') MSXKey = XK_Tab;
      else if(val == 3) { /* Ctrl+C handled as nothing */ }
      else if(val >= ' ' && val <= '~')
      {
        MSXKey = toupper(val);
        if(isupper(val) || strchr("!@#$%^&*()_+{}|:\"<>?", val))
          NeedShift = 1;
      }
    }

    if(MSXKey)
    {
      if(NeedShift)
      {
        HandleKeys(XK_Shift_L);
        StdinShiftPressed = 1;
      }
      HandleKeys(MSXKey);
      LastStdinKey = MSXKey;
      KeyTimer = 2; /* Keep pressed for 2 frames */
    }
  }
}

/** InitMachine() ********************************************/
/** Allocate resources needed by machine-dependent code.    **/
/*************************************************************/
int InitMachine(void)
{
  struct termios newt;
  int J;

  /* Initialize variables */
  UseZoom         = UseZoom<1? 1:UseZoom>5? 5:UseZoom;
  InMenu          = 0;
  FastForward     = 0;
  OldScrMode      = 0;
  NormScreen.Data = 0;
  WideScreen.Data = 0;

  /* Initialize system resources */
  InitUnix(Title,UseZoom*WIDTH,UseZoom*HEIGHT);

  /* Set visual effects */
  SetEffects(UseEffects);

  /* Create main image buffer */
  if(!NewImage(&NormScreen,WIDTH,HEIGHT)) { TrashUnix();return(0); }
  XBuf = NormScreen.Data;

#ifndef NARROW
  /* Create wide image buffer */
  if(!NewImage(&WideScreen,WIDTH*2,HEIGHT)) { TrashUnix();return(0); }
  WBuf = WideScreen.Data;
#endif

  /* Set correct screen drivers */
  if(!SetScreenDepth(NormScreen.D)) { TrashUnix();return(0); }

  /* Initialize video to main image */
  SetVideo(&NormScreen,0,0,WIDTH,HEIGHT);

  /* Set all colors to black */
  for(J=0;J<80;J++) SetColor(J,0,0,0);

  /* Create SCREEN8 palette (GGGRRRBB) */
  for(J=0;J<256;J++)
    BPal[J]=X11GetColor(((J>>2)&0x07)*255/7,((J>>5)&0x07)*255/7,(J&0x03)*255/3);

  /* Initialize temporary keyboard array */
  memset((void *)XKeyState,0xFF,sizeof(XKeyState));

  /* Attach keyboard handler */
  SetKeyHandler(HandleKeys);

  /* Initialize sound */
  InitSound(UseSound,150);
  SndSwitch=(1<<MAXCHANNELS)-1;
  SndVolume=64;
  SetChannels(SndVolume,SndSwitch);

  /* Initialize sync timer if needed */
  if((SyncFreq>0)&&!SetSyncTimer(SyncFreq*UPeriod/100)) SyncFreq=0;

  /* Initialize record/replay */
  RPLInit(SaveState,LoadState,MAX_STASIZE);
  RPLRecord(RPL_RESET);

  /* Set terminal to non-canonical mode if stdin is a terminal */
  if(isatty(STDIN_FILENO))
  {
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO | ISIG);
    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    term_initialized = 1;
    /* Make stdin non-blocking just in case */
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
  }

  /* Done */
  return(1);
}

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/*************************************************************/
void TrashMachine(void)
{
  /* Restore terminal settings */
  if(term_initialized)
  {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    term_initialized = 0;
  }

  /* Flush and free recording buffers */
  RPLTrash();

#ifndef NARROW
  FreeImage(&WideScreen);
#endif
  FreeImage(&NormScreen);
  TrashSound();
  TrashUnix();
}

/** PutImage() ***********************************************/
/** Put an image on the screen.                             **/
/*************************************************************/
void PutImage(void)
{
#ifndef NARROW
  /* If screen mode changed... */
  if(ScrMode!=OldScrMode)
  {
    /* Switch to the new screen mode */
    OldScrMode=ScrMode;
    /* Depending on the new screen width... */
    if((ScrMode==6)||((ScrMode==7)&&!ModeYJK)||(ScrMode==MAXSCREEN+1))
      SetVideo(&WideScreen,0,0,WIDTH*2,HEIGHT);
    else
      SetVideo(&NormScreen,0,0,WIDTH,HEIGHT);
  }
#endif

  /* Show replay icon */
  if(RPLPlay(RPL_QUERY)) RPLShow(VideoImg,VideoX+10,VideoY+10);

  /* Show display buffer */
  ShowVideo();
}

/** PlayAllSound() *******************************************/
/** Render and play given number of microseconds of sound.  **/
/*************************************************************/
void PlayAllSound(int uSec)
{
  /* @@@ Twice the argument to avoid skipping */
  RenderAndPlayAudio(2*uSec*UseSound/1000000);
}

/** Joystick() ***********************************************/
/** Query positions of two joystick connected to ports 0/1. **/
/** Returns 0.0.B2.A2.R2.L2.D2.U2.0.0.B1.A1.R1.L1.D1.U1.    **/
/*************************************************************/
unsigned int Joystick(void)
{
  byte RemoteKeyState[20];
  unsigned int J,I;

  /* Get joystick state */
  J = GetJoystick();

  /* Make SHIFT/CONTROL act as fire buttons */
  if(J&BTN_SHIFT)   J|=(J&BTN_ALT? (BTN_FIREB<<16):BTN_FIREB);
  if(J&BTN_CONTROL) J|=(J&BTN_ALT? (BTN_FIREA<<16):BTN_FIREA);

  /* Store joystick state for NetPlay transmission */
  *(unsigned int *)&XKeyState[sizeof(XKeyState)-sizeof(int)]=J;

  /* If failed exchanging KeyStates with remote... */
  if(!NETExchange((void *)RemoteKeyState,(const void *)XKeyState,sizeof(XKeyState)))
    /* Copy temporary keyboard map into permanent one */
    memcpy((void *)KeyState,(const void *)XKeyState,sizeof(KeyState));
  else
  {
    /* Merge local and remote KeyStates */
    for(I=0;I<sizeof(KeyState);++I) KeyState[I]=XKeyState[I]&RemoteKeyState[I];
    /* Merge joysticks, server is player #1, client is player #2 */
    I = *(unsigned int *)&RemoteKeyState[sizeof(XKeyState)-sizeof(int)];
    J = NETConnected()==NET_SERVER?
        ((J&(BTN_ALL|BTN_MODES))|((I&BTN_ALL)<<16))
      : ((I&(BTN_ALL|BTN_MODES))|((J&BTN_ALL)<<16));
  }

  /* Run replay user interface */
  RPLControls(J);

  /* Replay recorded joystick and keyboard states */
  I = RPLPlayKeys(RPL_NEXT,(byte *)KeyState,sizeof(KeyState));
  I = I!=RPL_ENDED? I:0;

  /* Parse joystick */
  if(J&BTN_LEFT)                    I|=JST_LEFT;
  if(J&BTN_RIGHT)                   I|=JST_RIGHT;
  if(J&BTN_UP)                      I|=JST_UP;
  if(J&BTN_DOWN)                    I|=JST_DOWN;
  if(J&(BTN_FIREA|BTN_FIRER))       I|=JST_FIREA;
  if(J&(BTN_FIREB|BTN_FIREL))       I|=JST_FIREB;
  if(J&(BTN_LEFT<<16))              I|=JST_LEFT<<8;
  if(J&(BTN_RIGHT<<16))             I|=JST_RIGHT<<8;
  if(J&(BTN_UP<<16))                I|=JST_UP<<8;
  if(J&(BTN_DOWN<<16))              I|=JST_DOWN<<8;
  if(J&((BTN_FIREA|BTN_FIRER)<<16)) I|=JST_FIREA<<8;
  if(J&((BTN_FIREB|BTN_FIREL)<<16)) I|=JST_FIREB<<8;

  /* Record joystick and keyboard states */
  RPLRecordKeys(I,(byte *)KeyState,sizeof(KeyState));

  /* Done */
  return(I);
}

/** Keyboard() ***********************************************/
/** Modify keyboard matrix.                                 **/
/*************************************************************/
void Keyboard(void)
{
  /* Process input from stdin (for SSH) */
  StdinUpdate();
}

/** Mouse() **************************************************/
/** Query coordinates of a mouse connected to port N.       **/
/** Returns F2.F1.Y.Y.Y.Y.Y.Y.Y.Y.X.X.X.X.X.X.X.X.          **/
/*************************************************************/
unsigned int Mouse(byte N)
{
  unsigned int J;
  int X,Y;

  J = GetMouse();
  X = ScanLines212? 212:192;
  Y = ((J&MSE_YPOS)>>16)-(HEIGHT-X)/2-VAdjust;
  Y = Y<0? 0:Y>=X? X-1:Y;
  X = J&MSE_XPOS;
  X = (ScrMode==6)||((ScrMode==7)&&!ModeYJK)||(ScrMode==MAXSCREEN+1)? (X>>1):X;
  X = X-(WIDTH-256)/2-HAdjust;
  X = X<0? 0:X>=256? 255:X;

  return(((J&MSE_BUTTONS)>>14)|X|(Y<<8));
}

/** SetColor() ***********************************************/
/** Set color N to (R,G,B).                                 **/
/*************************************************************/
void SetColor(byte N,byte R,byte G,byte B)
{
  if(N) XPal[N]=X11GetColor(R,G,B); else XPal0=X11GetColor(R,G,B);
}

/** HandleKeys() *********************************************/
/** Keyboard handler.                                       **/
/*************************************************************/
void HandleKeys(unsigned int Key)
{
  unsigned int J;

  /* When in MenuMSX() or ConDebug(), ignore keypresses */
  if(InMenu||CPU.Trace) return;

  /* Handle special keys */
  if(Key&CON_RELEASE)
  {
    switch(Key&CON_KEYCODE)
    {
      case CON_F9:
      case XK_F9:
        if(FastForward)
        {
          SetEffects(UseEffects);
          UPeriod=FastForward;
          FastForward=0;
        }
        break;

      case CON_BS:
      case XK_BackSpace: XKBD_RES(KBD_BS);break;
      case CON_TAB:
      case XK_Tab:       XKBD_RES(KBD_TAB);break;
      case CON_INSERT:
      case XK_Insert:    XKBD_RES(KBD_INSERT);break;
      case CON_DELETE:
      case XK_Delete:    XKBD_RES(KBD_DELETE);break;
      case CON_UP:
      case XK_Up:        XKBD_RES(KBD_UP);break;
      case CON_DOWN:
      case XK_Down:      XKBD_RES(KBD_DOWN);break;
      case CON_LEFT:
      case XK_Left:      XKBD_RES(KBD_LEFT);break;
      case CON_RIGHT:
      case XK_Right:     XKBD_RES(KBD_RIGHT);break;
      case CON_F1:
      case XK_F1:        XKBD_RES(KBD_F1);break;
      case CON_F2:
      case XK_F2:        XKBD_RES(KBD_F2);break;
      case CON_F3:
      case XK_F3:        XKBD_RES(KBD_F3);break;
      case CON_F4:
      case XK_F4:        XKBD_RES(KBD_F4);break;
      case CON_F5:
      case XK_F5:        XKBD_RES(KBD_F5);break;

      case XK_Shift_L:
      case XK_Shift_R:   XKBD_RES(KBD_SHIFT);break;
      case XK_Control_L:
      case XK_Control_R: XKBD_RES(KBD_CONTROL);break;
      case XK_Alt_L:
      case XK_Alt_R:     XKBD_RES(KBD_GRAPH);break;
      case XK_Caps_Lock: XKBD_RES(KBD_CAPSLOCK);break;
      case CON_EXIT:     /* ESC */
      case XK_Escape:    XKBD_RES(KBD_ESCAPE);break;
      case CON_OK:       /* Enter */
      case XK_Return:    XKBD_RES(KBD_ENTER);break;
      case XK_End:       XKBD_RES(KBD_SELECT);break;
      case XK_Home:      XKBD_RES(KBD_HOME);break;
      case XK_Page_Up:   XKBD_RES(KBD_STOP);break;
      case XK_Page_Down: XKBD_RES(KBD_COUNTRY);break;

      default:
        Key&=CON_KEYCODE;
        if(Key<128) XKBD_RES(Key);
        break;
    }
  }
  else
  {
    /* Cancel replay when a key is pressed */
    J = Key&CON_KEYCODE;
    if((J!=XK_F9)&&(J!=CON_F9)&&(J!=XK_Left)&&(J!=CON_LEFT)&&(J!=XK_Right)&&(J!=CON_RIGHT)&&(J!=XK_Up)&&(J!=CON_UP)) RPLPlay(RPL_OFF);

    switch(Key&CON_KEYCODE)
    {
      case CON_F6:
      case XK_F6:
        LoadSTA(STAName? STAName:"DEFAULT.STA");
        RPLPlay(RPL_OFF);
        break;
      case CON_F7:
      case XK_F7:
        SaveSTA(STAName? STAName:"DEFAULT.STA");
        break;
      case CON_F8:
      case XK_F8:
        if(!(Key&(CON_ALT|CON_CONTROL))) RPLPlay(RPL_RESET);
        else
        {
          /* [ALT]+[F8]  toggles screen softening */
          /* [CTRL]+[F8] toggles scanlines */
          UseEffects^=Key&CON_ALT? EFF_SOFTEN:EFF_TVLINES;
          SetEffects(UseEffects);
        }
        break;
      case CON_F9:
      case XK_F9:
        if(!FastForward) 
        {
          SetEffects(UseEffects&~EFF_SYNC);
          FastForward=UPeriod;
          UPeriod=10;  
        }
        break;
      case CON_F10:
      case XK_F10:
#ifdef DEBUG
        /* [CTRL]+[F10] invokes built-in debugger */
        if(Key&CON_CONTROL)
        { XKBD_RES(KBD_CONTROL);CPU.Trace=1;break; }
#endif
        /* [ALT]+[F10] invokes NetPlay */
        if(Key&CON_ALT)
        {
          XKBD_RES(KBD_GRAPH);
          InMenu=1;
          if(NETPlay(NET_TOGGLE)) ResetMSX(Mode,RAMPages,VRAMPages);
          InMenu=0;
          break;
        }
        /* [F10] invokes built-in menu */
        InMenu=1;
        ClearKey();
        MenuMSX();
        InMenu=0;
        break;
      case CON_F11:
      case XK_F11:
        ResetMSX(Mode,RAMPages,VRAMPages);
        RPLPlay(RPL_OFF);
        break;
      case CON_F12:
      case XK_F12:
        ExitNow=1;
        break;

      case XK_Shift_L:
      case XK_Shift_R:   XKBD_SET(KBD_SHIFT);break;
      case XK_Control_L:
      case XK_Control_R: XKBD_SET(KBD_CONTROL);break;
      case XK_Alt_L:
      case XK_Alt_R:     XKBD_SET(KBD_GRAPH);break;
      case XK_Caps_Lock: XKBD_SET(KBD_CAPSLOCK);break;
      case CON_EXIT:
      case XK_Escape:    XKBD_SET(KBD_ESCAPE);break;
      case CON_OK:
      case XK_Return:    XKBD_SET(KBD_ENTER);break;
      case CON_BS:
      case XK_BackSpace: XKBD_SET(KBD_BS);break;
      case CON_TAB:
      case XK_Tab:       XKBD_SET(KBD_TAB);break;
      case XK_End:       XKBD_SET(KBD_SELECT);break;
      case XK_Home:      XKBD_SET(KBD_HOME);break;
      case XK_Page_Up:   XKBD_SET(KBD_STOP);break;
      case XK_Page_Down: XKBD_SET(KBD_COUNTRY);break;
      case CON_INSERT:
      case XK_Insert:    XKBD_SET(KBD_INSERT);break;
      case CON_DELETE:
      case XK_Delete:    XKBD_SET(KBD_DELETE);break;
      case CON_UP:
      case XK_Up:        XKBD_SET(KBD_UP);break;
      case CON_DOWN:
      case XK_Down:      XKBD_SET(KBD_DOWN);break;
      case CON_LEFT:
      case XK_Left:      XKBD_SET(KBD_LEFT);break;
      case CON_RIGHT:
      case XK_Right:     XKBD_SET(KBD_RIGHT);break;
      case CON_F1:
      case XK_F1:        XKBD_SET(KBD_F1);break;
      case CON_F2:
      case XK_F2:        XKBD_SET(KBD_F2);break;
      case CON_F3:
      case XK_F3:        XKBD_SET(KBD_F3);break;
      case CON_F4:
      case XK_F4:        XKBD_SET(KBD_F4);break;
      case CON_F5:
      case XK_F5:        XKBD_SET(KBD_F5);break;

      default:
        Key&=CON_KEYCODE;
        if(Key<128) XKBD_SET(Key);
        break;
    }
  }
}
