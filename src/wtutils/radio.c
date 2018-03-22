#include <radio.h>
#include <ioport.h>
//#include <conf_board.h>
#include <delay.h>
#include <usart_mega.h>
#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <time.h>

static int RADIO_AUX = 0;
static int RADIO_MD0 = 0;
static int RADIO_MD1 = 0;

void waitRadioInit(int blink_pin)
{
	do
	{
		if (blink_pin)
			ioport_set_pin_high(blink_pin);
			
		delay_ms(500);
		
		if (blink_pin)
			ioport_set_pin_low(blink_pin);
			
		delay_ms(500);
	} while (!ioport_get_pin_level(RADIO_AUX));
	
	if (blink_pin)
		ioport_set_pin_high(blink_pin);
		
	delay_ms(5);
}

void radioWake(int blink_pin)
{
	ioport_set_pin_level(RADIO_MD0, true); // wake-up mode, add preamble to wake remote part
	ioport_set_pin_level(RADIO_MD1, false);
	
	waitRadioInit(blink_pin);
}

void radioSleep(void)
{
	ioport_set_pin_level(RADIO_MD0, true);
	ioport_set_pin_level(RADIO_MD1, true);
}

static char usart_buffer[0x70];
static volatile int usart_buffer_pos = -1;
static time_t usart_last_recv_time = 0;

static void usartWaitRespLenMsec(int resp_len, unsigned long timeout_msec)
{
	unsigned long cycles = timeout_msec / 10;
	for (unsigned long i = 0; i < cycles && resp_len > usart_buffer_pos; ++i)
	{
		delay_ms(10);
	}
}

ISR(USART_RX_vect)
{
	uint8_t c = usart_getchar((USART_t*) &UCSR0A);
	if (usart_buffer_pos >= 0 && usart_buffer_pos < sizeof(usart_buffer) - 1)
	{
		usart_buffer[usart_buffer_pos++] = c;
		usart_buffer[usart_buffer_pos] = 0;
	}
		
	time(&usart_last_recv_time);
}

static void enableUsartInterrupt(void)
{
	usart_buffer_pos = 0;
	memset(usart_buffer, 0, sizeof(usart_buffer));
	
	if (usart_rx_is_complete((USART_t*) &UCSR0A))
		usart_getchar((USART_t*) &UCSR0A);
}

static void disableUsartInterrupt(void)
{
	usart_buffer_pos = -1;
}

int usartReqWaitAck(const char *req, const char *resp, int radio_status_pin)
{
	int resp_len = strlen(resp);
	if (resp_len >= sizeof(usart_buffer))
		return 0;
	
	enableUsartInterrupt();
	radioWake(radio_status_pin);
	
	for (const char *p = req; *p; ++p)
		usart_putchar((USART_t*) &UCSR0A, *p);
	
	usartWaitRespLenMsec(resp_len, 15000);
	
	radioSleep();
	disableUsartInterrupt();
	
	return strncmp(usart_buffer, resp, resp_len) == 0;
}

static void usartEnterPowerSavingMode(void)
{
	ioport_set_pin_level(RADIO_MD0, false); 
	ioport_set_pin_level(RADIO_MD1, true);
	
	waitRadioInit(0);	
}

static void usartEnterNormalMode(void)
{
	ioport_set_pin_level(RADIO_MD0, false); 
	ioport_set_pin_level(RADIO_MD1, false);
	
	waitRadioInit(0);
}

static void initRadioComm(void)
{
	static usart_rs232_options_t usart_options = {
		.baudrate = 9600,
		.charlength = USART_CHSIZE_8BIT_gc,
		.paritytype = USART_PMODE_DISABLED_gc,
		.stopbits = false
	};
	
	radioSleep();
	
	//sysclk_enable_peripheral_clock(&UCSR0A);
	
	usart_double_baud_disable((USART_t*) &UCSR0A);
	usart_init_rs232((USART_t*) &UCSR0A, &usart_options);
	//usart_set_mode((USART_t*) &UCSR0A, USART_CMODE_SYNCHRONOUS_gc);
	//usart_enable_tx(USART0);
	//usart_rx_disable(USART0);
	
	((USART_t*) &UCSR0A)->UCSRnB |= USART_RXCIE_bm;
}

void usartTransInit(int aux, int md0, int md1)
{
	RADIO_MD1 = md1;
	RADIO_MD0 = md0;
	RADIO_AUX = aux;
	
	initRadioComm();
}

void usartRecvInit(int aux, int md0, int md1)
{
	RADIO_MD1 = md1;
	RADIO_MD0 = md0;
	RADIO_AUX = aux;

	initRadioComm();
	enableUsartInterrupt();
}

void enableWakeByRadioInterrupt(int interrupt)
{
	usartEnterPowerSavingMode();

	cli();	
	PCMSK1 |= (1 << interrupt);
	PCICR |= 1 << PCIE1;
	sei();
}

int usartReply(const char *resp)
{
	usartEnterNormalMode();
	
	for (const char *p = resp; *p; ++p)
		usart_putchar((USART_t*) &UCSR0A, *p);
	
	delay_ms(5);
	usartEnterPowerSavingMode();
	
	return 1;
}

int usartIsRecvComplete(const char **requests, int *bytes_available, int *rtime)
{
	int ret = -1;
	
	for (int i = 0; *requests; ++requests, ++i)
	{
		if (!strcmp(usart_buffer, *requests))
		{
			ret = i;
			break;
		}
	}
	
	*bytes_available = usart_buffer_pos;
	*rtime = time(NULL) - usart_last_recv_time;
	
	if (ret >= 0
		|| time(NULL) - usart_last_recv_time > 1)
	{
		usart_buffer_pos = 0;
		usart_buffer[0] = 0;
	}
	
	return ret;
}

