Radio control for village water pump on atmega328 and e32ttl100 radio usart modules

1) Why

A small village has water tower and a pump 1.5km away. Using cables to connect tower level sensor to pump is not possible. Option used before radio was an oldman switching pump on and off manually few times a day.

2) What's inside

Two components: tower and motor (pump). Source code for microcontrollers in atmel studio 7 and some schematics in kicad. No printed boards. This contents is mainly intended to allow other people repair existing devices.

3) Tower side

Runs on Atmega328 MCU with 1.84MHz quartz to make usart communication reliable. Uses china clone of E32TTL100 radio module with usart interface. As there's no power supply at tower side MCU is powered from solar panels and 12v lead-acid accumulator, using simple PWM solar controller from store. Voltage conversion uses L4940V5 low-dropout regulator. 

Water level sensor connects ground to MCU pins and so wakes it via pin change interrupt. MCU will then request on/off from pump side via radio (sends req and waits for ack a few times) and then will go to sleep to save battery. Radio uses wake-up mode assuming motor part radio is asleep.

4) Pump side

Runs on Atmega328 with 1.84Mhz quartz. Uses same E32TTL100 for radio, DS18B20 for air temp sense, ST7066U-based LCD and K293 solid-state 220V relay that's further controls powerful relay for switching pump. Schematics doesn't show timer relay (protects from MCU faults) and soft start module used to protect pump.

MCU monitors radio, temperature and time. Pump is not allowed to run for long periods, to stay off for long periods (those items assume tower side is having problems but water must flow anyway); in winter pump should run for short periods spilling water to drainage to protect tower from forming ice.

MCU deploys watchdog to reset itself. MCU can't run for over 100 years because time counter doesn't account for overflow, still there's a power failure every year so that's not an issue.

LCD shows device operation history, activated with log button push it cycles through log items back. Otherwise just shows last action taken. Format is 

Msg#: <time passed since event> <action taken>
<text>

5) Building

Pump side needs 1wire library, it uses atmel AVR318 sample with some changes 
Download from http://www.microchip.com/wwwAppNotes/AppNotes.aspx?appnote=en591191

Extract following to src/pump/src
// common_files
// OWIBitFunctions.h
// OWIHighLevelFunctions.c
// OWIHighLevelFunctions.h
// OWIPolled.h
// OWISWBitFunctions.c

And apply avr318.patch there
