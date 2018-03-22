#ifndef USART_MOTOR_LCD_H_INCLUDED
#define USART_MOTOR_LCD_H_INCLUDED

void lcd_init(void);
void lcd_reset(void);

// push msg in form
// msg#: time offset <action>
// <v>
// to front on msg lifo list
void push_msg(const char *v, const char *action);

int show_msg(int at);

void lcd_str(int line, const char *data);

#endif