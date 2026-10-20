/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                        SDMapper.h                       **/
/**                                                         **/
/** The SD Mapper cartridge of Fabio Belavenuto (msxsdmapper) **/
/** with its mapper switched off: a flash in ASCII16 pages  **/
/** and an SPI port to two SD cards, both in the cartridge  **/
/** slot SDMSlot:                                           **/
/**   6000h     (write) flash page of 4000h-7FFFh           **/
/**   6001h     (write) bit 0: 1 = SPI port on              **/
/**   4000h-47FFh       SPI data: a read gives the byte of   **/
/**                     the previous transfer and starts one **/
/**                     sending FFh; a write sends the byte  **/
/**   4800h     (read)  b0,b1: 0 = card 0/1 inserted;        **/
/**                     b2,b3: 0 = card 0/1 write protected  **/
/**             (write) b0,b1: chip select of card 0/1 (0 on) **/
/** The cards are SDHC (block addressing) on image files.   **/
/** With SDMRam (its RAM switch on, in Mapper mode) the slot **/
/** is expanded: the flash and the SPI port in subslot 1,   **/
/** a 512kB memory mapper (ports FCh-FFh, page n = segment n **/
/** after a reset) in subslot 3, as in its CPLD.            **/
/** With SDMV2 it is the SD Mapper v2 (msxsdmapperv2):      **/
/**   6000h-67FFh (write) flash page of 4000h-7FFFh, 0-7    **/
/**   7B00h-7EFFh SPI data, with page 7 in 4000h-7FFFh only **/
/**   7FF0h     (write) b0,b1: 1 = card 1/2 selected        **/
/**             (read)  no card selected: b0 = RAM switch;  **/
/**                     one: b1 = 0 inserted, b2 = 1 write  **/
/**                     protected, b0 = 1 changed           **/
/**   7FF1h     timer (here always 0, expired)              **/
/** and with SDMRam the flash is in subslot 0, the mapper   **/
/** in subslot 1.                                           **/
/*************************************************************/
#ifndef SDMAPPER_H
#define SDMAPPER_H

#include "Z80.h"

extern int SDMSlot;         /* Primary slot, -1 for none     */
extern int SDMSpi;          /* 1: the SPI port is on         */
extern int SDMRam;          /* 1: the mapper and expander on */
extern int SDMV2;           /* 1: the SD Mapper v2 (-sdmv2)  */
extern int SDMLate;         /* 1: its RAM shows up only once */
                            /* its header is read (-sdmlate) */

#define SDMSUB     (SDMRam&&!SDMV2? 1:0) /* Subslot of the flash */
#define SDMRAMSUB  (SDMV2? 1:3)  /* Subslot of the mapper     */
#define SDMRAMSEGS 32            /* 16kB segments of it       */

int  SDMCard(int N,const char *FileName); /* Card N, 0 = next */
void SDMReset(void);                /* Reset of the machine  */
int  SDMPort(word A);               /* 1: A reads the SPI port   */
byte SDMRead(word A);               /* Read of the SPI port      */
int  SDMWrite(word A,byte V);       /* Write in page 1: 1 if taken */

#endif /* SDMAPPER_H */
