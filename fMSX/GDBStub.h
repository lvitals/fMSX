/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                        GDBStub.h                        **/
/**                                                         **/
/** gdb's remote protocol, no network: on stdin/stdout      **/
/** (-gdb -, a pipe of the debugger that started fMSX) or a **/
/** terminal of the debugger fMSX starts (-debugger <cmd>). **/
/** gdb (target z80) reads and writes the registers and the **/
/** memory the CPU sees, sets breakpoints, steps, continues **/
/** and interrupts the program.                             **/
/*************************************************************/
#ifndef GDBSTUB_H
#define GDBSTUB_H

#include "Z80.h"

extern const char *GDBDebugger; /* -debugger: its command, 0: none */

/** GDBStdio() ***********************************************/
/** -gdb -: the protocol on the standard input and output   **/
/** as they are now; fMSX's messages to stderr from now on, **/
/** its input /dev/null (keys and serial data are no gdb's).**/
/*************************************************************/
void GDBStdio(void);

/** GDBStart() ***********************************************/
/** With -debugger, start the debugger on a terminal; with  **/
/** it or -gdb -, the program stops before its first        **/
/** instruction for the debugger. Returns 0 on error.       **/
/*************************************************************/
int GDBStart(Z80 *R);

/** GDBActive() **********************************************/
/** 1 while gdb is connected.                               **/
/*************************************************************/
int GDBActive(void);

/** GDBDebugZ80() ********************************************/
/** DebugZ80() while gdb is connected: report the stop and  **/
/** serve gdb until it resumes. Returns 0 to quit.          **/
/*************************************************************/
byte GDBDebugZ80(Z80 *R);

/** GDBPoll() ************************************************/
/** On each interrupt: an interrupt from gdb (Ctrl-C) stops **/
/** the program.                                            **/
/*************************************************************/
void GDBPoll(Z80 *R);

/** GDBStop() ***********************************************/
/** The emulation ends: tell gdb the program exited.        **/
/*************************************************************/
void GDBStop(void);

#endif /* GDBSTUB_H */
