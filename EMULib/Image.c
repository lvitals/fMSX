/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                          Image.c                        **/
/**                                                         **/
/** This file contains non-essential functions that operate **/
/** on images.                                              **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2021                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "EMULib.h"
#include <string.h>
#include <stdlib.h>

pixel GetColor(unsigned char R,unsigned char G,unsigned char B)
{
#if defined(BPP32) || defined(BPP24)
  return (pixel)(((int)R<<16)|((int)G<<8)|B);
#elif defined(BPP16)
  return (pixel)(((31*(R)/255)<<11)|((63*(G)/255)<<5)|(31*(B)/255));
#elif defined(BPP8)
  return (pixel)(((7*(R)/255)<<5)|((7*(G)/255)<<2)|(3*(B)/255));
#else
  return (pixel)(((int)R<<16)|((int)G<<8)|B);
#endif
}

void ClearImage(Image *Img,pixel Color)
{
  pixel *P;
  int X,Y;
  for(P=(pixel *)Img->Data,Y=Img->H;Y;--Y,P+=Img->L)
    for(X=0;X<Img->W;++X) P[X]=Color;
}

void IMGCopy(Image *Dst,int DX,int DY,const Image *Src,int SX,int SY,int W,int H,int TColor)
{
  const pixel *S;
  pixel *D;
  int X;
  if(DX<0) { W+=DX;SX-=DX;DX=0; }
  if(DY<0) { H+=DY;SY-=DY;DY=0; }
  if(SX<0) { W+=SX;DX-=SX;SX=0; } else if(SX+W>Src->W) W=Src->W-SX;
  if(SY<0) { H+=SY;DY-=SY;SY=0; } else if(SY+H>Src->H) H=Src->H-SY;
  if(DX+W>Dst->W) W=Dst->W-DX;
  if(DY+H>Dst->H) H=Dst->H-DY;
  if((W<=0)||(H<=0)) return;
  S = (pixel *)Src->Data+Src->L*SY+SX;
  D = (pixel *)Dst->Data+Dst->L*DY+DX;
  if(TColor<0)
    for(;H;--H,S+=Src->L,D+=Dst->L) memcpy(D,S,W*sizeof(pixel));
  else
    for(;H;--H,S+=Src->L,D+=Dst->L)
      for(X=0;X<W;++X) if(S[X]!=TColor) D[X]=S[X];
}

void IMGDrawRect(Image *Img,int X,int Y,int W,int H,pixel Color)
{
  pixel *P;
  int J;
  if(X<0) { W+=X;X=0; } else if(X+W>Img->W) W=Img->W-X;
  if(Y<0) { H+=Y;Y=0; } else if(Y+H>Img->H) H=Img->H-Y;
  if((W>0)&&(H>0)) {
    for(P=(pixel *)Img->Data+Img->L*Y+X,J=0;J<W;++J) P[J]=Color;
    for(H-=2,P+=Img->L;H;--H,P+=Img->L) P[0]=P[W-1]=Color;
    for(J=0;J<W;++J) P[J]=Color;
  }
}

void IMGFillRect(Image *Img,int X,int Y,int W,int H,pixel Color)
{
  pixel *P;
  if(X<0) { W+=X;X=0; } else if(X+W>Img->W) W=Img->W-X;
  if(Y<0) { H+=Y;Y=0; } else if(Y+H>Img->H) H=Img->H-Y;
  if((W>0)&&(H>0))
    for(P=(pixel *)Img->Data+Img->L*Y+X;H;--H,P+=Img->L)
      for(X=0;X<W;++X) P[X]=Color;
}

Image *CropImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  return GenericCropImage(Dst,Src,X,Y,W,H);
}

void ScaleImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  register pixel *DP,*SP,*S;
  register unsigned int DX,DY;
  if((Dst->W==W)&&(Dst->H==H)) { IMGCopy(Dst,0,0,Src,X,Y,W,H,-1);return; }
  if(W<0) { X+=W;W=-W; }
  if(H<0) { Y+=H;H=-H; }
  X=X<0? 0:X>Src->W? Src->W:X;
  Y=Y<0? 0:Y>Src->H? Src->H:Y;
  W=X+W>Src->W? Src->W-X:W;
  H=Y+H>Src->H? Src->H-Y:H;
  if(!W||!H) return;
  SP=(pixel *)Src->Data+Y*Src->L+X;
  DP=(pixel *)Dst->Data;
  W<<=16; H<<=16;
  DX=(W+Dst->W-1)/Dst->W;
  DY=(H+Dst->H-1)/Dst->H;
  for(Y=0;Y<H;Y+=DY) {
    S=SP+(Y>>16)*Src->L;
    for(X=0;X<W;X+=DX) *DP++=S[X>>16];
    DP+=Dst->L-Dst->W;
  }
}

void TelevizeImage(Image *Img,int X,int Y,int W,int H) {}

/* Stubs for missing effects used by Touch.c */
void SoftenImage(Image *Dst,const Image *Src,int X,int Y,int W,int H) { ScaleImage(Dst,Src,X,Y,W,H); }
void SoftenEPX(Image *Dst,const Image *Src,int X,int Y,int W,int H) { ScaleImage(Dst,Src,X,Y,W,H); }
void SoftenEAGLE(Image *Dst,const Image *Src,int X,int Y,int W,int H) { ScaleImage(Dst,Src,X,Y,W,H); }
void SoftenSCALE2X(Image *Dst,const Image *Src,int X,int Y,int W,int H) { ScaleImage(Dst,Src,X,Y,W,H); }
void InterpolateImage(Image *Dst,const Image *Src,int X,int Y,int W,int H) { ScaleImage(Dst,Src,X,Y,W,H); }
void LcdizeImage(Image *Img,int X,int Y,int W,int H) {}
void RasterizeImage(Image *Img,int X,int Y,int W,int H) {}
void CMYizeImage(Image *Img,int X,int Y,int W,int H) {}
void RGBizeImage(Image *Img,int X,int Y,int W,int H) {}
void MonoImage(Image *Img,int X,int Y,int W,int H) {}
void SepiaImage(Image *Img,int X,int Y,int W,int H) {}
void GreenImage(Image *Img,int X,int Y,int W,int H) {}
void AmberImage(Image *Img,int X,int Y,int W,int H) {}
