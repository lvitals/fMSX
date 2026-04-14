/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                         Menu.c                          **/
/**                                                         **/
/** This file contains runtime menu code for configuring    **/
/** the emulator. It uses console functions from Console.h. **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2005-2021                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "MSX.h"
#include "Console.h"
#include "Sound.h"
#include "Hunt.h"
#include "MCF.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define CLR_BACK   PIXEL(255,255,255)
#define CLR_BACK2  PIXEL(255,200,150)
#define CLR_BACK3  PIXEL(150,255,255)
#define CLR_BACK4  PIXEL(255,255,150)
#define CLR_BACK5  PIXEL(255,150,255)
#define CLR_TEXT   PIXEL(0,0,0)
#define CLR_WHITE  PIXEL(255,255,255)
#define CLR_ERROR  PIXEL(200,0,0)
#define CLR_INFO   PIXEL(0,128,0)

static char SndNameBuf[256];

#define REDRAW_BACKGROUND \
  if(RefreshLine[ScrMode]) \
    for(L=0;L<VideoH;++L) RefreshLine[ScrMode](L < (ScanLines212? 212:192)? L : (ScanLines212? 211:191))

extern byte *MemMap[4][4][8];         /* [PPage][SPage][Adr] */
extern byte *EmptyRAM;                /* Dummy memory area   */
extern int UseEffects;                /* Video effects       */
extern int SyncFreq;                  /* Sync frequency      */
extern byte VDP[64];                  /* VDP registers       */
extern void ResetSyncTimer(void);     /* Reset sync timer    */

/** Cheat Structures *****************************************/
extern int CheatCount;       /* # of cheats in CheatCodes[]  */
extern int MCFCount;         /* # of entries in MCFEntries[] */
extern CheatCode CheatCodes[MAXCHEATS];
extern MCFEntry MCFEntries[MAXCHEATS];

static void RedrawMain(const char *Items,int Selected)
{
  const char *P;
  int I,J,W,H,X,Y,L,Total;

  /* Redraw background */
  if(RefreshLine[ScrMode])
    for(L=0;L<VideoH;++L) RefreshLine[ScrMode](L < (ScanLines212? 212:192)? L : (ScanLines212? 211:191));

  /* Compute menu items count and width */
  for(P=Items,I=0,Total=-1;*P;Total++,P++)
  {
    for(J=0;*P;P++,J++);
    if(J>I) I=J;
  }

  /* Update menu coordinates and dimensions */
  J = VideoW>>3;
  W = I+3;
  W = W>J-2? J-2:W;
  X = (J-W)/2;

  J = VideoH>>3;
  H = Total+3;
  H = H>J-2? J-2:H;
  Y = (J-H)/2;

  CONWindow(X,Y,W,H,CLR_TEXT,CLR_BACK,Items);

  /* Skip to the first item */
  for(P=Items;*P;P++);
  for(P++,I=0;*P&&(I<H-3);I++,P++)
  {
    CONPrintN(X+2,Y+2+I,P,W-3);
    for(;*P;P++);
  }

  /* Draw arrow */
  if((Selected>0)&&(Selected<=Total))
    CONChar(X+1,Y+2+Selected-1,CON_ARROW);
}

