#include "header.h"

/* Initialize and turn SPI 1 ON */
void SPI_Init1()
{
   /* SPI2 pin config
    * RF6 = SCK2 | RF3 = MOSI | RF2= MISO | RB2 = SS/CS
    */
    _TRISF6 = 0; _TRISF2 = 0; _TRISF3 = 1;

   /* Clock = PBCLK/32 = 250kHz [ PBCLK/(2*(SP1BRG+1)) ] */
   //SPI1BRG = 15;
   SPI1BRG = 1;  // 4Mhz

   /* Master Mode, CKE = 1, SMP = 0, 8-BIT, SPI ON */
   SPI1CON = 0x8120;
}

/* Initialize the SD Card */
int SDCard_Init1()
{
   int i, result, done = 0;

   /* SD_CARD_SELECT pin set to output */
   _TRISB2 = 0; //SS pin
   // CARD DETECT IS IN 1 (.h file)

   /* Disable SD Card */
   SDCard_Disable1();

   /* Unlock */
   //SD_WRITE_PROTECT = 0; initialized in 0 (.h file)

   /* Minimum 74 clock cycle for boot up */
   for(i = 0; i < 10; i++)
   {
       SPI_Clock1();
   }

   /* Start up delay */
   delay_seconds(5);

   /* Send RESET */
   result = SDCard_SendCommand1(CMD0, 0x00000000, RESP_RA1, 0x95);
   if(result != 1) result = ERROR_RESET;

   /* Read output operation conditions registers (OCR) */
   result = SDCard_SendCommand1(CMD8, 0x000001AA, RESP_RA7, 0x87);
   if(result != 1) result = ERROR_VLTG;

   /* Send INIT */
   for(i = 0; i < INIT_TIMER; i++)
   {
       /* CMD55 - prerequisite for ACMD41 */
       result = SDCard_SendCommand1(CMD55, 0x00000000, RESP_RA1, 0xFF);
       /* Accepted ? */
       if(result != 1) break;

       /* Initiate initialization with ACMD41 */
       result = SDCard_SendCommand1(ACMD41, 0x40000000, RESP_RA1, 0xFF);
       /* Exited IDLE ? */
       if(result == 0)
       {
           done = 1;
           break;
       }
   }

   if(done != 1) result = ERROR_INIT;

   return result;
}


/* Write and read from SPI */
unsigned char SPI_Write1(unsigned char c)
{
   /* Load the TX register - MOSI */
   SPI1BUF = c;
   while(!SPI1STATbits.SPIRBF);
   /* RX register - MISO shifted in simultaneously */
   return SPI1BUF;
}

int SDCard_SendCommand1(unsigned char command, unsigned int addr, int num_response, unsigned char crc)
{
   int i, result, interm;
   int j = 24;
   unsigned char addr8;
   /* Enable SD Card */
   SDCard_Enable1();

   /* Command packet - 6 Bytes */
   /* 1 - Command */
   SPI_Write1(command | 0x40);

   /* 2-5 - Address (32-Bit) */
   while(j >= 0)
   {
      /* Most significant byte first */
      addr8 = ((addr >> j) & 0xFF);
      SPI_Write1(addr8);
      j = j - 8;
   }

   /* 6 - CRC Byte */
   SPI_Write1(crc);

   /* 1-8 byte cycle wait to process the command */
   for(i = 0; i < 8; i++)
   {
       result = SPI_Read1();
       if(result != 0xFF)
       {
           while(num_response > 1)
           {
               interm = SPI_Read1();
               num_response--;
           }
           break;
       }
   }

   /* Disable SD Card */
   if((command != CMD17) && (command != CMD24))
   {
       SDCard_Disable1();
   }

   return result;
}


/* Read one sector */
int SDCard_ReadSector1(unsigned int addr, char* buffer)
{
    int i, result;
    /* Enable SD Card */
    SDCard_Enable1();

    /* Send read command */
    result = SDCard_SendCommand1(CMD17, (addr << 9), RESP_RA1, 0xFF);

    /* Command accepted ? */
    if(result == 0)
    {
        for(i = 0; i < READ_TIMER; i++)
        {
            result = SPI_Read1();
            /* Wait for SD Card ready-to-send */
            if(result == START_TOKEN)
            {
                result = 1;
                break;
            }
        }

        if(result == 1)
        {
            /* Read one sector = 512 bytes */
            for(i = 0; i < SECTOR_SIZE; i++)
            {
                *buffer = SPI_Read1();
                buffer++;
            }

            /* Two dummy reads for CRC */
            SPI_Read1();
            SPI_Read1();
        }
    }

    /* Flag error */
    if(result != 1) result = ERROR_READ;

    /* Disable SD Card */
    SDCard_Disable1();

    return result;
}


/* Write one sector */
int SDCard_WriteSector1(unsigned int addr, char* buffer)
{
    int i, result;
    /* Enable SD Card */
    SDCard_Enable1();

    /* Send write command */
    result = SDCard_SendCommand1(CMD24, (addr << 9), RESP_RA1, 0xFF);

    /* Command accepted ? */
    if(result == 0)
    {
        /* Indicate writing start */
        SPI_Write1(START_TOKEN);

        /* Write one sector = 512 bytes */
        for(i = 0; i < SECTOR_SIZE; i++)
        {
            SPI_Write1(*buffer);
            buffer++;
        }

        /* Two dummy writes for CRC */
        SPI_Write1(0xFF);
        SPI_Write1(0xFF);

        /* Check if write accepted */
        result = SPI_Read1();

        /* Accepted ? */
        if((result & 0x0F) == ACCEPT_TOKEN)
        {
            for(i = 0; i < WRITE_TIMER; i++)
            {
                result = SPI_Read1();
                /* Write done! */
                if(result != 0)
                {
                    result = 1;
                    break;
                }
            }
        }
    }

    /* Flag error */
    if(result != 1) result = ERROR_WRITE;

    /* Disable SD Card */
    SDCard_Disable1();

    return result;
}

