/* 
 * File:   lcd.h
 * Author: bocal
 *
 * Created on July 9, 2017, 6:25 PM
 */

#ifndef LCD_H
#define	LCD_H

#ifdef	__cplusplus
extern "C" {
#endif
#include <p32xxxx.h>

typedef unsigned char   u8;
typedef unsigned short    u16;
typedef unsigned long   u32;

typedef char   s8;
typedef short    s16;
typedef long   s32;

//   pins

#define	DATA	LATD
#define	RS	    PORTEbits.RE0
#define RW      PORTEbits.RE1
#define	E	    PORTEbits.RE2
#define LCD_D7  PORTDbits.RD7


// direction
#define LCD_data_dir TRISD
#define LCD_busy_dir TRISDbits.TRISD7
#define LCD_rs_dir   TRISEbits.TRISE0
#define LCD_rw_dir   TRISEbits.TRISE1
#define LCD_en_dir   TRISEbits.TRISE2

void lcd_clear(void);

void lcd_puts(char* str);

#ifdef	__cplusplus
}
#endif

#endif	/* LCD_H */

