/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                       SunriseIDE.h                      **/
/**                                                         **/
/** The Sunrise IDE cartridge in the cartridge slot SRSlot: **/
/** a flash of 16kB banks in 4000h-7FFFh, and an ATA port   **/
/** with a master and a slave on image files.               **/
/**   4104h     (write) bit 0: IDE registers on; bits 7-3,  **/
/**             in reverse order, the bank of the flash     **/
/**   7C00h-7DFFh       data, 16 bits: an even address gives **/
/**                     the low byte, an odd one the high    **/
/**   7E00h-7E0Fh       the ATA registers                   **/
/** as in the SunriseIDE of openMSX.                        **/
/*************************************************************/
#ifndef SUNRISEIDE_H
#define SUNRISEIDE_H

#include "Z80.h"

extern int SRSlot;          /* Primary slot, -1 for none     */
extern int SRRegs;          /* 1: the IDE registers are on   */

int  SRLoadROM(const char *FileName); /* The flash image      */
int  SRDisk(const char *FileName);    /* Next disk: master, slave */
void SRReset(void);                   /* Reset of the machine */
byte SRRead(word A);                  /* Read, 7C00h-7FFFh, regs on */
int  SRWrite(word A,byte V);          /* Write in page 1: 1 if taken */

#endif /* SUNRISEIDE_H */
