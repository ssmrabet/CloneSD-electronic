/*------------------------------------------------------------------------/
/  MMCv3/SDv1/SDv2 (in SPI mode) control module
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2010, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/
//Edited by A. Morrison to function on PIC32.

#include "p32mx340f512h.h"
#include "diskio.h"
//#include "..\spi_3xx_4xx.h"


/* Definitions for MMC/SDC command */
#define CMD0   (0)			/* GO_IDLE_STATE */
#define CMD1   (1)			/* SEND_OP_COND */
#define ACMD41 (41|0x80)	/* SEND_OP_COND (SDC) */
#define CMD8   (8)			/* SEND_IF_COND */
#define CMD9   (9)			/* SEND_CSD */
#define CMD10  (10)			/* SEND_CID */
#define CMD12  (12)			/* STOP_TRANSMISSION */
#define ACMD13 (13|0x80)	/* SD_STATUS (SDC) */
#define CMD16  (16)			/* SET_BLOCKLEN */
#define CMD17  (17)			/* READ_SINGLE_BLOCK */
#define CMD18  (18)			/* READ_MULTIPLE_BLOCK */
#define CMD23  (23)			/* SET_BLOCK_COUNT */
#define ACMD23 (23|0x80)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24  (24)			/* WRITE_BLOCK */
#define CMD25  (25)			/* WRITE_MULTIPLE_BLOCK */
#define CMD41  (41)			/* SEND_OP_COND (ACMD) */
#define CMD55  (55)			/* APP_CMD */
#define CMD58  (58)			/* READ_OCR */

#if 0	
/* Port Controls  (Platform dependent) */
#define CS_SETOUT() TRISGbits.TRISG9 = 0 
#define CS_LOW()  _LATG9 = 0	/* MMC CS = L */
#define CS_HIGH() _LATG9 = 1	/* MMC CS = H */



//REVEIW THIS LATER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Makes assumptions that sockwp and sockins are on the same port...
// Should probably remove sockport define and then go fix what used it to be general.
#define SOCKPORT	PORTG		/* Socket contact port */
#define SOCKWP	0 // disable write protect (1<<10)		/* Write protect switch (RB10) */
#define SOCKINS	0 // Pretend card is always inserted (1<<11)	/* Card detect switch (RB11) */

#define	FCLK_SLOW()	SPI2BRG = 64		/* Set slow clock (100k-400k) */
#define	FCLK_FAST()	SPI2BRG = 2		/* Set fast clock (depends on the CSD) */



/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/

static volatile
DSTATUS Stat = STA_NOINIT;	/* Disk status */

static volatile
UINT Timer1, Timer2;		/* 1000Hz decrement timer */

static
UINT CardType;



/*-----------------------------------------------------------------------*/
/* Exchange a byte between PIC and MMC via SPI  (Platform dependent)     */
/*-----------------------------------------------------------------------*/

#define xmit_spi(dat) 	xchg_spi(dat)
#define rcvr_spi()		xchg_spi(0xFF)
#define rcvr_spi_m(p)	SPI2BUF = 0xFF; while (!SPI2STATbits.SPIRBF); *(p) = (BYTE)SPI2BUF;

static
BYTE xchg_spi (BYTE dat)
{
	SPI2BUF = dat;
	while (!SPI2STATbits.SPIRBF);
	return (BYTE)SPI2BUF;
}


#endif

#define xmit_spi(x)	xchg_spi(x)

static BYTE rcvr_spi(SDCard *sd) { 
 	return sd->xchg_spi(0xFF); 
}

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/
static BYTE wait_ready (SDCard *sd)
{
	BYTE res;


	sd->Timer2 = 500;	/* Wait for ready in timeout of 500ms */
	rcvr_spi(sd);
	do
		res = rcvr_spi(sd);
	while ((res != 0xFF) && sd->Timer2);

	return res;
}

void deselect(SDCard *sd)  { 
	sd->CS_HIGH(); 
	rcvr_spi(sd); 
}

BYTE select(SDCard *sd) {/* 1:Successful, 0:Timeout */ 
	sd->CS_LOW(); 
	if (wait_ready(sd) != 0xFF) { 
		deselect(sd); 
		return 0; 
	} 
	return 1; 
}

/*-----------------------------------------------------------------------*/
/* Send a command packet to MMC                                          */
/*-----------------------------------------------------------------------*/

