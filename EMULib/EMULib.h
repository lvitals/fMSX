/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        EMULib.h                         **/
/**                                                         **/
/** This file contains platform-independent definitions and **/
/** declarations for the emulation library.                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2021                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef EMULIB_H
#define EMULIB_H

#ifdef UNIX
#ifndef SDL2
#include "LibUnix.h"
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** pixel ****************************************************/
/** Pixels may be either 8bit, or 16bit, or 32bit. When no  **/
/** BPP* specified, we assume the pixel to have the largest **/
/** size and default to GetColor().                         **/
/*************************************************************/
#ifndef PIXEL_TYPE_DEFINED
#define PIXEL_TYPE_DEFINED
#if defined(BPP32) || defined(BPP24)
typedef unsigned int pixel;
#elif defined(BPP16)
typedef unsigned short pixel;
#elif defined(BPP8)
typedef unsigned char pixel;
#else
typedef unsigned int pixel;
#define PIXEL(R,G,B) GetColor(R,G,B)
#endif
#endif

/** sample ***************************************************/
/** Audio samples may be either 8bit or 16bit.              **/
/*************************************************************/
#ifndef SAMPLE_TYPE_DEFINED
#define SAMPLE_TYPE_DEFINED
#ifdef BPS16
typedef signed short sample;
#else
typedef signed char sample;
#endif
#endif

/** Image ****************************************************/
/** This data type encapsulates a bitmap.                   **/
/*************************************************************/
typedef struct
{
  pixel *Data;               /* Buffer containing WxH pixels */
  int W,H,L,D;               /* Image size, pitch, depth     */
  char Cropped;              /* 1: Cropped, do not free()    */
#ifdef WINDOWS
  void *hDC;                 /* Handle to device context     */
  void *hBMap;               /* Handle to bitmap             */
#endif
#ifdef MAEMO
  void *GImg;                /* Pointer to GdkImage object   */
#endif
#ifdef MEEGO
  void *QImg;                /* Pointer to QImage object     */
#endif
#if defined(UNIX) && !defined(SDL2)
  void *XImg;                /* Pointer to XImage structure  */
  int Attrs;                 /* USE_SHM and other attributes */
#ifdef MITSHM
  unsigned char SHMInfo[128]; /* Shared memory info placeholder */
#endif
#endif
} Image;

/** Current Video Image **************************************/
/** These parameters are set with SetVideo() and used by    **/
/** ShowVideo() to show a WxH fragment from <X,Y> of Img.   **/
/*************************************************************/
extern Image *VideoImg;        /* Current ShowVideo() image  */
extern int VideoX;             /* X for ShowVideo()          */
extern int VideoY;             /* Y for ShowVideo()          */
extern int VideoW;             /* Width for ShowVideo()      */
extern int VideoH;             /* Height for ShowVideo()     */

/** KeyHandler ***********************************************/
/** This function receives key presses and releases.        **/
/*************************************************************/
extern void (*KeyHandler)(unsigned int Key);

/** Special Key Codes ****************************************/
/** Modifiers returned by GetKey() and WaitKey().           **/
/*************************************************************/
#define CON_KEYCODE  0x03FFFFFF /* Key code                  */
#define CON_MODES    0xFC000000 /* Mode bits, as follows:    */
#define CON_CLICK    0x04000000 /* Key click (LiteS60 only)  */
#define CON_CAPS     0x08000000 /* CapsLock held             */
#define CON_SHIFT    0x10000000 /* SHIFT held                */
#define CON_CONTROL  0x20000000 /* CONTROL held              */
#define CON_ALT      0x40000000 /* ALT held                  */
#define CON_RELEASE  0x80000000 /* Key released (going up)   */

#define CON_F1       0xEE
#define CON_F2       0xEF
#define CON_F3       0xF0
#define CON_F4       0xF1
#define CON_F5       0xF2
#define CON_F6       0xF3
#define CON_F7       0xF4
#define CON_F8       0xF5
#define CON_F9       0xF6
#define CON_F10      0xF7
#define CON_F11      0xF8
#define CON_F12      0xF9
#define CON_LEFT     0xFA
#define CON_RIGHT    0xFB
#define CON_UP       0xFC
#define CON_DOWN     0xFD
#define CON_OK       0xFE
#define CON_EXIT     0xFF

/** SetVideo() ***********************************************/
/** Set part of the image as "active" for display.          **/
/*************************************************************/
void GenericSetVideo(Image *Img,int X,int Y,int W,int H);
void SetVideo(Image *Img,int X,int Y,int W,int H);

