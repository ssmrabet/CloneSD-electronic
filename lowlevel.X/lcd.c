#include "header.h"

/********************************* PRAGMA ***************************************************************/

// DEVCFG3
// USERID = No Setting

// DEVCFG2
#pragma config FPLLIDIV = DIV_4         // PLL Input Divider (4x Divider)
#pragma config FPLLMUL = MUL_16         // PLL Multiplier (16x Multiplier)

// DEVCFG1
#pragma config FNOSC = FRC              // Oscillator Selection Bits (Fast RC Osc (FRC))
#pragma config FSOSCEN = ON             // Secondary Oscillator Enable (Enabled)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = OFF            // Primary Oscillator Configuration (Primary osc disabled)
#pragma config OSCIOFNC = ON            // CLKO Output Signal Active on the OSCO Pin (Enabled)
#pragma config FPBDIV = DIV_8           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/8)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config FWDTEN = ON              // Watchdog Timer Enable (WDT Enabled)

// DEVCFG0
#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is disabled)
#pragma config ICESEL = ICS_PGx2        // ICE/ICD Comm Channel Select (ICE EMUC2/EMUD2 pins shared with PGC2/PGD2)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

/***********************************************************************************************************/

int     LCD_busy()
{
    LCD_data_dir = 0xFF; // set RD to input pins
    RS = 0; // command register
    RW = 1; // read
    E = 1;
    while(LCD_D7)
    {
        E=0;
        E=1;
        delay_ms(20);
    }
    E = 0;
    delay_ms(1);
    LCD_data_dir = 0x00; // set RD back to outputs
}

void    LCD_codeNoWait(int code)
{
    LCD_en_dir = 0x00;
    RS   = (code & 0b1000000000) >> 9;
    RW   = (code & 0b0100000000) >> 8;
    DATA =  (code & 0b0011111111);
    E   = 1;
    E   = 0;
}

void    LCD_code(int code)
{
    LCD_codeNoWait(code);
    LCD_busy();
}

void    LCD_displayChar(char inputChar)
{
    LCD_code(0b1000000000 + inputChar);
}

void    out4bits(void)
{
    LCD_data_dir = LCD_rs_dir = LCD_rw_dir = 0;
    LCD_en_dir = 0x00; // all LCD pins to output
    E = RS = RW  = 0;
    DATA = 0x00;// default values for LCD pins are 0

    delay_ms(150);
    LCD_codeNoWait(0x38);
    LCD_codeNoWait(0x38);
    LCD_codeNoWait(0x38);
    LCD_codeNoWait(0x38);

    delay_ms(20);
    LCD_code(0x38);
    delay_ms(50);
    delay_ms(10);
    LCD_codeNoWait(0x01);
    delay_ms(20);
    LCD_codeNoWait(0x06);
    delay_ms(20);
    LCD_codeNoWait(0x0F);
    delay_ms(20);
    LCD_codeNoWait(0x02);
    delay_ms(20);
}

void    LCD_setCursor(int column, int row) {
    if(row == 1) LCD_code(0b0000000000 + (column-1));
    if(row == 2) LCD_code(0b0000000000 + 0x40 + (column-1));
}

void    WriteString(char *str)
{
    if (*str)
        while (++*str)
            LCD_displayChar(*str);
}
