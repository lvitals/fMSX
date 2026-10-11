/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                        GDBStub.c                        **/
/**                                                         **/
/** The remote protocol of gdb, no network: on the standard **/
/** input and output (-gdb -: the debugger started fMSX,    **/
/** "target remote | fmsx -gdb - ..."), or on a terminal    **/
/** fMSX gives the debugger it starts (-debugger <command>: **/
/** gdb's serial line, "target remote /dev/pts/N"). Packets **/
/** are $data#checksum, acknowledged with +. The registers  **/
/** are gdb's z80 ones, 16 bits each, little-endian: AF BC  **/
/** DE HL SP PC IX IY AF' BC' DE' HL' IR. Breakpoints (Z0,  **/
/** Z1) are flags of BreakZ80, not instructions written to  **/
/** the memory, which is ROM where the program runs.        **/
/*************************************************************/
#define _XOPEN_SOURCE 600
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "GDBStub.h"
#include "MSX.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <poll.h>
#include <fcntl.h>

#define MAXPACKET 4096

const char *GDBDebugger = 0;

static int InFd = -1;            /* from gdb                     */
static int OutFd = -1;           /* to gdb                       */
static int Owned = 0;            /* -gdb -: gdb started fMSX: a  */
                                 /* connection ended without a   */
                                 /* detach ends fMSX too         */
static pid_t Child = 0;          /* -debugger: the debugger      */
static int Running = 0;          /* resumed, no stop reported    */
static int Stepping = 0;         /* resumed by a step            */
static int Interrupted = 0;      /* stopped by gdb's Ctrl-C      */
static byte Breaks[0x10000];     /* BreakZ80 while connected     */

static const char Hex[] = "0123456789abcdef";

/* The memory map (MSX.c) */
extern byte *RAM[8],*EmptyRAM,*RAMData;
extern byte *MemMap[4][4][8];

/** InProgram() **********************************************/
/** PC is in the program: a page of a cartridge in slot 1  **/
/** or 2, or of the RAM - not of the BIOS, the SUB-ROM, the **/
/** disk ROM or a ROM fMSX added (slots 0 and 3). The same  **/
/** address is other code there: a breakpoint only stops    **/
/** the program's.                                          **/
/*************************************************************/
static int InProgram(word PC)
{
  byte *P = RAM[PC>>13];
  int S,J,K;

  if(P==EmptyRAM) return(0);
  if(RAMData&&(P>=RAMData)&&(P<RAMData+RAMPages*0x4000)) return(1);
  for(S=1;S<=2;++S)
    for(J=0;J<4;++J)
      for(K=0;K<8;++K)
        if(MemMap[S][J][K]==P) return(1);
  return(0);
}

static int HexVal(char C)
{
  if((C>='0')&&(C<='9')) return(C-'0');
  if((C>='a')&&(C<='f')) return(C-'a'+10);
  if((C>='A')&&(C<='F')) return(C-'A'+10);
  return(-1);
}

/** Close the connection: the program runs on alone.        **/
static void Disconnect(Z80 *R)
{
  if(OutFd>=0) close(OutFd);
  if((InFd>=0)&&(InFd!=OutFd)) close(InFd);
  InFd     = -1;
  OutFd    = -1;
  Running  = 0;
  BreakZ80 = 0;
  if(R) R->Trace=0;
}

static int GetChar(void)
{
  unsigned char C;
  int N;

  do N=read(InFd,&C,1); while((N<0)&&(errno==EINTR));
  return(N==1? C:-1);
}

static int PutPacket(const char *Data)
{
  char Buf[MAXPACKET+8];
  unsigned int Sum,J;
  int N;

  for(Sum=0,J=0;Data[J];J++) Sum+=(unsigned char)Data[J];
  N=snprintf(Buf,sizeof(Buf),"$%s#%c%c",Data,Hex[(Sum>>4)&15],Hex[Sum&15]);
  return(write(OutFd,Buf,N)==N);
}

/** The next packet's data into Buf (acknowledged); -1 when **/
/** the connection ends.                                    **/
static int GetPacket(char *Buf,int Size)
{
  int C,N,Sum,Got;

  for(;;)
  {
    do if((C=GetChar())<0) return(-1); while(C!='$');
    for(N=0,Sum=0;;)
    {
      if((C=GetChar())<0) return(-1);
      if(C=='#') break;
      if(N<Size-1) Buf[N++]=C;
      Sum+=C;
    }
    Buf[N]='\0';
    if((C=GetChar())<0) return(-1);
    Got=HexVal(C)<<4;
    if((C=GetChar())<0) return(-1);
    Got|=HexVal(C);
    if(Got==(Sum&255)) { if(write(OutFd,"+",1)!=1) return(-1);return(N); }
    if(write(OutFd,"-",1)!=1) return(-1);
  }
}

