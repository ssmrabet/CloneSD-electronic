#include "header.h"

/* OSC - SYSCLK Configuration - 32 MHz */
#pragma config FPLLIDIV = DIV_2
#pragma config POSCMOD = XT, FNOSC = PRIPLL, FWDTEN = OFF
#pragma config FPLLMUL = MUL_16
#pragma config FPLLODIV = DIV_2

/* OSC - PBCLK Configuration - 16 MHz */
/* Default PBCLK = SYSCLK/8 */
#pragma config FPBDIV = DIV_2

unsigned char on = 0;

int     copy()
{
    int i = SECTOR_SIZE - 1, j = 0, k = 1, result;
    char readbuf[512], readbuf1[512];

    /* Set LED pin to output */
    _TRISF1 = 0;
    _RF1 = 0;

    /* Initialize buffers */
    while(i >= 0)
    {
        
        readbuf[i] = 0;
        readbuf1[i] = 0;
        i--;
    }
    i = 1;

    /* Start up delay */
    delay_seconds(1);
    /* Initialize SPI */
    SPI_Init();
    SPI_Init1();
    
    /* Initialize SD Card */
    result = SDCard_Init();
    if(result == ERROR_INIT || result == ERROR_RESET || result == ERROR_VLTG)
    {
        /* Flag error */
        // WriteString("SD 1 ERR");
         _RF1 = 1;
         return (-1);
    }
    result = SDCard_Init1();
    if(result == ERROR_INIT || result == ERROR_RESET || result == ERROR_VLTG)
    {
        /* Flag error */
      //  WriteString("SD 2 ERR");
        _RF1 = 1;
        return (-1);
    }

    //READ SD1(SPI1) WRITE SD2(SPI2)
    //4194304 for 1GB sdcard we have 2.048.000 sector.
    for (j=0; j < 245919; j++)
    {
    	led_tog();
        /* Read from SD 1 */
        if(SDCard_ReadSector1(j, readbuf) == ERROR_READ)
        {
            _RF1 = 1;
          //  WriteString("READ ERR");
            return (-1);
        }
    
        /* Write to SD Card 2 and verify*/
        if(SDCard_WriteSector(j, readbuf) == ERROR_WRITE)
        {
            _RF1 = 1;
            //WriteString("WRITE ERR");
            return (-1);
        }
        if (j == 1000 * i) {
            _RF1 = 1;
            delay_seconds(5);
            _RF1 = 0;
            i++;
        }
        if (j == (245919 / 100) * k)
        {
            k++;
            //WriteString((char)k+"%");
        }
        if (j == 245918) {
            _RF1 = 1;
            //WriteString("END");
        }
    }
}

int     main()
{
    int ret;

    ret = copy();
    if (ret == -1)
        goto wait;

    wait:
    while(1);
}

void delay_us()
{
    while (TMR2 < 29);
}

void delay_100us(u8 t)
{
    u16 x = 0;
    while(x++ < t)
        delay_us();
}
void delay_ms(u8 t)
{
    u16 x = 0;
    while(x++ < 10* t)
        delay_us();
}

/* Delay in seconds */
void delay_seconds(int count)
{
    long int load;
    while(count > 0)
    {
        load = ONE_SECOND;
        while(load > 0) load--;
        count--;
    }
}

void led_tog() {
	if(on) {
		_RF1 = 0;
		on = 0;
	} else {
		_RF1 = 1;
		on = 1;
	}
}