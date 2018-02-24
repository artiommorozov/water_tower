#ifndef DS18B20_H_INCLUDED
#define DS18B20_H_INCLUDED

void ds18b20_init(int pin);
void ds18b20_scan(unsigned char **roms, int *inout_num_rom);
int ds18b20_read_temp(unsigned char *rom);

#endif