/** Register N (gdb's order) of the CPU **/
static pair *Reg(Z80 *R,int N)
{
  switch(N)
  {
    case 0:  return(&R->AF);
    case 1:  return(&R->BC);
    case 2:  return(&R->DE);
    case 3:  return(&R->HL);
    case 4:  return(&R->SP);
    case 5:  return(&R->PC);
    case 6:  return(&R->IX);
    case 7:  return(&R->IY);
    case 8:  return(&R->AF1);
    case 9:  return(&R->BC1);
    case 10: return(&R->DE1);
    case 11: return(&R->HL1);
  }
  return(0);
}

#define NREGS 13

static word GetReg(Z80 *R,int N)
{
  return(N==12? (R->I<<8)|R->R:Reg(R,N)->W);
}

static void SetReg(Z80 *R,int N,word V)
{
  if(N==12) { R->I=V>>8;R->R=V&255; }
  else Reg(R,N)->W=V;
}

static char *PutWord(char *P,word V)
{
  *P++=Hex[(V>>4)&15];*P++=Hex[V&15];
  *P++=Hex[(V>>12)&15];*P++=Hex[(V>>8)&15];
  return(P);
}

static word GetWord(const char *P)
{
  return((HexVal(P[0])<<4)|HexVal(P[1])|(HexVal(P[2])<<12)|(HexVal(P[3])<<8));
}

int GDBActive(void) { return(InFd>=0); }

void GDBStdio(void)
{
  int N;

  /* the protocol on the input and output fMSX has: fMSX's own messages
     on stderr from now on, and no input of its own (it reads keys and
     serial data from stdin) */
  fflush(stdout);
  OutFd = dup(1);
  InFd  = dup(0);
  dup2(2,1);
  if((N=open("/dev/null",O_RDONLY))>=0) { dup2(N,0);close(N); }
  Owned = 1;
}

/* -debugger: the debugger's command line, run on a terminal of its own
   (a pseudo-terminal: gdb's serial line), the program's .elf beside its
   ROM when there is one */
static int Debugger(void)
{
  struct termios T;
  char *Cmd,*Elf,*P;
  const char *Pts;
  size_t N;
  int M,F;

  if((M=posix_openpt(O_RDWR|O_NOCTTY))<0) { perror("debugger terminal");return(0); }
  if((grantpt(M)<0)||(unlockpt(M)<0)||!(Pts=ptsname(M)))
  { perror("debugger terminal");close(M);return(0); }
  /* the protocol's bytes as they are */
  if(!tcgetattr(M,&T)) { cfmakeraw(&T);tcsetattr(M,TCSANOW,&T); }
  Elf = 0;
  if(ROMName[0])
  {
    N   = strlen(ROMName[0]);
    Elf = malloc(N+5);
    if(!Elf) { close(M);return(0); }
    strcpy(Elf,ROMName[0]);
    P = strrchr(Elf,'.');
    if(P&&!strchr(P,'/')) strcpy(P,".elf"); else strcat(Elf,".elf");
    if(access(Elf,R_OK)) { free(Elf);Elf=0; }
  }
  N   = strlen(GDBDebugger)+strlen(Pts)+(Elf? strlen(Elf):0)+64;
  Cmd = malloc(N);
  if(!Cmd) { free(Elf);close(M);return(0); }
  /* the connection first, before the commands of the command's own
     options (-ex, -x run in order); the program last */
  P = (char *)GDBDebugger+strcspn(GDBDebugger," \t");
  snprintf(Cmd,N,"%.*s -ex 'target remote %s'%s%s%s",(int)(P-GDBDebugger),GDBDebugger,
           Pts,P,Elf? " ":"",Elf? Elf:"");
  free(Elf);
  fflush(stdout);
  fflush(stderr);
  Child = fork();
  if(Child<0) { perror("debugger");free(Cmd);close(M);return(0); }
  if(!Child)
  {
    close(M);
    execl("/bin/sh","sh","-c",Cmd,(char *)0);
    perror("debugger");
    _exit(127);
  }
  free(Cmd);
  /* the terminal is the debugger's now: no keys or serial data for
     fMSX from it (Ctrl-C reaches the program through the debugger) */
  if((F=open("/dev/null",O_RDONLY))>=0) { dup2(F,0);close(F); }
  InFd = OutFd = M;
  Owned = 1;
  return(1);
}

int GDBStart(Z80 *R)
{
  if(GDBDebugger&&!Debugger()) return(0);
  if(InFd<0) return(1);
  memset(Breaks,0,sizeof(Breaks));
  BreakZ80    = Breaks;
  Running     = 0;
  Interrupted = 0;
  /* Stop before the first instruction */
  R->Trace    = 1;
  return(1);
}

