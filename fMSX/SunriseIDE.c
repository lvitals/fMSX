/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                       SunriseIDE.c                      **/
/**                                                         **/
/** The Sunrise IDE cartridge: its flash and its ATA port,  **/
/** with ATA disks (LBA and CHS, PIO) on image files. See   **/
/** SunriseIDE.h.                                           **/
/*************************************************************/
#define _FILE_OFFSET_BITS 64
#include "SunriseIDE.h"
#include "MSX.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* The memory maps of MSX.c */
extern byte *RAM[8];
extern byte *EmptyRAM;
extern byte *MemMap[4][4][8];
extern byte PSL[4];

#define ROMSIZE  0x20000    /* 8 banks of 16kB                */
#define HEADS    16         /* geometry for CHS and IDENTIFY  */
#define SPT      63

/* Status bits */
#define ST_BSY   0x80
#define ST_DRDY  0x40
#define ST_DSC   0x10
#define ST_DRQ   0x08
#define ST_ERR   0x01

/* What a disk is transferring */
#define TR_NONE  0
#define TR_READ  1
#define TR_WRITE 2

typedef struct
{
  FILE *F;
  unsigned long Sectors;
  byte Status,Error;
  int Transfer;
  unsigned long Sector;     /* Being transferred              */
  int Left;                 /* Sectors left, the current one in */
  int Pos;                  /* Byte of Buf                    */
  byte Buf[512];
} IDEDisk;

int SRSlot = -1;
int SRRegs = 0;

static byte *ROM = 0;
static byte Control;
static IDEDisk Disks[2];
static int NDisks = 0;
static byte Feature,Count,Lba0,Lba1,Lba2,Head,DevCtrl;
static byte ReadLatch,WriteLatch;

/** SRLoadROM() **********************************************/
/** Load the image of the flash (up to 128kB, repeated).    **/
/*************************************************************/
int SRLoadROM(const char *FileName)
{
  FILE *F;
  size_t N,J;

  if(!ROM&&!(ROM=malloc(ROMSIZE))) return(0);
  memset(ROM,0xFF,ROMSIZE);
  F=fopen(FileName,"rb");
  if(!F) return(0);
  N=fread(ROM,1,ROMSIZE,F);
  fclose(F);
  /* A smaller image repeats, as the address lines it ignores */
  if(N&&(N<ROMSIZE)) for(J=N;J<ROMSIZE;++J) ROM[J]=ROM[J%N];
  return(N>0);
}

/** SRDisk() *************************************************/
/** Attach an image file as the master, then the slave.     **/
/*************************************************************/
int SRDisk(const char *FileName)
{
  IDEDisk *D;
  off_t Size;

  if(NDisks>=2) return(0);
  D=&Disks[NDisks];
  memset(D,0,sizeof(*D));
  D->F=fopen(FileName,"r+b");
  if(!D->F) return(0);
  fseeko(D->F,0,SEEK_END);
  Size=ftello(D->F);
  D->Sectors=(unsigned long)(Size/512);
  if(!D->Sectors) { fclose(D->F);D->F=0;return(0); }
  NDisks++;
  return(1);
}

/* The flash bank in page 1 of the slot, for the CPU */
static void MapBank(void)
{
  byte B,J;
  byte *P;

  if(SRSlot<0) return;
  /* Bits 7-3 of the control register, reversed, are the bank */
  for(B=0,J=0;J<5;++J) if(Control&(0x80>>J)) B|=1<<J;
  B&=(ROMSIZE/0x4000)-1;
  P=ROM? ROM+B*0x4000:EmptyRAM;
  MemMap[SRSlot][0][2]=P;
  MemMap[SRSlot][0][3]=P+0x2000;
  if(PSL[1]==SRSlot) { RAM[2]=MemMap[SRSlot][0][2];RAM[3]=MemMap[SRSlot][0][3]; }
}

static void Signature(void)
{
  Count=1;Lba0=1;Lba1=0;Lba2=0;Head=0;
}

/** SRReset() ************************************************/
/** The cartridge after a reset: all control bits set, as   **/
/** in openMSX (the last bank, the IDE registers on).       **/
/*************************************************************/
void SRReset(void)
{
  int J;

  if(SRSlot<0) return;
  Control=0xFF;
  SRRegs=1;
  MapBank();
  for(J=0;J<2;++J)
  {
    Disks[J].Status=Disks[J].F? ST_DRDY|ST_DSC:0x00;
    Disks[J].Error=0x01;
    Disks[J].Transfer=TR_NONE;
  }
  Feature=0;DevCtrl=0;
  Signature();
}

