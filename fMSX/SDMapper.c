/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                        SDMapper.c                       **/
/**                                                         **/
/** The SD Mapper cartridge: its SPI port and two SD cards  **/
/** (SDHC, SPI mode) on image files. See SDMapper.h.        **/
/*************************************************************/
#define _FILE_OFFSET_BITS 64
#include "SDMapper.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>

#define MAXCARDS 2

/* What a card does with the bytes it gets */
#define ST_CMD    0  /* commands                        */
#define ST_MREAD  1  /* sending blocks (CMD18)          */
#define ST_WTOKEN 2  /* waiting for the data token      */
#define ST_WDATA  3  /* getting a block and its CRC     */

typedef struct
{
  FILE *F;
  unsigned long Blocks;     /* Size of the card, in blocks   */
  int Idle;                 /* In idle state (after CMD0)    */
  int App;                  /* The next command is an ACMD   */
  int Inits;                /* ACMD41 seen while idle        */
  int State;
  int Multi;                /* Writing blocks (CMD25)        */
  unsigned long Block;      /* Block being read or written   */
  byte Cmd[6];
  int CmdN;
  byte Out[600];            /* Bytes the card sends next     */
  int OutN,OutP;
  byte Buf[514];            /* A block written, and its CRC  */
  int BufN;
} SDCard;

int SDMSlot = -1;
int SDMSpi  = 0;

static SDCard Cards[MAXCARDS];
static int NCards = 0;
static byte CS    = 0x03;   /* Chip selects, 0 = selected    */
static byte Port1 = 0xFF;   /* Byte of the last transfer     */

/** SDMCard() ************************************************/
/** Attach an image file as the next card.                  **/
/*************************************************************/
int SDMCard(const char *FileName)
{
  SDCard *C;
  off_t Size;

  if(NCards>=MAXCARDS) return(0);
  C=&Cards[NCards];
  memset(C,0,sizeof(*C));
  C->F=fopen(FileName,"r+b");
  if(!C->F) return(0);
  fseeko(C->F,0,SEEK_END);
  Size=ftello(C->F);
  /* SDHC sizes go in units of 512kB */
  C->Blocks=(unsigned long)(Size/512)&~1023UL;
  if(!C->Blocks) { fclose(C->F);C->F=0;return(0); }
  NCards++;
  return(1);
}

/** SDMReset() ***********************************************/
/** The cartridge after a reset of the machine.             **/
/*************************************************************/
void SDMReset(void)
{
  int J;

  SDMSpi=0;
  CS=0x03;
  Port1=0xFF;
  for(J=0;J<NCards;++J)
  {
    Cards[J].Idle=0;Cards[J].App=0;Cards[J].State=ST_CMD;
    Cards[J].CmdN=0;Cards[J].OutN=Cards[J].OutP=0;
  }
}

static void Put(SDCard *C,byte V)
{
  if(C->OutN<(int)sizeof(C->Out)) C->Out[C->OutN++]=V;
}

/* A data token, the 512 bytes of the current block and a CRC */
static void PutBlock(SDCard *C)
{
  byte *P=C->Out+C->OutN;
  int J;

  Put(C,0xFF);
  Put(C,0xFE);
  P=C->Out+C->OutN;
  memset(P,0,512);
  if(C->Block<C->Blocks)
  {
    fseeko(C->F,(off_t)C->Block*512,SEEK_SET);
    J=fread(P,1,512,C->F);
    (void)J;
  }
  C->OutN+=512;
  Put(C,0xFF);
  Put(C,0xFF);
  C->Block++;
}

/* A register (CSD or CID) as a data block of 16 bytes */
static void PutReg(SDCard *C,const byte *R)
{
  int J;

  Put(C,0xFF);
  Put(C,0xFE);
  for(J=0;J<16;++J) Put(C,R[J]);
  Put(C,0xFF);
  Put(C,0xFF);
}

