#define GetSystemClock() (80000000L)

#include "p32mx340f512h.h"
#include <p32xxxx.h>
#include "ff.h"
#include "diskio.h"
#include"lcd.h"

#pragma config FPLLMUL  = MUL_20        // PLL Multiplier 20, 2
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider
#pragma config FPLLODIV = DIV_1         // PLL Output Divider
#pragma config FPBDIV   = DIV_2         // Peripheral Clock divisor
#pragma config FWDTEN   = OFF           // Watchdog Timer
#pragma config WDTPS    = PS1           // Watchdog Timer Postscale
#pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = HS            // Primary Oscillator
#pragma config IESO     = OFF           // Internal/External Switch-over
#pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable (KLO was off)
#pragma config FNOSC    = PRIPLL        // Oscillator Selection
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx2      // ICE/ICD Comm Channel Select

//File system object
FATFS Fatfs1, Fatfs2;
FIL file1, file2;		/* File objects */
SDCard sd1, sd2;

//======================== specific functions for SDCard 1 on SPI1 =================== 
void sd1_cs_low() {
	_RB2 = 0;	/* MMC CS = L */
}

void sd1_cs_high() {
	_RB2 = 1;	/* MMC CS = H */
}

void sd1_fclk_slow() {
	SPI1BRG = 64	;	/* Set slow clock (100k-400k) */
}

void sd1_fclk_fast() {
	SPI1BRG = 2;		/* Set fast clock (depends on the CSD) */
}

void sd1_power_on() {
	/* SD_CARD_SELECT pin set to output */
        _TRISB2 = 0; //SS pin
        _TRISF6 = 0; _TRISF3 = 0; _TRISF2 = 1;

        /* Clock = PBCLK/32 = 250kHz [ PBCLK/(2*(SP1BRG+1)) ] */
        SPI1BRG = 0;

        /* Master Mode, CKE = 1, SMP = 0, 8-BIT, SPI ON */
        SPI1CON = 0x8120;
	// configured for ~400 kHz operation - reset later to 20 MHz
	SPI1CONbits.ON = 1;
}

void sd1_power_off() {
	select(&sd1);			/* Wait for card ready */
	deselect(&sd1);
	SPI1CONbits.ON = 0;			/* Disable SPI2 */
	sd1.Stat |= STA_NOINIT;	/* Set STA_NOINIT */
}

BYTE sd1_xchg(BYTE dat) {
	SPI1BUF = dat;
	while (!SPI1STATbits.SPIRBF);
	return (BYTE)SPI1BUF;
}

void sd1_rcvr(BYTE *rdat) {
	SPI1BUF = 0xFF; 
	while (!SPI1STATbits.SPIRBF); 
	*(rdat) = (BYTE)SPI1BUF;
}

void card1_init() {
	sd1.index = 0;
	
	sd1.CS_LOW = sd1_cs_low;
	sd1.CS_HIGH = sd1_cs_high;
	sd1.FCLK_SLOW = sd1_fclk_slow;
	sd1.FCLK_FAST = sd1_fclk_fast;
	sd1.power_on = sd1_power_on;
	sd1.power_off = sd1_power_off;
	sd1.xchg_spi = sd1_xchg;
	sd1.rcvr_spi_m = sd1_rcvr;
}

// ======================= specific functions for SDCard 2 on SPI2 =====================
void sd2_cs_low() {
	_LATG9 = 0;	/* MMC CS = H */
}

void sd2_cs_high() {
	_LATG9 = 1;	/* MMC CS = H */
}

void sd2_fclk_slow() {
	SPI2BRG = 64	;	/* Set slow clock (100k-400k) */
}

void sd2_fclk_fast() {
	SPI2BRG = 2;		/* Set fast clock (depends on the CSD) */
}

void sd2_power_on() {
	// Setup CS as output
	TRISGbits.TRISG9 = 0 ;
        _TRISG6 = 0; _TRISG8 = 0; _TRISG7 = 1;

        /* Clock = PBCLK/32 = 250kHz [ PBCLK/(2*(SP1BRG+1)) ] */
        SPI2BRG = 0;

        /* Master Mode, CKE = 1, SMP = 0, 8-BIT, SPI ON */
        SPI2CON = 0x8120;
	// configured for ~400 kHz operation - reset later to 20 MHz
	SPI2CONbits.ON = 1;
}

