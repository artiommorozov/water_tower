#ifndef RADIO_H_INCLUDED
#define RADIO_H_INCLUDED

// transmitter
void usartTransInit(int aux, int md0, int md1);
void waitRadioInit(int blink_pin);
void radioWake(int blink_pin);
void radioSleep(void);
int usartReqWaitAck(const char *req, const char *resp, int radio_status_pin);

// receiver
void usartRecvInit(int aux, int md0, int md1);
// index of response or -1
int usartIsRecvComplete(const char **requests, int *bytes_available, int *rtime);
int usartReply(const char *resp);
void enableWakeByRadioInterrupt(int interrupt);

#endif