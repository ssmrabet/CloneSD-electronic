#ifndef _HEADER_H8
#define	_HEADER_H_

#ifdef	__cplusplus
extern "C" {
#endif

#include <xc.h>
#include <p32xxxx.h>

    typedef unsigned char   u8;
    typedef unsigned short    u16;
    typedef unsigned long   u32;

    typedef char   s8;
    typedef short    s16;
    typedef long   s32;
    
/*
 * WD - j'ai pas ça
 * CD - j'ai pas ça
 * CS -  RG9 / RB2
 * SCK - RG6 / RF6
 * SDI - RG7 / RF2
 * SDO - RG8 / RF3
 */

/* DELAY */
void    delay_us();
void    delay_ms(u8 t);
void    delay_100us(u8 t);
void    delay_seconds(int count);
void    led_tog();

int     copy();

/************* SD2 **************/
int     SDCard_Init(void);
void    SPI_Init(void);
unsigned char SPI_Write(unsigned char c);
int     SDCard_SendCommand(unsigned char command, unsigned int addr, int num_response, unsigned char crc);
int     SDCard_WriteSector(unsigned int addr, char* buffer);
int     SDCard_ReadSector(unsigned int addr, char* buffer);

/************* SD1 **************/
int     SDCard_Init1(void);
void    SPI_Init1(void);
unsigned char SPI_Write1(unsigned char c);
int     SDCard_SendCommand1(unsigned char command, unsigned int addr, int num_response, unsigned char crc);
int     SDCard_WriteSector1(unsigned int addr, char* buffer);
int     SDCard_ReadSector1(unsigned int addr, char* buffer);

/************* LCD **************/
int     LCD_busy();
void    LCD_codeNoWait(int code);
void    LCD_code(int code);
void    LCD_displayChar(char inputChar);
void    out4bits(void);
void    LCD_setCursor(int column, int row);
void    WriteString(char *str);

/* SD Pin connections */
#define SD_WRITE_PROTECT 0
#define SD_CARD_DETECT   1
#define SD_CARD_SELECT   _RG9

#define SD_WRITE_PROTECT1 0
#define SD_CARD_DETECT1   1
#define SD_CARD_SELECT1   _RB2

#define ERASE_BLOCK_START_ADDR   32
#define ERASE_BLOCK_END_ADDR     33
#define ERASE_SELECTED_BLOCKS    38

/* Easy macros */
#define SDCard_Disable() SD_CARD_SELECT = 1; SPI_Clock()
#define SDCard_Enable()  SD_CARD_SELECT = 0

#define SDCard_Disable1() SD_CARD_SELECT1 = 1; SPI_Clock1()
#define SDCard_Enable1()  SD_CARD_SELECT1 = 0

/* Read from SPI device by shifting out dummy 0xFF */
#define SPI_Read()  SPI_Write(0xFF)
#define SPI_Read1()  SPI_Write1(0xFF)

/* Clock SPI device by shifting out dummy 0xFF */
#define SPI_Clock() SPI_Write(0xFF)
#define SPI_Clock1() SPI_Write1(0xFF)

/* SD Card commands */
/* RESET = CMD0, INIT = CMD1 */
#define CMD0   0
#define CMD1   1
#define CMD8   8
#define CMD17  17
#define CMD24  24
#define CMD55  55
#define ACMD41 41

/* Initialize response time - SDCard size dependent */
#define INIT_TIMER  10000
#define READ_TIMER  10000
#define WRITE_TIMER 10000

/* Error codes */
#define ERROR_RESET 101
#define ERROR_INIT  102
#define ERROR_VLTG  103
#define ERROR_READ  104
#define ERROR_WRITE 105

/* More defines */
#define START_TOKEN  0xFE
#define ACCEPT_TOKEN 0x05
#define SECTOR_SIZE  512

/* Response types */
#define RESP_RA1 1
#define RESP_RA7 5

/* Delay */
#define ONE_SECOND 320000

/* LCD PIN CONNECTION */
#define	DATA	PORTD
#define	RS	PORTEbits.RE0
#define RW      PORTEbits.RE1
#define	E	PORTEbits.RE2
#define LCD_D7  PORTDbits.RD7

// direction
#define LCD_data_dir TRISD
#define LCD_busy_dir TRISDbits.TRISD7
#define LCD_rs_dir   TRISEbits.TRISE0
#define LCD_rw_dir   TRISEbits.TRISE1
#define LCD_en_dir   TRISEbits.TRISE2

#ifdef	__cplusplus
}
#endif

#endif	/* _HEADER_H_ */