/** ShowVideo() **********************************************/
/** Show "active" image at the actual screen or window.     **/
/*************************************************************/
int ShowVideo(void);

/** GetColor() ***********************************************/
/** Return pixel corresponding to the given <R,G,B> value.  **/
/*************************************************************/
pixel GetColor(unsigned char R,unsigned char G,unsigned char B);

/** X11GetColor **********************************************/
/** Get pixel for the current screen depth based on the RGB **/
/** values.                                                 **/
/*************************************************************/
unsigned int X11GetColor(unsigned char R,unsigned char G,unsigned char B);

/** ProcessEvents() ******************************************/
/** Process UI event messages. Returns 1 for continued      **/
/** execution, 0 if application has been closed.            **/
/*************************************************************/
int ProcessEvents(int Wait);

/** InitUnix() ***********************************************/
/** Initialize resources and set initial window title and   **/
/** dimensions.                                             **/
/*************************************************************/
int InitUnix(const char *Title,int Width,int Height);

/** TrashUnix() **********************************************/
/** Free resources allocated in InitUnix()                  **/
/*************************************************************/
void TrashUnix(void);

/** SetKeyHandler() ******************************************/
/** Attach keyboard handler that will be called when a key  **/
/** is pressed or released.                                 **/
/*************************************************************/
void SetKeyHandler(void (*Handler)(unsigned int Key));

/** GetJoystick() ********************************************/
/** Get the state of joypad buttons (1="pressed").          **/
/*************************************************************/
unsigned int GetJoystick(void);

/** WaitJoystick() *******************************************/
/** Wait for joystick buttons.                              **/
/*************************************************************/
unsigned int WaitJoystick(unsigned int Mask);

/** GetMouse() ***********************************************/
/** Get mouse position and button states.                   **/
/*************************************************************/
unsigned int GetMouse(void);

/** GetKey() *************************************************/
/** Get currently pressed key.                              **/
/*************************************************************/
unsigned int GetKey(void);

/** ClearKey() ***********************************************/
/** Clear the last key pressed.                             **/
/*************************************************************/
void ClearKey(void);

/** WaitKey() ************************************************/
/** Wait for key press.                                     **/
/*************************************************************/
unsigned int WaitKey(void);

/** WaitKeyOrMouse() *****************************************/
/** Wait for key or mouse press.                            **/
/*************************************************************/
unsigned int WaitKeyOrMouse(void);

/** NewImage() ***********************************************/
/** Create a new image of the given size.                   **/
/*************************************************************/
pixel *GenericNewImage(Image *Img,int Width,int Height);
pixel *NewImage(Image *Img,int Width,int Height);

/** FreeImage() **********************************************/
/** Free previously allocated image.                        **/
/*************************************************************/
void GenericFreeImage(Image *Img);
void FreeImage(Image *Img);

/** CropImage() **********************************************/
/** Create a subimage.                                      **/
/*************************************************************/
Image *GenericCropImage(Image *Dst,const Image *Src,int X,int Y,int W,int H);
Image *CropImage(Image *Dst,const Image *Src,int X,int Y,int W,int H);

/** ScaleImage() *********************************************/
/** Scale image.                                            **/
/*************************************************************/
void ScaleImage(Image *Dst,const Image *Src,int X,int Y,int W,int H);

/** InterpolateImage() ***************************************/
/** Interpolate image.                                      **/
/*************************************************************/
void InterpolateImage(Image *Dst,const Image *Src,int X,int Y,int W,int H);

/** Soften functions *****************************************/
void SoftenImage(Image *Dst,const Image *Src,int X,int Y,int W,int H);
void SoftenEPX(Image *Dst,const Image *Src,int X,int Y,int W,int H);
void SoftenEAGLE(Image *Dst,const Image *Src,int X,int Y,int W,int H);
void SoftenSCALE2X(Image *Dst,const Image *Src,int X,int Y,int W,int H);

/** Image effects ********************************************/
void TelevizeImage(Image *Img,int X,int Y,int W,int H);
void LcdizeImage(Image *Img,int X,int Y,int W,int H);
void RasterizeImage(Image *Img,int X,int Y,int W,int H);
void CMYizeImage(Image *Img,int X,int Y,int W,int H);
void RGBizeImage(Image *Img,int X,int Y,int W,int H);
void MonoImage(Image *Img,int X,int Y,int W,int H);
void SepiaImage(Image *Img,int X,int Y,int W,int H);
void GreenImage(Image *Img,int X,int Y,int W,int H);
void AmberImage(Image *Img,int X,int Y,int W,int H);

