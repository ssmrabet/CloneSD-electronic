#include "header.h"

/* Initialize and turn SPI ON */
void SPI_Init()
{
   /* SPI2 pin config
    * RG6 = SCK2 | RG7 = MOSI | RG8= MISO | RG9 = SS/CS
    */
   _TRISG6 = 0; _TRISG8 = 0; _TRISG7 = 1;

   /* Clock = PBCLK/32 = 250kHz [ PBCLK/(2*(SP1BRG+1)) ] */
   //SPI2BRG = 15;
   SPI2BRG = 1;		// 4Mhz

   /* Master Mode, CKE = 1, SMP = 0, 8-BIT, SPI ON */
   SPI2CON = 0x8120;
}

/* Initialize the SD Card */
int SDCard_Init()
{
   int i, result, done = 0;

   /* SD_CARD_SELECT pin set to output */
   _TRISG9 = 0; //SS pin
   // CARD DETECT IS IN 1 (.h file)

   /* Disable SD Card */
   SDCard_Disable();

   /* Unlock */
   //SD_WRITE_PROTECT = 0; initialized in 0 (.h file)

   /* Minimum 74 clock cycle for boot up */
   for(i = 0; i < 10; i++)
   {
       SPI_Clock();
   }

   /* Start up delay */
   delay_seconds(5);

   /* Send RESET */
   result = SDCard_SendCommand(CMD0, 0x00000000, RESP_RA1, 0x95);
   if(result != 1) result = ERROR_RESET;

   /* Read output operation conditions registers (OCR) */
   result = SDCard_SendCommand(CMD8, 0x000001AA, RESP_RA7, 0x87);
   if(result != 1) result = ERROR_VLTG;

   /* Send INIT */
   for(i = 0; i < INIT_TIMER; i++)
   {
       /* CMD55 - prerequisite for ACMD41 */
       result = SDCard_SendCommand(CMD55, 0x00000000, RESP_RA1, 0xFF);
       /* Accepted ? */
       if(result != 1) break;

       /* Initiate initialization with ACMD41 */
       result = SDCard_SendCommand(ACMD41, 0x40000000, RESP_RA1, 0xFF);
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
unsigned char SPI_Write(unsigned char c)
{
   /* Load the TX register - MOSI */
   SPI2BUF = c;
   while(!SPI2STATbits.SPIRBF);
   /* RX register - MISO shifted in simultaneously */
   return SPI2BUF;
}


int SDCard_SendCommand(unsigned char command, unsigned int addr, int num_response, unsigned char crc)
{
   int i, result, interm;
   int j = 24;
   unsigned char addr8;
   /* Enable SD Card */
   SDCard_Enable();

   /* Command packet - 6 Bytes */
   /* 1 - Command */
   SPI_Write(command | 0x40);

   /* 2-5 - Address (32-Bit) */
   while(j >= 0)
   {
      /* Most significant byte first */
      addr8 = ((addr >> j) & 0xFF);
      SPI_Write(addr8);
      j = j - 8;
   }

   /* 6 - CRC Byte */
   SPI_Write(crc);

   /* 1-8 byte cycle wait to process the command */
   for(i = 0; i < 8; i++)
   {
       result = SPI_Read();
       if(result != 0xFF)
       {
           while(num_response > 1)
           {
               interm = SPI_Read();
               num_response--;
           }
           break;
       }
   }

   /* Disable SD Card */
   if((command != CMD17) && (command != CMD24))
   {
       SDCard_Disable();
   }

   return result;
}


/* Read one sector */
int SDCard_ReadSector(unsigned int addr, char* buffer)
{
    int i, result;
    /* Enable SD Card */
    SDCard_Enable();

    /* Send read command */
    result = SDCard_SendCommand(CMD17, (addr << 9), RESP_RA1, 0xFF);

    /* Command accepted ? */
    if(result == 0)
    {
        for(i = 0; i < READ_TIMER; i++)
        {
            result = SPI_Read();
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
                *buffer = SPI_Read();
                buffer++;
            }

            /* Two dummy reads for CRC */
            SPI_Read();
            SPI_Read();
        }
    }

    /* Flag error */
    if(result != 1) result = ERROR_READ;

    /* Disable SD Card */
    SDCard_Disable();

    return result;
}


/* Write one sector */
int SDCard_WriteSector(unsigned int addr, char* buffer)
{
    int i, result;
    /* Enable SD Card */
    SDCard_Enable();

    /* Send write command */
    result = SDCard_SendCommand(CMD24, (addr << 9), RESP_RA1, 0xFF);

    /* Command accepted ? */
    if(result == 0)
    {
        /* Indicate writing start */
        SPI_Write(START_TOKEN);

        /* Write one sector = 512 bytes */
        for(i = 0; i < SECTOR_SIZE; i++)
        {
            SPI_Write(*buffer);
            buffer++;
        }

        /* Two dummy writes for CRC */
        SPI_Write(0xFF);
        SPI_Write(0xFF);

        /* Check if write accepted */
        result = SPI_Read();

        /* Accepted ? */
        if((result & 0x0F) == ACCEPT_TOKEN)
        {
            for(i = 0; i < WRITE_TIMER; i++)
            {
                result = SPI_Read();
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
    SDCard_Disable();

    return result;
}