void sd2_power_off() {
	select(&sd2);			/* Wait for card ready */
	deselect(&sd2);
	SPI2CONbits.ON = 0;			/* Disable SPI2 */
	sd2.Stat |= STA_NOINIT;	/* Set STA_NOINIT */
}

BYTE sd2_xchg(BYTE dat) {
	SPI2BUF = dat;
	while (!SPI2STATbits.SPIRBF);
	return (BYTE)SPI2BUF;
}

void sd2_rcvr(BYTE *rdat) {
	SPI2BUF = 0xFF; 
	while (!SPI2STATbits.SPIRBF); 
	*(rdat) = (BYTE)SPI2BUF;
}

void card2_init() {
	sd2.index = 1;
	
	sd2.CS_LOW = sd2_cs_low;
	sd2.CS_HIGH = sd2_cs_high;
	sd2.FCLK_SLOW = sd2_fclk_slow;
	sd2.FCLK_FAST = sd2_fclk_fast;
	sd2.power_on = sd2_power_on;
	sd2.power_off = sd2_power_off;
	sd2.xchg_spi = sd2_xchg;
	sd2.rcvr_spi_m = sd2_rcvr;
}

#define true	1
#define false	0
static unsigned char isOn = false;
void led_on() {
	_RF1 = 1;
	isOn = true;
}

void led_off() {
	_RF1 = 0;
	isOn = false;
}

void led_tog() {

	if(isOn) {
		led_off();
	} else {
		led_on();
	}
}

void delay_ms(int _ms) {
	long ms = _ms * 80; // this is not exactly
	while(ms-- >=0);
}    

/*-----------------------------------------------------------------------*/
/* Find next file                                                        */
/*-----------------------------------------------------------------------*/

FRESULT f_findnext (SDCard* sd,
    DIR* dp,        /* Pointer to the open directory object */
    FILINFO* fno    /* Pointer to the file information structure */
){
    FRESULT res;
    res = f_readdir(sd, dp, fno);       /* Get a directory item */
    return res;
}

/*-----------------------------------------------------------------------*/
/* Find first file                                                       */
/*-----------------------------------------------------------------------*/

FRESULT f_findfirst (
    DIR* dp,                /* Pointer to the blank directory object */
    FILINFO* fno,           /* Pointer to the file information structure */
    const TCHAR* path      /* Pointer to the directory to open */
)
{
    FRESULT res;

    res = f_opendir(&sd1, dp, path);      /* Open the target directory */
    if (res == FR_OK)
        res = f_findnext(&sd1, dp, fno);  /* Find the first item */
    return res;
}
static
FRESULT validate (  /* FR_OK(0): The object is valid, !=0: Invalid */
    SDCard *sd,
    void* obj       /* Pointer to the object FIL/DIR to check validity */
)
{
    FIL *fil = (FIL*)obj;   /* Assuming offset of .fs and .id in the FIL/DIR structure is identical */
 
 
    if (!fil || !fil->fs || !fil->fs->fs_type || fil->fs->id != fil->id || (disk_status(sd, fil->fs->drv) & STA_NOINIT))
        return FR_INVALID_OBJECT;
    return FR_OK;
}
FRESULT f_closedir (
    SDCard *sd,
    DIR *dp     /* Pointer to the directory object to be closed */
)
{
    FRESULT res;
    res = validate(sd, dp);
    if (res == FR_OK) {
            dp->fs = 0;             /* Invalidate directory object */
    }
    return res;
}
static char buff[513];

TCHAR tmp[30];
void mkdirs(TCHAR* path) {
    TCHAR dirs[30];
    BYTE i=0;
    FRESULT res;
    for(;;i++) {
        if(path[i] == '/' || path[i]=='\\') {
            dirs[i] = 0;
            res = f_mkdir(&sd2, dirs);
            if(!(res == FR_OK || res == FR_EXIST)) {
             //   lcd_puts("mkdirs error");
                return;
            }
        } else if(path[i]==0) 
            break;
        dirs[i] = path[i];
    }
}
long accessmode_sd1 = (FA_OPEN_ALWAYS | FA_READ);
long accessmode_sd2 = (FA_CREATE_ALWAYS | FA_OPEN_ALWAYS | FA_CREATE_NEW | FA_WRITE);