/* The selected disk, or 0 if there is none */
static IDEDisk *Selected(void)
{
  IDEDisk *D=&Disks[(Head>>4)&1];
  return(D->F? D:0);
}

/* The sector of the task file registers */
static unsigned long TaskSector(void)
{
  if(Head&0x40)
    return(((unsigned long)(Head&0x0F)<<24)|((unsigned long)Lba2<<16)
          |((unsigned long)Lba1<<8)|Lba0);
  /* CHS: cylinder, head, sector from 1 */
  return(((((unsigned long)Lba2<<8)|Lba1)*HEADS+(Head&0x0F))*SPT+(Lba0? Lba0-1:0));
}

static void ReadSector(IDEDisk *D)
{
  size_t N;

  memset(D->Buf,0,512);
  fseeko(D->F,(off_t)D->Sector*512,SEEK_SET);
  N=fread(D->Buf,1,512,D->F);
  (void)N;
  D->Pos=0;
}

static void PutWord(byte *P,int W,unsigned V)
{
  P[W*2]=V&0xFF;
  P[W*2+1]=(V>>8)&0xFF;
}

/* An ATA string: two characters per word, the first in the high byte */
static void PutString(byte *P,int W,int Words,const char *S)
{
  int J;
  char C[2];

  for(J=0;J<Words;++J)
  {
    C[0]=*S? *S++:' ';
    C[1]=*S? *S++:' ';
    P[(W+J)*2]=C[1];
    P[(W+J)*2+1]=C[0];
  }
}

static void Identify(IDEDisk *D)
{
  unsigned long Cyl,Lba;

  memset(D->Buf,0,512);
  Cyl=D->Sectors/(HEADS*SPT);
  if(Cyl>16383) Cyl=16383;
  Lba=D->Sectors>0x0FFFFFFFUL? 0x0FFFFFFFUL:D->Sectors;
  PutWord(D->Buf,0,0x0040);           /* fixed disk */
  PutWord(D->Buf,1,Cyl);
  PutWord(D->Buf,3,HEADS);
  PutWord(D->Buf,6,SPT);
  PutString(D->Buf,10,10,D==&Disks[0]? "FMSX0001":"FMSX0002");
  PutString(D->Buf,23,4,"1.0");
  PutString(D->Buf,27,20,"fMSX hard disk");
  PutWord(D->Buf,47,0x8001);
  PutWord(D->Buf,49,0x0200);          /* LBA */
  PutWord(D->Buf,51,0x0200);
  PutWord(D->Buf,53,0x0003);
  PutWord(D->Buf,54,Cyl);
  PutWord(D->Buf,55,HEADS);
  PutWord(D->Buf,56,SPT);
  PutWord(D->Buf,57,(Cyl*HEADS*SPT)&0xFFFF);
  PutWord(D->Buf,58,(Cyl*HEADS*SPT)>>16);
  PutWord(D->Buf,60,Lba&0xFFFF);
  PutWord(D->Buf,61,Lba>>16);
  PutWord(D->Buf,64,0x0003);          /* PIO 3 and 4 */
  D->Pos=0;
  D->Left=1;
  D->Transfer=TR_READ;
  D->Sector=0xFFFFFFFFUL;             /* (nothing after it) */
  D->Status=ST_DRDY|ST_DSC|ST_DRQ;
}

