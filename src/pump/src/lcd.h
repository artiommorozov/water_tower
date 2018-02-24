#ifndef USART_MOTOR_LCD_H_INCLUDED
#define USART_MOTOR_LCD_H_INCLUDED

void lcd_init(void);

// push msg in form
// msg#: time offset <action>
// <v>
// to front on msg lifo list
void push_msg(const char *v, const char *action);

int show_msg(int at);

void lcd_str(int line, const char *data);

//////////////////////////////////////////////////////////////////////////

void lcd_half_command(int bits, int rs_high);
void lcd_command(int bits);
void lcd_data(int bits);
int lcd_address2bits(int line, int ofs);
unsigned char lcd_getcharcode(char v);

void lcd_clear(void);

#endif