#include <asf.h>
#include <clock.h>
#include <ds18b20.h>
#include <string.h>
#include <lcd.h>
#include <stdio.h>

// using atmel AVR318 sample with some changes 
// download from http://www.microchip.com/wwwAppNotes/AppNotes.aspx?appnote=en591191
// extract following to src
// common_files
// OWIBitFunctions.h
// OWIHighLevelFunctions.c
// OWIHighLevelFunctions.h
// OWIPolled.h
// OWISWBitFunctions.c
// then apply patches from this source tree

#include "OWIPolled.h"
#include "OWIHighLevelFunctions.h"
#include "OWIBitFunctions.h"

static int pin = 0;

void ds18b20_init(int a_pin)
{
	pin = a_pin;
	
	ioport_set_pin_dir(pin, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(pin, IOPORT_MODE_PULLUP);	

#if 0	
	OWI_PULL_BUS_LOW(a);
	
	delay_ms(1);
	
	OWI_RELEASE_BUS(a);
	
	delay_ms(1);
	
	OWI_SAMPLE_BUS(pin);
	
	delay_ms(1);
	
	OWI_SAMPLE_BUS(pin);
#else
	delay_ms(1);
#endif
}

static const size_t rom_size = 8;

void ds18b20_scan(unsigned char **roms, int *inout_num_rom)
{
	char buf[0x20];
	int numDevices = 0, pres = 0;
	unsigned char *newID = roms[0];
	uint8_t pres2 = 0;
	
	cli();
	
	pres = OWI_DetectPresence(OWI_PORT_PINMASK);
	
	int lastDeviation = 0;
	// Do slave search on each bus, and place identifiers and corresponding
	// bus "addresses" in the array.
	while (numDevices < *inout_num_rom)
	{
		if (numDevices)
			memcpy(newID, roms[numDevices - 1], rom_size);
			
		if ((lastDeviation = OWI_SearchRom(newID, lastDeviation, 0))
			== OWI_ROM_SEARCH_FINISHED)
		{
			++numDevices;
			break;	
		}
				
		newID = roms[++numDevices];
	}  

	sei();	

	*inout_num_rom = numDevices;
	//sprintf(buf, "p=%d n=%d 1=%d 0=%d", pres, numDevices, ones, zeroes);
	//lcd_str(0, buf);
	//delay_s(6);
}

#define DS1820_START_CONVERSION         0x44
#define DS1820_READ_SCRATCHPAD          0xbe

int ds18b20_read_temp(unsigned char *rom)
{
	int16_t ret = 99;

	cli();
	
	if (OWI_DetectPresence(OWI_PORT_PINMASK))
	{
		OWI_MatchRom(rom, 0);
		OWI_SendByte(DS1820_START_CONVERSION, 0);
	
		delay_s(1);

		if (OWI_DetectPresence(OWI_PORT_PINMASK))
		{
			uint8_t low = 0, high = 0;
			OWI_MatchRom(rom, 0);
			OWI_SendByte(DS1820_READ_SCRATCHPAD, 0);
			low = OWI_ReceiveByte(0);
			high = OWI_ReceiveByte(0);
			
			ret = (low >> 4) + ((high & 7) << 4);
			
			if (high & ~7)
				ret = -ret;
		}
	}
	
	sei();
	
	return ret;
}