static void Command(byte V)
{
  IDEDisk *D=Selected();
  unsigned long S;
  int N;

  if(!D) return;
  D->Error=0;
  switch(V)
  {
    case 0x20: case 0x21:             /* READ SECTORS  */
    case 0x30: case 0x31:             /* WRITE SECTORS */
      S=TaskSector();
      N=Count? Count:256;
      if(S+N>D->Sectors) { D->Error=0x10;D->Status=ST_DRDY|ST_DSC|ST_ERR;return; }
      D->Sector=S;
      D->Left=N;
      D->Pos=0;
      if(V<0x30) { D->Transfer=TR_READ;ReadSector(D); }
      else D->Transfer=TR_WRITE;
      D->Status=ST_DRDY|ST_DSC|ST_DRQ;
      break;
    case 0xEC:                        /* IDENTIFY DEVICE */
      Identify(D);
      break;
    case 0x90:                        /* EXECUTE DEVICE DIAGNOSTIC */
      D->Error=0x01;
      D->Status=ST_DRDY|ST_DSC;
      Signature();
      break;
    case 0x10: case 0x91: case 0xEF:  /* RECALIBRATE, INITIALIZE, SET FEATURES */
    case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE5: case 0xE7:
      D->Status=ST_DRDY|ST_DSC;
      break;
    default:
      D->Error=0x04;                  /* ABRT */
      D->Status=ST_DRDY|ST_DSC|ST_ERR;
      break;
  }
}

/* The next byte pair of the data register, read */
static byte DataRead(int High)
{
  IDEDisk *D=Selected();
  byte V;

  if(High) return(ReadLatch);
  if(!D||(D->Transfer!=TR_READ)) { ReadLatch=0xFF;return(0xFF); }
  V=D->Buf[D->Pos];
  ReadLatch=D->Buf[D->Pos+1];
  D->Pos+=2;
  if(D->Pos>=512)
  {
    if(--D->Left>0) { D->Sector++;ReadSector(D); }
    else { D->Transfer=TR_NONE;D->Status=ST_DRDY|ST_DSC; }
  }
  return(V);
}

static void DataWrite(int High,byte V)
{
  IDEDisk *D=Selected();

  if(!High) { WriteLatch=V;return; }
  if(!D||(D->Transfer!=TR_WRITE)) return;
  D->Buf[D->Pos]=WriteLatch;
  D->Buf[D->Pos+1]=V;
  D->Pos+=2;
  if(D->Pos>=512)
  {
    fseeko(D->F,(off_t)D->Sector*512,SEEK_SET);
    fwrite(D->Buf,1,512,D->F);
    fflush(D->F);
    D->Pos=0;
    if(--D->Left>0) D->Sector++;
    else { D->Transfer=TR_NONE;D->Status=ST_DRDY|ST_DSC; }
  }
}

/** SRRead() *************************************************/
/** A read of 7C00h-7FFFh of the slot with the registers on. **/
/*************************************************************/
byte SRRead(word A)
{
  IDEDisk *D;

  if(A<0x7E00) return(DataRead(A&1));
  if(A>=0x7E10) return(0xFF);
  D=Selected();
  switch(A&0x0F)
  {
    case 0x0: return(DataRead(0));
    case 0x1: return(D? D->Error:0x00);
    case 0x2: return(Count);
    case 0x3: return(Lba0);
    case 0x4: return(Lba1);
    case 0x5: return(Lba2);
    case 0x6: return(Head);
    case 0x7:
    case 0xE: return(D? D->Status:0x00);  /* nobody answers: 00h */
  }
  return(0xFF);
}

/** SRWrite() ************************************************/
/** A write to page 1 of the slot: 1 if the cartridge took  **/
/** it (the control register, or a register while on).      **/
/*************************************************************/
int SRWrite(word A,byte V)
{
  int J;

  if((A&0xBF04)==0x0104)
  {
    Control=V;
    SRRegs=V&0x01;
    MapBank();
    return(1);
  }
  if(!SRRegs||(A<0x7C00)) return(0);
  if(A<0x7E00) { DataWrite(A&1,V);return(1); }
  if(A>=0x7E10) return(1);
  switch(A&0x0F)
  {
    case 0x0: DataWrite(0,V);break;
    case 0x1: Feature=V;break;
    case 0x2: Count=V;break;
    case 0x3: Lba0=V;break;
    case 0x4: Lba1=V;break;
    case 0x5: Lba2=V;break;
    case 0x6: Head=V;break;
    case 0x7: Command(V);break;
    case 0xE:                         /* device control: SRST */
      if((DevCtrl&0x04)&&!(V&0x04))
        for(J=0;J<2;++J)
          if(Disks[J].F)
          {
            Disks[J].Transfer=TR_NONE;
            Disks[J].Status=ST_DRDY|ST_DSC;
            Disks[J].Error=0x01;
            Signature();
          }
      DevCtrl=V;
      break;
  }
  return(1);
}
