#include "lcd.h"
#include "conf_board.h"
#include "ioport.h"
#include "delay.h"
#include "clock.h"
#include <string.h>
#include <stdio.h>

enum LcdDataBits
{
	LcdData_Db4 = 16,
	LcdData_Db5 = 32,
	LcdData_Db6 = 64,
	LcdData_Db7 = 128,
	
	LcdData_Db0 = 1,
	LcdData_Db1 = 2,
	LcdData_Db2 = 4,
	LcdData_Db3 = 8,
};

struct Message
{
	time_t t;
	const char *action;
	char line2[0x14];
};

static const size_t lcd_msg_count = 9;
static struct Message lcd_msg[9];

static char last_msg_l1[0x14] = { 0 };

void push_msg(const char *v, const char *action)
{
	memmove(&lcd_msg[1], &lcd_msg[0], sizeof(lcd_msg) - sizeof(struct Message));
	
	time(&lcd_msg[0].t);
	strncpy(lcd_msg[0].line2, v, sizeof(lcd_msg[0].line2) - 1);
	lcd_msg[0].action = action;
}

static void fmt_time(int num, char *to, time_t v)
{
	time_t now;
	time(&now);
	
	if (now - v < 100)
		sprintf(to, "%d: -%dñ, ", num, (int) (now - v));
	else if (now - v < 60 * 60)
		sprintf(to, "%d: -%dìèí, ", num, minutes(now - v));
	else 
		sprintf(to, "%d: -%d÷, ", num, hours(now - v));
}

int show_msg(int at)
{
	char line1[0x20];
	
	for (; at < lcd_msg_count && !lcd_msg[at].line2[0]; ++at)
		;
			
	if (at >= lcd_msg_count)
		at = 0;

	fmt_time(at + 1, line1, lcd_msg[at].t);		
	strcat(line1, lcd_msg[at].action);

	if (strcmp(last_msg_l1, line1))
	{
		lcd_str(0, line1);
		lcd_str(1, lcd_msg[at].line2);
	}
	
	return at;
}

static void lcd_half_command_ni(int bits, int rs_high)
{
	ioport_set_pin_low(LCD_RW);
	ioport_set_pin_level(LCD_RS, rs_high);
	
	ioport_set_pin_high(LCD_E);
	
	ioport_set_pin_level(LCD_DB4, bits & LcdData_Db0);
	ioport_set_pin_level(LCD_DB5, bits & LcdData_Db1);
	ioport_set_pin_level(LCD_DB6, bits & LcdData_Db2);
	ioport_set_pin_level(LCD_DB7, bits & LcdData_Db3);
	
	delay_us(1);
	ioport_set_pin_low(LCD_E);
	
	ioport_set_pin_low(LCD_RS);
	
	delay_us(50);
}

static void lcd_data_high_ni(void)
{
	ioport_set_pin_high(LCD_DB4);
	ioport_set_pin_high(LCD_DB5);
	ioport_set_pin_high(LCD_DB6);
	ioport_set_pin_high(LCD_DB7);
}

static void lcd_command_nobusy_ni(int bits)
{
	lcd_half_command_ni(bits >> 4, 0);
	lcd_half_command_ni(bits, 0);
	lcd_data_high_ni();
	
	delay_us(100);
}

static void wait_busy_clear_ni(void)
{
	int ready = 0;
	while (!ready)
	{
		static int pins[] = { LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7 };
		for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i)
		{
			ioport_set_pin_dir(pins[i], IOPORT_DIR_INPUT);
			ioport_set_pin_mode(pins[i], IOPORT_MODE_PULLUP);
		}
	
		ioport_set_pin_low(LCD_RS);
		ioport_set_pin_high(LCD_RW);
		
		ioport_set_pin_high(LCD_E);
		ready = !ioport_get_pin_level(LCD_DB7);
		ioport_set_pin_low(LCD_E);

		for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i)
		{
			ioport_set_pin_dir(pins[i], IOPORT_DIR_OUTPUT);
			ioport_set_pin_high(pins[i]);
		}
		
		ioport_set_pin_low(LCD_RS);
		ioport_set_pin_high(LCD_RW);
		
		ioport_set_pin_high(LCD_E);
		delay_us(1);
		ioport_set_pin_low(LCD_E);
	}
	
	ioport_set_pin_low(LCD_RW);
}

static void lcd_command(int bits)
{
	//wait_busy_clear();

	cli();	
	delay_ms(1);
	
	lcd_half_command_ni(bits >> 4, 0);
	lcd_half_command_ni(bits, 0);
	
	delay_us(50);
	lcd_data_high_ni();
	delay_us(50);
	sei();
}

static void lcd_data(int bits)
{
	cli();
	lcd_half_command_ni(bits >> 4, 1);
	lcd_half_command_ni(bits, 1);
	
	delay_us(40);
	sei();
}

static int lcd_address2bits(int line, int ofs)
{
	return (line ? 0x40 : 0x0) + ofs;
}

static char lcd_chargen[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x10, 0x21, 0xC8, 0x22, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x00, 0x5D, 0x5E, 0x5F,
0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x41, 0xA0, 0x42, 0xA1, 0xE0, 0x45, 0xA3, 0xA4, 0xA5, 0xA6, 0x4B, 0xA7, 0x4D, 0x48, 0x4F, 0xA8,
0x50, 0x43, 0x54, 0xA9, 0xAA, 0x58, 0xE1, 0xAB, 0xAC, 0xE2, 0xAD, 0xAE, 0x62, 0xAF, 0xB0, 0xB1,
0x61, 0xB2, 0xB3, 0xB4, 0xE3, 0x65, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0x6F, 0xBE,
0x70, 0x63, 0xBF, 0x79, 0xE4, 0x78, 0xE5, 0xC0, 0xC1, 0xE6, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7
};

static unsigned char lcd_getcharcode(char v)
{
	return lcd_chargen[(unsigned char) v];
}

void lcd_str(int line, const char *data)
{
	int i;
	int whitespace = 0x10;
	lcd_command(LcdData_Db7 | lcd_address2bits(line, 0));
	
	for (i = 0; *data; ++data, ++i)
		lcd_data(lcd_getcharcode(*data));
	
	for (; i < 16; ++i)
		lcd_data(whitespace);
}

static void lcd_clear(void)
{
	lcd_command(LcdData_Db0);
	delay_ms(2);
}

void lcd_reset(void)
{
	{
		cli();
		int pins[] = { LCD_RS, LCD_RW, LCD_E, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7 };
		for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i)
		{
			ioport_set_pin_dir(pins[i], IOPORT_DIR_OUTPUT);
			ioport_set_pin_low(pins[i]);
		}
	
		lcd_data_high_ni();
		sei();
	
		delay_ms(50);
	}

	for (int i = 0; i < 2; ++i)
	{
		cli();
		lcd_half_command_ni(LcdData_Db1 | LcdData_Db0, 0);
		lcd_data_high_ni();
		delay_us(40);
	
		lcd_command_nobusy_ni(LcdData_Db5 | LcdData_Db3); // 4bit (Db4=0) 2 lines (Db3=1) 5x8 font (Db2=0)
		lcd_command_nobusy_ni(LcdData_Db5 | LcdData_Db3); // repeat per doc
		sei();
	}

	lcd_command(LcdData_Db3 | LcdData_Db2); // Display on (Db2=1)
	lcd_clear();
	lcd_command(LcdData_Db2 | LcdData_Db1); // Entry mode set, move right, no shift	
}

void lcd_init(void)
{
	lcd_reset();
	memset(lcd_msg, 0, sizeof(lcd_msg));
}