void GDBPoll(Z80 *R)
{
  unsigned char C;

  struct pollfd P;

  if((InFd<0)||!Running) return;
  P.fd     = InFd;
  P.events = POLLIN;
  while(poll(&P,1,0)==1)
  {
    if(!(P.revents&POLLIN)||(read(InFd,&C,1)!=1))
    {
      /* gdb gone */
      Disconnect(R);
      if(Owned) ExitNow=1;
      return;
    }
    if(C==0x03) { Interrupted=1;R->Trace=1; }
  }
}

void GDBStop(void)
{
  if((InFd>=0)&&Running) PutPacket("W00");
  Disconnect(0);
  /* the debugger fMSX started has the terminal until it quits */
  if(Child>0) { while((waitpid(Child,0,0)<0)&&(errno==EINTR));Child=0; }
}

byte GDBDebugZ80(Z80 *R)
{
  static char In[MAXPACKET],Out[MAXPACKET];
  unsigned int Addr,Len,J,N;
  char *P;

  /* A breakpoint's address in other code than the program's */
  if(Running&&!Stepping&&!Interrupted&&Breaks[R->PC.W]&&!InProgram(R->PC.W))
  {
    R->Trace=0;
    return(1);
  }

  /* The stop of a resumed program: a breakpoint, a step or Ctrl-C */
  if(Running)
  {
    Running=0;
    if(!PutPacket(Interrupted? "S02":"S05")) { Disconnect(R);return(1); }
    Interrupted=0;
  }

  for(;;)
  {
    if(GetPacket(In,sizeof(In))<0) { Disconnect(R);return(!Owned); }
    Out[0]='\0';
    switch(In[0])
    {
      case '?':
        strcpy(Out,"S05");
        break;
      case 'g':
        for(P=Out,J=0;J<NREGS;J++) P=PutWord(P,GetReg(R,J));
        *P='\0';
        break;
      case 'G':
        for(J=0;(J<NREGS)&&(strlen(In+1)>=4*(J+1));J++) SetReg(R,J,GetWord(In+1+4*J));
        strcpy(Out,"OK");
        break;
      case 'p':
        N=strtoul(In+1,0,16);
        if(N<NREGS) { P=PutWord(Out,GetReg(R,N));*P='\0'; }
        else strcpy(Out,"E01");
        break;
      case 'P':
        N=strtoul(In+1,&P,16);
        if((N<NREGS)&&(*P=='=')&&(strlen(P+1)>=4)) { SetReg(R,N,GetWord(P+1));strcpy(Out,"OK"); }
        else strcpy(Out,"E01");
        break;
      case 'm':
        Addr=strtoul(In+1,&P,16);
        Len=*P==','? strtoul(P+1,0,16):0;
        if(2*Len>=sizeof(Out)) Len=sizeof(Out)/2-1;
        for(P=Out,J=0;J<Len;J++)
        {
          byte V=RdZ80((Addr+J)&0xFFFF);
          *P++=Hex[V>>4];*P++=Hex[V&15];
        }
        *P='\0';
        break;
      case 'M':
        Addr=strtoul(In+1,&P,16);
        Len=*P==','? strtoul(P+1,&P,16):0;
        if(*P!=':') { strcpy(Out,"E01");break; }
        for(P++,J=0;(J<Len)&&P[0]&&P[1];J++,P+=2)
          WrZ80((Addr+J)&0xFFFF,(HexVal(P[0])<<4)|HexVal(P[1]));
        strcpy(Out,"OK");
        break;
      case 'Z':
      case 'z':
        if((In[1]=='0')||(In[1]=='1'))
        {
          Addr=strtoul(In+3,0,16)&0xFFFF;
          Breaks[Addr]=In[0]=='Z';
          strcpy(Out,"OK");
        }
        break;
      case 'c':
      case 's':
        if(In[1]) R->PC.W=strtoul(In+1,0,16);
        Running=1;
        /* A step stops before the next instruction */
        Stepping=In[0]=='s';
        R->Trace=Stepping;
        return(1);
      case 'k':
        Disconnect(R);
        return(0);
      case 'D':
        PutPacket("OK");
        Disconnect(R);
        Owned=0;
        return(1);
      case 'H':
        strcpy(Out,"OK");
        break;
      case 'q':
        if(!strncmp(In,"qSupported",10))
          sprintf(Out,"PacketSize=%x",MAXPACKET-16);
        else if(!strcmp(In,"qAttached")) strcpy(Out,"1");
        else if(!strcmp(In,"qC")) strcpy(Out,"QC1");
        else if(!strcmp(In,"qfThreadInfo")) strcpy(Out,"m1");
        else if(!strcmp(In,"qsThreadInfo")) strcpy(Out,"l");
        break;
    }
    if(!PutPacket(Out)) { Disconnect(R);return(1); }
  }
}
