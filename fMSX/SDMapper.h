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
/*************************************************************/
#ifndef SDMAPPER_H
#define SDMAPPER_H

#include "Z80.h"

extern int SDMSlot;         /* Primary slot, -1 for none     */
extern int SDMSpi;          /* 1: the SPI port is on         */
extern int SDMRam;          /* 1: the mapper and expander on */
extern int SDMLate;         /* 1: its RAM shows up only once */
                            /* its header is read (-sdmlate) */

#define SDMSUB     (SDMRam? 1:0) /* Subslot of the flash      */
#define SDMRAMSUB  3             /* Subslot of the mapper     */
#define SDMRAMSEGS 32            /* 16kB segments of it       */

int  SDMCard(const char *FileName); /* Attach the next card  */
void SDMReset(void);                /* Reset of the machine  */
byte SDMRead(word A);               /* Read, SPI on, 4000h-4800h */
int  SDMWrite(word A,byte V);       /* Write in page 1: 1 if taken */

#endif /* SDMAPPER_H */
