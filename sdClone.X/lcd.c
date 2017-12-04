#include "lcd.h"

static void delay_us() {
    int cnt = 2;
    while (--cnt > 0);
}

static void delay_100us(int t)
{
    int x = 0;
    while(x++ < t)
        delay_us();
}
static void delay_ms(int _ms) {
	long ms = _ms * 80; // this is not exactly
	while(ms-- >=0);
}

static int LCD_busy() {
    LCD_data_dir = 0xFF; // set RD to input pins
     RS = 0; // command register
     RW = 1; // read
     E = 1;
     //delay_ms(5);
     while(LCD_D7)
     {
         E=0;
         //delay_ms(5);
         E=1;
         delay_ms(20);
     }
     E = 0;
     delay_ms(1);
      LCD_data_dir = 0x00; // set RD back to outputs
}

static void LCD_codeNoWait(int code) {
    LCD_en_dir = 0x00;
  RS   = (code & 0b1000000000) >> 9;
  RW   = (code & 0b0100000000) >> 8;
  DATA =  (code & 0b0011111111);
  E   = 1;
  delay_us(1);// enable pin HIGH-->LOW
  E   = 0;
  delay_us(1);
}

static void LCD_code(int code) {
  LCD_codeNoWait(code);
  LCD_busy();
}

static void LCD_displayChar(char inputChar) {
  LCD_code(0b1000000000 + inputChar);
}

/*________________________________________________________________________*/
static void out4bits(void) {
    LCD_data_dir = LCD_rs_dir = LCD_rw_dir = 0;
    LCD_en_dir = 0; // all LCD pins to output
    E = RS = RW  =0;
    DATA   = 0x00;// default values for LCD pins are 0

   delay_ms(150);
   LCD_codeNoWait(0x30);
   delay_ms(100);
    LCD_codeNoWait(0x0C);
   delay_ms(10);
    LCD_codeNoWait(0x01);
   delay_ms(10);
    LCD_codeNoWait(0x02);
   delay_ms(10);
}


static void  LCD_setCursor(int column, int row) {
  if(row == 1) LCD_code(0b0000000000 + (column-1));
  if(row == 2) LCD_code(0b0000000000 + 0x40 + (column-1));
}

void lcd_clear() {
    out4bits();
}

void lcd_puts(char* str) {
    char* s=str;
    //lcd_clear();
    while(*s) {
        LCD_displayChar(*s);
        s++;
    }
}

#ifdef TEST
int main() {
    T2CON = 0x0; // Stops the Timer1 and resets the control register
    TMR2 = 0x0; // Clear timer register
    T2CON = 0x0000; // Set prescaler 1:16, internal clock, 16bits
    PR2 = 0xFFFF; // Load period register
    T2CONSET = 0x8000; // Start timer

    out4bits();

    LCD_displayChar('C');
    LCD_displayChar('O');
    LCD_displayChar('P');
    LCD_displayChar('Y');
    LCD_displayChar('A');
    LCD_displayChar('G');
    LCD_displayChar('C');
    LCD_displayChar('D');
   // LCD_setCursor(46,1);
    LCD_displayChar('M');
    LCD_displayChar('F');
    LCD_displayChar('G');

    while(1);
    return (EXIT_SUCCESS);
}
#endif