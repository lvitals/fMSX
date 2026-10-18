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
/*************************************************************/
#ifndef SDMAPPER_H
#define SDMAPPER_H

#include "Z80.h"

extern int SDMSlot;         /* Primary slot, -1 for none     */
extern int SDMSpi;          /* 1: the SPI port is on         */

int  SDMCard(const char *FileName); /* Attach the next card  */
void SDMReset(void);                /* Reset of the machine  */
byte SDMRead(word A);               /* Read, SPI on, 4000h-4800h */
int  SDMWrite(word A,byte V);       /* Write in page 1: 1 if taken */

#endif /* SDMAPPER_H */