/** MenuMSX() ************************************************/
/** Invoke a menu system allowing to configure the emulator **/
/** and perform several common tasks.                       **/
/*************************************************************/
void MenuMSX(void)
{
  const char *P;
  char S[512],MainS[512],*T,*PP;
  int I,J,K,N,V,M,L;

  /* Display and activate top menu */
  for(J=1;J;)
  {
    /* Redraw background */
    if(RefreshLine[ScrMode])
      for(L=0;L<VideoH;++L) RefreshLine[ScrMode](L < (ScanLines212? 212:192)? L : (ScanLines212? 211:191));

    /* Compose menu */
    sprintf(S,
      "fMSX\n"
      "Load file\n"
      "Save file\n"
      "  \n"
      "Hardware model\n"
      "Input devices\n"
      "Cartridge slots\n"
      "Disk drives\n"
      "Cheats\n"
      "Search cheats\n"
      "  \n"
      "Log soundtrack    %c\n"
      "Hit MIDI drums    %c\n"
      "  \n"
      "Show real-time FPS %c\n"
      "Show all sprites  %c\n"
      "Patch DiskROM     %c\n"
      "  \n"
      "POKE &hFFFF,&hAA\n"
      "Rewind tape\n"
      "Reset emulator\n"
      "Quit emulator\n"
      "  \n"
      "Done\n",
      MIDILogging(MIDI_QUERY)?     CON_CHECK:' ',
      OPTION(MSX_DRUMS)?           CON_CHECK:' ',
      (UseEffects&EFF_SHOWFPS)?    CON_CHECK:' ',
      OPTION(MSX_ALLSPRITE)?       CON_CHECK:' ',
      OPTION(MSX_PATCHBDOS)?       CON_CHECK:' '
    );

    /* Store main menu for redrawing */
    memcpy(MainS,S,sizeof(S));

    /* Replace all EOLNs with zeroes */
    for(L=0;MainS[L];L++) if(MainS[L]=='\n') MainS[L]='\0';
    memcpy(S,MainS,sizeof(S));

    /* Run menu */
    K=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK,S,J);
    /* Exit top menu on ESC */
    if(K<0) break;
    /* Update selection */
    J=K;
    /* Handle menu selection */
    switch(J)
    {
      case 1: /* Load cartridge, disk image, state, or font */
        /* Request file name */
        RedrawMain(MainS,J);
        P=CONFile(CLR_TEXT,CLR_BACK3,".rom\0.rom.gz\0.mx1\0.mx1.gz\0.mx2\0.mx2.gz\0.dsk\0.dsk.gz\0.sta\0.sta.gz\0.cas\0.fnt\0.fnt.gz\0.cht\0.pal\0",0);
        /* Try loading file, show error on failure */
        if(P)
        {
          if(!LoadSTA(P)&&!LoadFile(P))
          {
            RedrawMain(MainS,J);
            CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot load file.\0\0");
          }
          else J=0; /* Close menu on success */
        }
        break;

      case 2: /* Save state, printer output, or soundtrack */
        /* Run menu */
        RedrawMain(MainS,J);
        V=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK4,
          "Save File\0Emulation state\0Printer output\0MIDI soundtrack\0",1
        );
        if(V<0) break;
        switch(V)
        {
          case 1: /* Save state */
            /* Request file name */
            RedrawMain(MainS,J);
            P=CONFile(CLR_TEXT,CLR_BACK2,".sta\0",1);
            /* Try saving state, show error on failure */
            if(P&&!SaveSTA(P))
            {
              RedrawMain(MainS,J);
              CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot save state.\0\0");
            }
            break;
          case 2: /* Printer output file */
            /* Request file name */
            RedrawMain(MainS,J);
            P=CONFile(CLR_TEXT,CLR_BACK2,".prn\0.out\0.txt\0",1);
            /* Try changing printer output */
            if(P) ChangePrinter(P);
            break;
          case 3: /* Soundtrack output file */
            /* Request file name */
            RedrawMain(MainS,J);
            P=CONFile(CLR_TEXT,CLR_BACK2,".mid\0.rmi\0",1);
            if(P)
            {
              /* Try changing MIDI log output, show error on failure */
              if(strlen(P)+1>sizeof(SndNameBuf))
              {
                RedrawMain(MainS,J);
                CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Name too long.\0\0");
              }
              else
              {
                strcpy(SndNameBuf,P);
                SndName=SndNameBuf;
                InitMIDI(SndName);
                MIDILogging(MIDI_ON);
              }
            }
            break;
        }
        break;

      case 4: /* Hardware model */
        for(K=1;K;)
        {
          /* Redraw background */
          RedrawMain(MainS,J);

          /* Compose menu */
          sprintf(S,
            "Hardware Model\n"
            "MSX1 (TMS9918)    %c\n"
            "MSX2 (V9938)      %c\n"
            "MSX2+ (V9958)     %c\n"
            "  \n"
            "NTSC (US/Japan)   %c\n"
            "PAL (Europe)      %c\n"
            "  \n"
            "Main memory   %3dkB\n"
            "Video memory  %3dkB\n"
            "  \n"
            "Done\n",
            MODEL(MSX_MSX1)?       CON_CHECK:' ',
            MODEL(MSX_MSX2)?       CON_CHECK:' ',
            MODEL(MSX_MSX2P)?      CON_CHECK:' ',
            VIDEO(MSX_NTSC)?       CON_CHECK:' ',
            VIDEO(MSX_PAL)?        CON_CHECK:' ',
            RAMPages*16,
            VRAMPages*16
          );
          /* Replace all EOLNs with zeroes */
          for(L=0;S[L];L++) if(S[L]=='\n') S[L]='\0';
          /* Run menu */
          V=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK4,S,K);
          /* Exit submenu on ESC */
          if(V<0) break;
          /* Update selection */
          K=V;
          /* Handle menu selection */
          switch(K)
          {
            case 1:  if(!MODEL(MSX_MSX1))  ResetMSX((Mode&~MSX_MODEL)|MSX_MSX1,RAMPages,VRAMPages);break;
            case 2:  if(!MODEL(MSX_MSX2))  ResetMSX((Mode&~MSX_MODEL)|MSX_MSX2,RAMPages,VRAMPages);break;
            case 3:  if(!MODEL(MSX_MSX2P)) ResetMSX((Mode&~MSX_MODEL)|MSX_MSX2P,RAMPages,VRAMPages);break;
            case 5:  Mode=(Mode&~MSX_VIDEO)|MSX_NTSC; VDP[9]&=~0x02; UpdateTimings(); break;
            case 6:  Mode=(Mode&~MSX_VIDEO)|MSX_PAL;  VDP[9]|=0x02;  UpdateTimings(); break;
            case 8:  ResetMSX(Mode,RAMPages<32? RAMPages*2:4,VRAMPages);break;
            case 9:  ResetMSX(Mode,RAMPages,VRAMPages<32? VRAMPages*2:2);break;
            case 11: K=0;break;
          }
        }
        break;

      case 5: /* Input devices */
        for(K=1;K;)
        {
          /* Redraw background */
          RedrawMain(MainS,J);

          /* Compose menu */
          sprintf(S,
            "Input Devices\n"
            "SOCKET1:Empty       %c\n"
            "SOCKET1:Joystick    %c\n"
            "SOCKET1:JoyMouse    %c\n"
            "SOCKET1:Mouse       %c\n"
            "  \n"
            "SOCKET2:Empty       %c\n"
            "SOCKET2:Joystick    %c\n"
            "SOCKET2:JoyMouse    %c\n"
            "SOCKET2:Mouse       %c\n"
            "  \n"
            "Autofire on SPACE   %c\n"
            "Autofire on FIRE-A  %c\n"
            "Autofire on FIRE-B  %c\n"
            "  \n"
            "Done\n",
            JOYTYPE(0)==JOY_NONE?     CON_CHECK:' ',
            JOYTYPE(0)==JOY_STICK?    CON_CHECK:' ',
            JOYTYPE(0)==JOY_MOUSTICK? CON_CHECK:' ',
            JOYTYPE(0)==JOY_MOUSE?    CON_CHECK:' ',
            JOYTYPE(1)==JOY_NONE?     CON_CHECK:' ',
            JOYTYPE(1)==JOY_STICK?    CON_CHECK:' ',
            JOYTYPE(1)==JOY_MOUSTICK? CON_CHECK:' ',
            JOYTYPE(1)==JOY_MOUSE?    CON_CHECK:' ',
            OPTION(MSX_AUTOSPACE)?    CON_CHECK:' ',
            OPTION(MSX_AUTOFIREA)?    CON_CHECK:' ',
            OPTION(MSX_AUTOFIREB)?    CON_CHECK:' '
          );
          /* Replace all EOLNs with zeroes */
          for(L=0;S[L];L++) if(S[L]=='\n') S[L]='\0';
          /* Run menu */
          V=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK4,S,K);
          /* Exit submenu on ESC */
          if(V<0) break;
          /* Update selection */
          K=V;
          /* Handle menu selection */
          switch(K)
          {
            case 1:  SETJOYTYPE(0,JOY_NONE);break;
            case 2:  SETJOYTYPE(0,JOY_STICK);break;
            case 3:  SETJOYTYPE(0,JOY_MOUSTICK);break;
            case 4:  SETJOYTYPE(0,JOY_MOUSE);break;
            case 6:  SETJOYTYPE(1,JOY_NONE);break;
            case 7:  SETJOYTYPE(1,JOY_STICK);break;
            case 8:  SETJOYTYPE(1,JOY_MOUSTICK);break;
            case 9:  SETJOYTYPE(1,JOY_MOUSE);break;
            case 11: Mode^=MSX_AUTOSPACE;break;
            case 12: Mode^=MSX_AUTOFIREA;break;
            case 13: Mode^=MSX_AUTOFIREB;break;
            case 15: K=0;break;
          }
        }
        break;

      case 6: /* Cartridge slots */
        /* Create slot selection menu */
        sprintf(S,
          "Cartridges\n"
          "Slot A:%c\n"
          "Slot B:%c\n",
          MemMap[1][0][2]!=EmptyRAM? CON_FILE:' ',
          MemMap[2][0][2]!=EmptyRAM? CON_FILE:' '
        );
        /* Replace all EOLNs with zeroes */
        for(L=0;S[L];L++) if(S[L]=='\n') S[L]='\0';
        /* Redraw background */
        RedrawMain(MainS,J);
        /* Get cartridge slot number */
        V=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK4,S,1);
        /* Exit to top menu if cancelled or ESC */
        if(V<=0) break; else N=V-1;
        /* Run slot-specific menu */
        for(K=1;K;)
        {
          /* Redraw background */
          RedrawMain(MainS,J);

          /* Compose menu */
          sprintf(S,
            "Cartridge Slot %c\n"
            "Load cartridge\n"
            "Eject cartridge\n"
            "Guess MegaROM mapper  %c\n"
            "  \n"
            "Generic 8kB switch    %c\n"
            "Generic 16kB switch   %c\n"
            "Konami 5000h mapper   %c\n"
            "Konami 4000h mapper   %c\n"
            "ASCII 8kB mapper      %c\n"
            "ASCII 16kB mapper     %c\n"
            "Konami GameMaster2    %c\n"
            "Panasonic FMPAC       %c\n"
            "  \n"
            "Done\n",
            'A'+N,
            ROMGUESS(N)?              CON_CHECK:' ',
            ROMTYPE(N)==MAP_GEN8?     CON_CHECK:' ',
            ROMTYPE(N)==MAP_GEN16?    CON_CHECK:' ',
            ROMTYPE(N)==MAP_KONAMI5?  CON_CHECK:' ',
            ROMTYPE(N)==MAP_KONAMI4?  CON_CHECK:' ',
            ROMTYPE(N)==MAP_ASCII8?   CON_CHECK:' ',
            ROMTYPE(N)==MAP_ASCII16?  CON_CHECK:' ',
            ROMTYPE(N)==MAP_GMASTER2? CON_CHECK:' ',
            ROMTYPE(N)==MAP_FMPAC?    CON_CHECK:' '
          );
          /* Replace all EOLNs with zeroes */
          for(L=0;S[L];L++) if(S[L]=='\n') S[L]='\0';
          /* Run menu */
          V=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK4,S,K);
          /* Exit submenu on ESC */
          if(V<0) break;
          /* Update selection */
          K=V;
          /* Handle menu selection */
          switch(K)
          {
            case 1:
              /* Request file name */
              RedrawMain(MainS,J);
              P=CONFile(CLR_TEXT,CLR_BACK3,".rom\0.rom.gz\0.mx1\0.mx1.gz\0.mx2\0.mx2.gz\0",0);
              /* Try loading file, show error on failure */
              if(P&&!LoadCart(P,N,ROMGUESS(N)|ROMTYPE(N)))
              {
                RedrawMain(MainS,J);
                CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot load file.\0\0");
              }
              /* Exit to top menu */
              K=0;
              break;
            case 2:  LoadCart(0,N,ROMGUESS(N)|ROMTYPE(N));K=0;break;
            case 3:  Mode^=MSX_GUESSA<<N;break;
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12: SETROMTYPE(N,K-5);ResetMSX(Mode,RAMPages,VRAMPages);break;
            case 14: K=0;break;
          }
        }
        break;

      case 7: /* Disk drives */
        /* Create drive selection menu */
        sprintf(S,
          "Disk Drives\n"
          "Drive A:%c\n"
          "Drive B:%c\n",
          FDD[0].Data? CON_FILE:' ',
          FDD[1].Data? CON_FILE:' '
        );
        /* Replace all EOLNs with zeroes */
        for(L=0;S[L];L++) if(S[L]=='\n') S[L]='\0';
        /* Redraw background */
        RedrawMain(MainS,J);
        /* Get disk drive number */
        V=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK4,S,1);
        /* Exit to top menu if cancelled or ESC */
        if(V<=0) break; else N=V-1;
        /* Create disk operations menu */
        sprintf(S,
          "Disk Drive %c:\n"
          "Load disk\n"
          "New disk\n"
          "Eject disk\n"
          " \n"
          "Save DSK image\n"
          "Save FDI image\n",
          'A'+N
        );
        /* Replace all EOLNs with zeroes */
        for(L=0;S[L];L++) if(S[L]=='\n') S[L]='\0';
        /* Redraw background */
        RedrawMain(MainS,J);
        /* Run menu and handle menu selection */
        V=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK4,S,1);
        if(V<0) break;
        switch(V)
        {
          case 1: /* Load disk */
            RedrawMain(MainS,J);
            P=CONFile(CLR_TEXT,CLR_BACK3,".dsk\0.dsk.gz\0.fdi\0.fdi.gz\0",0);
            if(P&&!ChangeDisk(N,P))
            {
              RedrawMain(MainS,J);
              CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot load disk image.\0\0");
            }
            break;
          case 2: /* New disk */
            ChangeDisk(N,"");
            break;
          case 3: /* Eject disk */
            ChangeDisk(N,0);
            break;
          case 5: /* Save .DSK image */
            RedrawMain(MainS,J);
            P=CONFile(CLR_TEXT,CLR_BACK2,".dsk\0",1);
            if(P&&!SaveFDI(&FDD[N],P,FMT_MSXDSK))
            {
              RedrawMain(MainS,J);
              CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot save disk image.\0\0");
            }
            break;
          case 6: /* Save .FDI image */
            RedrawMain(MainS,J);
            P=CONFile(CLR_TEXT,CLR_BACK2,".fdi\0",1);
            if(P&&!SaveFDI(&FDD[N],P,FMT_FDI))
            {
              RedrawMain(MainS,J);
              CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot save disk image.\0\0");
            }
            break;
        }
        break;

      case 8: /* Cheats */
        /* Allocate buffer for cheats */
        PP=malloc(MAXCHEATS*2*16+64);
        if(!PP) break;
        /* Save cheat setting and turn cheats off */
        K=Cheats(CHTS_QUERY);
        Cheats(CHTS_OFF);
        /* Menu loop */
        for(I=1;I;)
        {
          /* Redraw background */
          RedrawMain(MainS,J);

          /* Compose menu */
          sprintf(PP,
            "Cheat Codes\n"
            "Enabled     %c\n"
            "New cheat\n"
            "Done\n"
            " \n",
            K? CON_CHECK:' '
          );
          T=PP+strlen(PP);
          for(L=0;L<CheatCount;++L)
          { strcpy(T,(const char *)CheatCodes[L].Text);T+=strlen(T);*T++='\n'; }
          *T='\0';

          /* Replace all EOLNs with zeroes */
          for(L=0;PP[L];L++) if(PP[L]=='\n') PP[L]='\0';
          /* Run menu */
          V=CONMenu(-1,-1,16,24,CLR_TEXT,CLR_BACK4,PP,I);
          /* Exit submenu on ESC */
          if(V<0) break;
          /* Update selection */
          I=V;
          /* Handle menu selection */
          switch(I)
          {
            case 1:
              K=!K;
              break;
            case 2:
              T=CONInput(-1,-1,CLR_TEXT,CLR_BACK2,"New cheat:",S,14);
              if(T) AddCheat(T);
              break;
            case 3:
              I=0;
              break;
            default:
              /* No cheats above this line */
              if(I<5) break;
              /* Find cheat */
              for(T=PP,L=0;*T&&(L<I);++L) T+=strlen(T)+1;
              /* Delete cheat */
              if(T) DelCheat(T);
              break;
          }
        }
        /* Put cheat settings into effect */
        if(K) Cheats(CHTS_ON);
        /* Done */
        free(PP);
        break;

      case 9: /* Hunt for cheat codes */
        /* Until user quits the menu... */
        for(I=1;I;)
        {
          /* Redraw background */
          RedrawMain(MainS,J);

          /* Compose menu */
          sprintf(S,
            "Cheat Hunter\n"
            "Clear all watches\n"
            "Add a new watch\n"
            "Scan watches\n"
            "See cheat codes\n"
            "  \n"
            "Done\n"
          );

          /* Replace all EOLNs with zeroes */
          for(L=0;S[L];L++) if(S[L]=='\n') S[L]='\0';

          /* Run menu */
          V=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK5,S,I);
          /* Exit submenu on ESC */
          if(V<0) break;
          /* Update selection */
          I=V;

          /* Handle menu selection */
          switch(I)
          {
            case 1: InitHUNT();break;
            case 6: I=0;break;

            case 2:
              /* Ask for search value in 0..65535 range */
              for(K=-1;(K<0)&&(P=CONInput(-1,-1,CLR_TEXT,CLR_BACK4,"Watch Value",S,6|CON_DEC));)
              {
                K = strtoul(P,0,10);
                K = K<0x10000? K:-1;
              }

              /* If cancelled, drop out */
              if(!P) { I=0;break; }

              /* Ask for search options */
              for(I=1,V=K,M=0;I;)
              {
                /* Redraw background */
                RedrawMain(MainS,J);

                /* Force 16bit mode for large values */
                if((K>=0x100)||(V>=0x100)) M|=HUNT_16BIT;

                sprintf(S,
                  "Search for %d\n"
                  "New value %5d\n"
                  "  \n"
                  "8bit value    %c\n"
                  "16bit value   %c\n"
                  "  \n"
                  "Constant      %c\n"
                  "Changes by +1 %c\n"
                  "Changes by -1 %c\n"
                  "Changes by +N %c\n"
                  "Changes by -N %c\n"
                  "  \n"
                  "Search now\n",
                  K,V,
                  !(M&HUNT_16BIT)? CON_CHECK:' ',
                  M&HUNT_16BIT?    CON_CHECK:' ',
                  (M&HUNT_MASK_CHANGE)==HUNT_CONSTANT?  CON_CHECK:' ',
                  (M&HUNT_MASK_CHANGE)==HUNT_PLUSONE?   CON_CHECK:' ',
                  (M&HUNT_MASK_CHANGE)==HUNT_MINUSONE?  CON_CHECK:' ',
                  (M&HUNT_MASK_CHANGE)==HUNT_PLUSMANY?  CON_CHECK:' ',
                  (M&HUNT_MASK_CHANGE)==HUNT_MINUSMANY? CON_CHECK:' '
                );

                /* Replace all EOLNs with zeroes */
                for(L=0;S[L];L++) if(S[L]=='\n') S[L]='\0';

                /* Run menu */
                N=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK2,S,I);
                /* Exit submenu on ESC */
                if(N<0) break;
                /* Update selection */
                I=N;

                /* Change options */
                switch(I)
                {
                  case 1:
                    /* Ask for replacement value in 0..65535 range */
                    P  = CONInput(-1,-1,CLR_TEXT,CLR_BACK4,"New Value",S,6|CON_DEC);
                    N  = P? strtoul(P,0,10):-1;
                    V = (N>=0)&&(N<0x10000)? N:V;
                    I  = 1;
                    break;
                  case 3:  M&=~HUNT_16BIT;break;
                  case 4:  M|=HUNT_16BIT;break;
                  case 6:  M=(M&~HUNT_MASK_CHANGE)|HUNT_CONSTANT;break;
                  case 7:  M=(M&~HUNT_MASK_CHANGE)|HUNT_PLUSONE;break;
                  case 8:  M=(M&~HUNT_MASK_CHANGE)|HUNT_MINUSONE;break;
                  case 9:  M=(M&~HUNT_MASK_CHANGE)|HUNT_PLUSMANY;break;
                  case 10: M=(M&~HUNT_MASK_CHANGE)|HUNT_MINUSMANY;break;
                  case 12:
                    /* Search for value RAM */
                    L = AddHUNT(0xC000,0x4000,K,V,M);
                    I = 0;
                    /* Show number of found locations */
                    sprintf(S,"Found %d locations.\n",L);
                    for(L=0;S[L];L++) if(S[L]=='\n') S[L]='\0';
                    CONMsg(-1,-1,-1,-1,CLR_WHITE,CLR_INFO,"Initial Search",S);
                    break;
                }
              }
              I=0;
              break;

            case 3: ScanHUNT();
            /* Fall through */
            case 4: /* Show current cheats */
              /* Find current number of locations, limit it to 32 */
              K = TotalHUNT();
              K = K<32? K:32;

              /* If no locations, show a warning */
              if(!K)
              {
                CONMsg(-1,-1,-1,-1,CLR_WHITE,CLR_INFO,"Empty","No cheats found.\0\0");
                I=0;
                break;
              }

              /* Show cheat selection dialog */
              for(I=1,M=0;I;)
              {
                /* Redraw background */
                RedrawMain(MainS,J);

                /* Compose dialog */
                sprintf(S,"Found %d Cheats\n",K);
                for(L=0;(L<K)&&(strlen(S)<sizeof(S)-64);++L)
                {
                  if(!(PP=(char *)HUNT2Cheat(L,HUNT_MSX))) break;
                  if(strlen(PP)>9) { PP[8]=CON_DOTS;PP[9]='\0'; }
                  sprintf(S+strlen(S),"%-9s %c\n",PP,M&(1<<L)? CON_CHECK:' ');
                }
                strcat(S,"  \nAdd cheats\n");
     
                /* Number of shown locations */
                K=L;
     
                /* Replace all EOLNs with zeroes */
                for(L=0;S[L];L++) if(S[L]=='\n') S[L]='\0';
     
                /* Run menu */
                N=CONMenu(-1,-1,-1,16,CLR_TEXT,CLR_BACK2,S,I);
                /* Exit submenu on ESC */
                if(N<0) break;
                /* Update selection */
                I=N;
     
                /* Toggle checkmarks */
                if((I>=1)&&(I<=K)) M^=1<<(I-1);
                else if(I)
                {
                  /* If there are cheats to add, drop out */
                  if(!M)
                  {
                    CONMsg(-1,-1,-1,-1,CLR_WHITE,CLR_INFO,"Empty","No cheats chosen.\0\0");
                    I=0;
                    break;
                  }

                  /* Disable and clear current cheats */
                  ResetCheats();

                  /* Add found cheats */
                  for(L=0;L<K;++L)
                    if((M&(1<<L))&&(P=(char *)HUNT2Cheat(L,HUNT_MSX)))
                      for(T=0;P;P=T)
                      {
                        T=(char *)strchr(P,';');
                        if(T) *T++='\0';
                        AddCheat(P);
                      }

                  /* Activate new cheats */
                  Cheats(CHTS_ON);
                  I=0;
                }
              }

              /* Done with the menu */
              I=0;
              break;
          }
        }
        break;

      case 11: /* Log MIDI soundtrack */
        MIDILogging(MIDI_TOGGLE);
        break;
      case 12: /* Hit MIDI drums for noise */
        Mode^=MSX_DRUMS;
        break;
      case 14: /* Show real-time FPS */
        UseEffects^=EFF_SHOWFPS;
        SetEffects(UseEffects);
        break;
      case 15: /* Show all sprites */
        Mode^=MSX_ALLSPRITE;
        break;
      case 16: /* Patch DiskROM routines */
        ResetMSX(Mode^MSX_PATCHBDOS,RAMPages,VRAMPages);
        break;
      case 18: /* POKE &hFFFF,&hAA */
        WrZ80(0xFFFF,0xAA);
        J=0;
        break;
      case 19: /* Rewind */
        RewindTape();
        J=0;
        break;
      case 20: /* Reset */
        ResetMSX(Mode,RAMPages,VRAMPages);
        J=0;
        break;
      case 21: /* Quit */
        ExitNow=1;
        J=0;
        break;
      case 23: /* Done */
        J=0;
        break;
    }
  }
}