/** IMGCopy() ************************************************/
void IMGCopy(Image *Dst,int DX,int DY,const Image *Src,int SX,int SY,int W,int H,int TColor);

/** IMGDrawRect()/IMGFillRect() ******************************/
void IMGDrawRect(Image *Img,int X,int Y,int W,int H,pixel Color);
void IMGFillRect(Image *Img,int X,int Y,int W,int H,pixel Color);

/** SetEffects() *********************************************/
/** Set visual effects applied to video in ShowVideo().     **/
/*************************************************************/
void SetEffects(unsigned int NewEffects);

/** ParseEffects() *******************************************/
/** Parse command line visual effect options.               **/
/*************************************************************/
unsigned int ParseEffects(char *Args[],unsigned int Effects);

/** SetSyncTimer() *******************************************/
/** Set synchronization timer to a given frequency in Hz.   **/
/*************************************************************/
int SetSyncTimer(int Hz);

/** WaitSyncTimer() ******************************************/
/** Wait for the timer to become ready.                     **/
/*************************************************************/
int WaitSyncTimer(void);

/** Audio Functions ******************************************/
unsigned int InitAudio(unsigned int Rate,unsigned int Latency);
void TrashAudio(void);
unsigned int WriteAudio(sample *Data,unsigned int Length);
unsigned int GetFreeAudio(void);
unsigned int GetTotalAudio(void);

#define EFF_NONE       0x0000
#define EFF_SCALE      0x0001
#define EFF_SOFTEN     0x0002
#define EFF_TVLINES    0x0004
#define EFF_SAVECPU    0x0008
#define EFF_SYNC       0x0010
#define EFF_STRETCH    0x0100
#define EFF_SHOWFPS    0x0200
#define EFF_LCDLINES   0x0400
#define EFF_VKBD       0x0800
#define EFF_DIRECT    0x20000
#define EFF_CMYMASK   0x40000
#define EFF_RGBMASK   0x80000
#define EFF_MONO    0x2000000
#define EFF_4X3     0x8000000
#define EFF_PENCUES    0x0020
#define EFF_DIALCUES   0x0040

#define EFF_MITSHM     0x100000
#define EFF_VARBPP     0x200000

#define EFF_SOFTEN_ALL (EFF_SOFTEN|0x1000|0x1000000)
#define EFF_2XSAI      (EFF_SOFTEN)
#define EFF_EPX        0x1000
#define EFF_EAGLE      (EFF_SOFTEN|0x1000)
#define EFF_SCALE2X    0x1000000
#define EFF_HQ4X       (EFF_SOFTEN|0x1000000)
#define EFF_NEAREST    (0x1000|0x1000000)
#define EFF_LINEAR     (EFF_SOFTEN|0x1000|0x1000000)

#define EFF_RASTER_ALL (EFF_TVLINES|EFF_LCDLINES|0x800000)
#define EFF_RASTER     0x800000

#define EFF_MASK_ALL   (EFF_CMYMASK|EFF_RGBMASK|EFF_MONO)
#define EFF_GREEN      (EFF_MONO|EFF_CMYMASK)
#define EFF_AMBER      (EFF_MONO|EFF_RGBMASK)
#define EFF_SEPIA      (EFF_MONO|EFF_CMYMASK|EFF_RGBMASK)

#define BTN_LEFT     0x0001
#define BTN_RIGHT    0x0002
#define BTN_UP       0x0004
#define BTN_DOWN     0x0008
#define BTN_FIREA    0x0010
#define BTN_FIREB    0x0020
#define BTN_FIREL    0x0040
#define BTN_FIRER    0x0080
#define BTN_START    0x0100
#define BTN_SELECT   0x0200
#define BTN_EXIT     0x0400
#define BTN_FIREX    0x0800
#define BTN_FIREY    0x1000
#define BTN_FFWD     0x2000
#define BTN_MENU     0x4000
#define BTN_ALL      0x7FFF

#define BTN_SHIFT    CON_SHIFT
#define BTN_CONTROL  CON_CONTROL
#define BTN_ALT      CON_ALT
#define BTN_MODES    (BTN_SHIFT|BTN_CONTROL|BTN_ALT)
#define BTN_ARROWS   (BTN_LEFT|BTN_RIGHT|BTN_UP|BTN_DOWN)

#define MSE_RIGHT    0x80000000
#define MSE_LEFT     0x40000000
#define MSE_BUTTONS  (MSE_RIGHT|MSE_LEFT)
#define MSE_YPOS     0x3FFF0000
#define MSE_XPOS     0x0000FFFF

#define SND_CHANNELS 16

#ifdef __cplusplus
}
#endif
#endif /* EMULIB_H */