static void Command(SDCard *C)
{
  static const byte CID[16] =
  { 0x03,'F','M','f','M','S','X','S','D',0x10,0x12,0x34,0x56,0x78,0x01,0x5B };
  byte CSD[16] =
  { 0x40,0x0E,0x00,0x32,0x5B,0x59,0x00,0x00,0x00,0x00,0x7F,0x80,0x0A,0x40,0x00,0x01 };
  unsigned long Arg,Size;
  byte Cmd,R1;
  int App;

  Cmd = C->Cmd[0]&0x3F;
  Arg = ((unsigned long)C->Cmd[1]<<24)|((unsigned long)C->Cmd[2]<<16)
      | ((unsigned long)C->Cmd[3]<<8)|C->Cmd[4];
  if(getenv("SDM_TRACE"))
    fprintf(stderr,"SD%d %sCMD%d %08lX\n",(int)(C-Cards),C->App? "A":"",Cmd,Arg);
  App = C->App;
  C->App = 0;
  R1 = C->Idle? 0x01:0x00;

  /* A response comes one byte after the command */
  C->OutN=C->OutP=0;
  Put(C,0xFF);

  if(App&&(Cmd==41)) { Put(C,R1);if(C->Idle&&(++C->Inits>1)) C->Idle=0;return; }
  if(App&&(Cmd==23)) { Put(C,R1);return; }

  switch(Cmd)
  {
    case 0:  C->Idle=1;C->Inits=0;C->State=ST_CMD;Put(C,0x01);break;
    case 1:  Put(C,R1);if(C->Idle&&(++C->Inits>1)) C->Idle=0;break;
    case 8:  Put(C,R1);Put(C,0x00);Put(C,0x00);Put(C,(Arg>>8)&0x0F);Put(C,Arg&0xFF);break;
    case 55: C->App=1;Put(C,R1);break;
    case 58: Put(C,R1);Put(C,C->Idle? 0x40:0xC0);Put(C,0xFF);Put(C,0x80);Put(C,0x00);break;
    case 16: Put(C,R1);break;
    case 9:
      Size=C->Blocks/1024-1;
      CSD[7]=(Size>>16)&0x3F;CSD[8]=(Size>>8)&0xFF;CSD[9]=Size&0xFF;
      Put(C,R1);PutReg(C,CSD);
      break;
    case 10: Put(C,R1);PutReg(C,CID);break;
    case 17:
      if(Arg>=C->Blocks) { Put(C,R1|0x40);break; }
      Put(C,R1);C->Block=Arg;PutBlock(C);
      break;
    case 18:
      if(Arg>=C->Blocks) { Put(C,R1|0x40);break; }
      Put(C,R1);C->Block=Arg;C->State=ST_MREAD;PutBlock(C);
      break;
    case 12: C->State=ST_CMD;Put(C,0xFF);Put(C,R1);break;
    case 24:
    case 25:
      if(Arg>=C->Blocks) { Put(C,R1|0x40);break; }
      Put(C,R1);C->Block=Arg;C->Multi=(Cmd==25);C->State=ST_WTOKEN;
      break;
    default: Put(C,R1|0x04);break;
  }
}

/* One SPI transfer with card C: the byte it sends for In */
static byte Transfer(SDCard *C,byte In)
{
  byte Out;

  Out = C->OutP<C->OutN? C->Out[C->OutP++]:0xFF;
  if(C->OutP>=C->OutN)
  {
    C->OutN=C->OutP=0;
    if((C->State==ST_MREAD)&&(C->CmdN==0)) PutBlock(C);
  }

  switch(C->State)
  {
    case ST_WTOKEN:
      if((In==0xFE)||(C->Multi&&(In==0xFC))) { C->State=ST_WDATA;C->BufN=0;return(Out); }
      if(C->Multi&&(In==0xFD))
      {
        /* Stop token: busy for a byte, then ready */
        C->State=ST_CMD;C->OutN=C->OutP=0;Put(C,0xFF);Put(C,0x00);
        return(Out);
      }
      break;
    case ST_WDATA:
      C->Buf[C->BufN++]=In;
      if(C->BufN==514)
      {
        if(C->Block<C->Blocks)
        {
          fseeko(C->F,(off_t)C->Block*512,SEEK_SET);
          fwrite(C->Buf,1,512,C->F);
          fflush(C->F);
        }
        C->Block++;
        /* Data accepted, then busy for a byte */
        C->OutN=C->OutP=0;Put(C,0x05);Put(C,0x00);
        C->State=C->Multi? ST_WTOKEN:ST_CMD;
      }
      return(Out);
  }

  /* Commands (also during ST_MREAD, for CMD12) */
  if(!C->CmdN)
  {
    if((In&0xC0)==0x40)
    {
      C->Cmd[0]=In;C->CmdN=1;
      /* CMD12 stops the blocks being sent */
      if((In&0x3F)==12) { C->OutN=C->OutP=0;C->State=ST_CMD; }
    }
  }
  else
  {
    C->Cmd[C->CmdN++]=In;
    if(C->CmdN==6) { C->CmdN=0;Command(C); }
  }
  return(Out);
}

/* A transfer on the SPI bus: the selected card answers */
static byte Bus(byte In)
{
  int J;

  for(J=0;J<NCards;++J)
    if(!(CS&(1<<J))) return(Transfer(&Cards[J],In));
  return(0xFF);
}

/** SDMRead() ************************************************/
/** A read of 4000h-4800h of the slot with the SPI port on. **/
/*************************************************************/
byte SDMRead(word A)
{
  byte V;

  if(A==0x4800)
  {
    /* Missing cards read as not inserted, none protected */
    V=0x0C;
    if(NCards<1) V|=0x01;
    if(NCards<2) V|=0x02;
    return(V);
  }
  V=Port1;
  Port1=Bus(0xFF);
  return(V);
}

/** SDMWrite() ***********************************************/
/** A write to page 1 of the slot: 1 if the SPI port or the **/
/** mode register took it.                                  **/
/*************************************************************/
int SDMWrite(word A,byte V)
{
  int J;

  if(A==0x6001) { SDMSpi=V&0x01;return(1); }
  if(!SDMSpi||(A>0x4800)) return(0);
  if(A==0x4800)
  {
    /* A card deselected forgets a half-received command */
    for(J=0;J<NCards;++J)
      if(!(CS&(1<<J))&&(V&(1<<J))) Cards[J].CmdN=0;
    CS=V&0x03;
    return(1);
  }
  Port1=Bus(V);
  return(1);
}