BYTE send_cmd (SDCard *sd,
	BYTE cmd,		/* Command byte */
	DWORD arg		/* Argument */
)
{
	BYTE n, res;


	if (cmd & 0x80) {	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
		cmd &= 0x7F;
		res = send_cmd(sd, CMD55, 0);
		if (res > 1) return res;
	}

	/* Select the card and wait for ready */
	deselect(sd);
	if (!select(sd)) return 0xFF;

	/* Send command packet */
	sd->xmit_spi(0x40 | cmd);			/* Start + Command index */
	sd->xmit_spi((BYTE)(arg >> 24));	/* Argument[31..24] */
	sd->xmit_spi((BYTE)(arg >> 16));	/* Argument[23..16] */
	sd->xmit_spi((BYTE)(arg >> 8));		/* Argument[15..8] */
	sd->xmit_spi((BYTE)arg);			/* Argument[7..0] */
	n = 0x01;						/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;		/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;		/* Valid CRC for CMD8(0x1AA) */
	sd->xmit_spi(n);

	/* Receive command response */
	if (cmd == CMD12) rcvr_spi(sd);	/* Skip a stuff byte when stop reading */
	n = 10;							/* Wait for a valid response in timeout of 10 attempts */
	do
		res = rcvr_spi(sd);
	while ((res & 0x80) && --n);

	return res;			/* Return with the response value */
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _READONLY == 0
DRESULT disk_write (SDCard *sd,
	BYTE drv,				/* Physical drive nmuber (0) */
	const BYTE *buff,		/* Pointer to the data to be written */
	DWORD sector,			/* Start sector number (LBA) */
	BYTE count				/* Sector count (1..255) */
)
{
	if (drv || !count) return RES_PARERR;
	if (sd->Stat & STA_NOINIT) return RES_NOTRDY;
	if (sd->Stat & STA_PROTECT) return RES_WRPRT;

	if (!(sd->CardType & CT_BLOCK)) sector *= 512;	/* Convert to byte address if needed */

	if (count == 1) {		/* Single block write */
		if ((send_cmd(sd, CMD24, sector) == 0)	/* WRITE_BLOCK */
			&& xmit_datablock(sd, buff, 0xFE))
			count = 0;
	}
	else {				/* Multiple block write */
		if (sd->CardType & CT_SDC) send_cmd(sd, ACMD23, count);
		if (send_cmd(sd, CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!xmit_datablock(sd, buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(sd, 0, 0xFD))	/* STOP_TRAN token */
				count = 1;
		}
	}
	deselect(sd);

	return count ? RES_ERROR : RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Send a data packet to MMC                                             */
/*-----------------------------------------------------------------------*/
int xmit_datablock (SDCard *sd,	/* 1:OK, 0:Failed */
	const BYTE *buff,	/* 512 byte data block to be transmitted */
	BYTE token			/* Data token */
)
{
	BYTE resp;
	UINT bc = 512;


	if (wait_ready(sd) != 0xFF) return 0;

	sd->xmit_spi(token);		/* Xmit a token */
	if (token != 0xFD) {	/* Not StopTran token */
		do {						/* Xmit the 512 byte data block to the MMC */
			sd->xmit_spi(*buff++);
			sd->xmit_spi(*buff++);
		} while (bc -= 2);
		sd->xmit_spi(0xFF);				/* CRC (Dummy) */
		sd->xmit_spi(0xFF);
		resp = rcvr_spi(sd);			/* Receive a data response */
		if ((resp & 0x1F) != 0x05)	/* If not accepted, return with error */
			return 0;
	}

	return 1;
}
#endif /* _READONLY */

/*-----------------------------------------------------------------------*/
/* Receive a data packet from MMC                                        */
/*-----------------------------------------------------------------------*/
static int rcvr_datablock (SDCard *sd,	/* 1:OK, 0:Failed */
	BYTE *buff,			/* Data buffer to store received data */
	UINT btr			/* Byte count (must be multiple of 4) */
)
{
	BYTE token;


	sd->Timer1 = 100;
	do {							/* Wait for data packet in timeout of 100ms */
		token = rcvr_spi(sd);
	} while ((token == 0xFF) && sd->Timer1);

	if(token != 0xFE) return 0;		/* If not valid data token, retutn with error */

	do {							/* Receive the data block into buffer */
		sd->rcvr_spi_m(buff++);
		sd->rcvr_spi_m(buff++);
		sd->rcvr_spi_m(buff++);
		sd->rcvr_spi_m(buff++);
	} while (btr -= 4);
	rcvr_spi(sd);						/* Discard CRC */
	rcvr_spi(sd);

	return 1;						/* Return with success */
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (SDCard *sd,
	BYTE drv		/* Physical drive nmuber (0) */
)
{
	BYTE n, cmd, ty, ocr[4];


	if (drv) return STA_NOINIT;			/* Supports only single drive */
	if (sd->Stat & STA_NODISK) return sd->Stat;	/* No card in the socket */

	sd->power_on();							/* Force socket power on */
	sd->FCLK_SLOW();
	for (n = 10; n; n--) rcvr_spi(sd);	/* 80 dummy clocks */

	ty = 0;
	if (send_cmd(sd, CMD0, 0) == 1) {			/* Enter Idle state */
		sd->Timer1 = 1000;						/* Initialization timeout of 1000 msec */
		if (send_cmd(sd, CMD8, 0x1AA) == 1) {	/* SDv2? */
			for (n = 0; n < 4; n++) ocr[n] = rcvr_spi(sd);			/* Get trailing return value of R7 resp */
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {				/* The card can work at vdd range of 2.7-3.6V */
				while (sd->Timer1 && send_cmd(sd, ACMD41, 0x40000000));	/* Wait for leaving idle state (ACMD41 with HCS bit) */
				if (sd->Timer1 && send_cmd(sd, CMD58, 0) == 0) {			/* Check CCS bit in the OCR */
					for (n = 0; n < 4; n++) ocr[n] = rcvr_spi(sd);
					ty = (ocr[0] & 0x40) ? CT_SD2|CT_BLOCK : CT_SD2;	/* SDv2 */
				}
			}
		} else {							/* SDv1 or MMCv3 */
			if (send_cmd(sd, ACMD41, 0) <= 1) 	{
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
			}
			while (sd->Timer1 && send_cmd(sd, cmd, 0));		/* Wait for leaving idle state */
			if (!sd->Timer1 || send_cmd(sd, CMD16, 512) != 0)	/* Set read/write block length to 512 */
				ty = 0;
		}
	}
	sd->CardType = ty;
	deselect(sd);

	if (ty) {			/* Initialization succeded */
		sd->Stat &= ~STA_NOINIT;	/* Clear STA_NOINIT */
		sd->FCLK_FAST();
	} else {			/* Initialization failed */
		sd->power_off();
	}

	return sd->Stat;
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (SDCard *sd,
	BYTE drv		/* Physical drive nmuber (0) */
)
{
	if (drv) return STA_NOINIT;		/* Supports only single drive */
	return sd->Stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (SDCard *sd,
	BYTE drv,		/* Physical drive nmuber (0) */
	BYTE *buff,		/* Pointer to the data buffer to store read data */
	DWORD sector,	/* Start sector number (LBA) */
	BYTE count		/* Sector count (1..255) */
)
{
	if (drv || !count) return RES_PARERR;
	if (sd->Stat & STA_NOINIT) return RES_NOTRDY;

	if (!(sd->CardType & CT_BLOCK)) sector *= 512;	/* Convert to byte address if needed */

	if (count == 1) {		/* Single block read */
		if ((send_cmd(sd, CMD17, sector) == 0)	/* READ_SINGLE_BLOCK */
			&& rcvr_datablock(sd, buff, 512))
			count = 0;
	}
	else {				/* Multiple block read */
		if (send_cmd(sd, CMD18, sector) == 0) {	/* READ_MULTIPLE_BLOCK */
			do {
				if (!rcvr_datablock(sd, buff, 512)) break;
				buff += 512;
			} while (--count);
			send_cmd(sd, CMD12, 0);				/* STOP_TRANSMISSION */
		}
	}
	deselect(sd);

	return count ? RES_ERROR : RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _READONLY == 0
static DRESULT w (SDCard *sd,
	BYTE drv,				/* Physical drive nmuber (0) */
	const BYTE *buff,		/* Pointer to the data to be written */
	DWORD sector,			/* Start sector number (LBA) */
	BYTE count				/* Sector count (1..255) */
)
{
	if (drv || !count) return RES_PARERR;
	if (sd->Stat & STA_NOINIT) return RES_NOTRDY;
	if (sd->Stat & STA_PROTECT) return RES_WRPRT;

	if (!(sd->CardType & CT_BLOCK)) sector *= 512;	/* Convert to byte address if needed */

	if (count == 1) {		/* Single block write */
		if ((send_cmd(sd, CMD24, sector) == 0)	/* WRITE_BLOCK */
			&& xmit_datablock(sd, buff, 0xFE))
			count = 0;
	}
	else {				/* Multiple block write */
		if (sd->CardType & CT_SDC) send_cmd(sd, ACMD23, count);
		if (send_cmd(sd, CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!xmit_datablock(sd, buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(sd, 0, 0xFD))	/* STOP_TRAN token */
				count = 1;
		}
	}
	deselect(sd);

	return count ? RES_ERROR : RES_OK;
}
#endif /* _READONLY */

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (SDCard *sd,
	BYTE drv,		/* Physical drive nmuber (0) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive data block */
)
{
	DRESULT res;
	BYTE n, csd[16], *ptr = buff;
	DWORD csize;


	if (drv) return RES_PARERR;
	if (sd->Stat & STA_NOINIT) return RES_NOTRDY;

	res = RES_ERROR;
	switch (ctrl) {
		case CTRL_SYNC :	/* Flush dirty buffer if present */
			if (select(sd)) {
				deselect(sd);
				res = RES_OK;
			}
			break;

		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (WORD) */
			if ((send_cmd(sd, CMD9, 0) == 0) && rcvr_datablock(sd, csd, 16)) {
				if ((csd[0] >> 6) == 1) {	/* SDv2? */
					csize = csd[9] + ((WORD)csd[8] << 8) + 1;
					*(DWORD*)buff = (DWORD)csize << 10;
				} else {					/* SDv1 or MMCv2 */
					n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
					csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
					*(DWORD*)buff = (DWORD)csize << (n - 9);
				}
				res = RES_OK;
			}
			break;

		case GET_SECTOR_SIZE :	/* Get sectors on the disk (WORD) */
			*(WORD*)buff = 512;
			res = RES_OK;
			break;

		case GET_BLOCK_SIZE :	/* Get erase block size in unit of sectors (DWORD) */
			if (sd->CardType & CT_SD2) {	/* SDv2? */
				if (send_cmd(sd, ACMD13, 0) == 0) {		/* Read SD status */
					rcvr_spi(sd);
					if (rcvr_datablock(sd, csd, 16)) {				/* Read partial block */
						for (n = 64 - 16; n; n--) rcvr_spi(sd);	/* Purge trailing data */
						*(DWORD*)buff = 16UL << (csd[10] >> 4);
						res = RES_OK;
					}
				}
			} else {					/* SDv1 or MMCv3 */
				if ((send_cmd(sd, CMD9, 0) == 0) && rcvr_datablock(sd, csd, 16)) {	/* Read CSD */
					if (sd->CardType & CT_SD1) {	/* SDv1 */
						*(DWORD*)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
					} else {					/* MMCv3 */
						*(DWORD*)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
					}
					res = RES_OK;
				}
			}
			break;

		case MMC_GET_TYPE :		/* Get card type flags (1 byte) */
			*ptr = sd->CardType;
			res = RES_OK;
			break;

		case MMC_GET_CSD :	/* Receive CSD as a data block (16 bytes) */
			if ((send_cmd(sd, CMD9, 0) == 0)	/* READ_CSD */
				&& rcvr_datablock(sd, buff, 16))
				res = RES_OK;
			break;

		case MMC_GET_CID :	/* Receive CID as a data block (16 bytes) */
			if ((send_cmd(sd, CMD10, 0) == 0)	/* READ_CID */
				&& rcvr_datablock(sd, buff, 16))
				res = RES_OK;
			break;

		case MMC_GET_OCR :	/* Receive OCR as an R3 resp (4 bytes) */
			if (send_cmd(sd, CMD58, 0) == 0) {	/* READ_OCR */
				for (n = 0; n < 4; n++)
					*((BYTE*)buff+n) = rcvr_spi(sd);
				res = RES_OK;
			}
			break;

		case MMC_GET_SDSTAT :	/* Receive SD statsu as a data block (64 bytes) */
			if (send_cmd(sd, ACMD13, 0) == 0) {	/* SD_STATUS */
				rcvr_spi(sd);
				if (rcvr_datablock(sd, buff, 64))
					res = RES_OK;
			}
			break;

		default:
			res = RES_PARERR;
	}

	deselect(sd);

	return res;
}

/*-----------------------------------------------------------------------*/
/* Device Timer Interrupt Procedure  (Platform dependent)                */
/*-----------------------------------------------------------------------*/
/* This function must be called in period of 1ms                         */

void disk_timerproc (SDCard *sd)
{
#if 0
	static WORD pv;
	WORD p;
	BYTE s;
#endif
	UINT n;


	n = sd->Timer1;						/* 1000Hz decrement timer */
	if (n) sd->Timer1 = --n;
	n = sd->Timer2;
	if (n) sd->Timer2 = --n;
#if 0
	p = pv;
	pv = SOCKPORT & (SOCKWP | SOCKINS);	/* Sample socket switch */

	if (p == pv) {						/* Have contacts stabled? */
		s = Stat;

		if (p & SOCKWP)					/* WP is H (write protected) */
			s |= STA_PROTECT;
		else							/* WP is L (write enabled) */
			s &= ~STA_PROTECT;

		if (p & SOCKINS)				/* INS = H (Socket empty) */
			s |= (STA_NODISK | STA_NOINIT);
		else							/* INS = L (Card inserted) */
			s &= ~STA_NODISK;

		Stat = s;
	}
#endif
}