#include <string.h>
BYTE docopy(TCHAR *pname) {
    FRESULT FOpenRes = f_open(&sd1, &file1, pname, (BYTE)accessmode_sd1);
    if(FOpenRes != FR_OK) {
      //  lcd_puts("sdcard 1 error");
        return 12;
    }
    mkdirs(pname);
    TCHAR zzz[30];
    strcpy(zzz,  pname);
    FOpenRes=f_open(&sd2, &file2, pname, (BYTE)accessmode_sd2);
    if(FOpenRes != FR_OK) {
       // lcd_puts("sdcard 2 error");
        return 13;
    }

    unsigned int numread=0;
    do {
            led_tog();
            FOpenRes=f_read (&sd1, &file1, &buff, 512, &numread);
            if(FOpenRes != FR_OK) {
                //lcd_puts("read error");
                return 21;
            }
            if (numread > 0) 	{
                   FOpenRes=f_write(&sd2, &file2, &buff, 512, &numread);
                   if(FOpenRes != FR_OK) {
                       // lcd_puts("write error");
                        return 31;
                    }
                    //numread recycled as numwritten
            }
    } while(numread > 0);
    //close first file
    f_close(&sd1, &file1);
    f_close(&sd2, &file2);
}

#include <stdio.h>
void recusive_copy(TCHAR *path) {
    DIR dp;
    FILINFO fno; 
    FRESULT res = f_opendir(&sd1, &dp, path);      /* Open the target directory */
    if (res == FR_OK) {
        res = f_findnext(&sd1, &dp, &fno);  /* Find the first item */
        while(res==FR_OK && fno.fname[0]) {
            //FILINFO fnostat;
            memset(tmp, 0, 30);
            sprintf(tmp, "%s/%s", path, fno.fname);
            //f_stat(&sd1, tmp, &fnostat);
            if(fno.fattrib & AM_DIR) { // is directory ==> recursive
                recusive_copy(tmp);
                continue;
            }
            // else it is file ==> copy it
            docopy(tmp);
            res = f_findnext(&sd1, &dp, &fno);  /* Find the first item */
        }
    }
    f_closedir(&sd1, &dp);
}

int main (void)
{
    /* Set LED pin to output */
    _TRISF1 = 0;
    // button for erase sd2 card
  //  _TRISD8 = 1;

	led_on();
    delay_ms(1000);
	led_off();

	// init hardware
	card1_init();
	card2_init();

	//Initialize Disk
	DSTATUS sts = disk_initialize(&sd1, 0);
	sts = disk_initialize(&sd2, 0);

	//Mount Filesystem
	f_mount(&sd1, 0, &Fatfs1);
    f_mount(&sd2, 0, &Fatfs2);

	FRESULT FOpenRes;
        {
            FATFS *fs;
            DWORD fre_clust, fre_sect, tot_sect;
            FOpenRes = f_getfree(&sd2, "0:", &fre_clust, &fs);
            if (FOpenRes != FR_OK) {
                //lcd_puts("unknow error");
                goto END;
            }
            /* Get total sectors and free sectors */
            tot_sect = (fs->n_fatent - 2) * fs->csize;
            fre_sect = fre_clust * fs->csize;

            if( (fre_sect*1.0)/tot_sect < 0.8) {
                //lcd_puts("sd2 full, format?");
                //fre_clust = 10000;
               // while(--fre_clust > 0) {
                //    if(_RD8 == 1)
                //        break;
               // }
               // if(fre_clust > 0) {
                    f_mkfs(&sd2, 0, 0, 512);
             //   }
            }
            f_mount(&sd2, 0, &Fatfs2);
        }
    
        FRESULT fr;     /* Return value */
        DIR dj;         /* Directory search object */
        FILINFO fno;    /* File information */

        fr = f_findfirst(&dj, &fno, "");  /* Start to search for photo files */

        while (fr == FR_OK && fno.fname[0]) {         /* Repeat while an item is found */
            if(fno.fattrib & AM_DIR) { // is directory ==> recursive
                recusive_copy(fno.fname);
                //continue;
            }
            /******* DO COPY HERE */
            docopy(fno.fname);
            /**** END DO COPY*/
            fr = f_findnext(&sd1, &dj, &fno);               /* Search for next item */
        }
        f_closedir(&sd1, &dj);
	//Read 1 byte from file
        //lcd_puts("copy done");
	
	f_mount(&sd1, 0, NULL);
	f_mount(&sd2, 0, NULL);
 END:
	while(1);
}