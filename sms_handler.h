#ifndef SMS_HANDLER_H
#define SMS_HANDLER_H

#include <Arduino.h>

void sms_init();
void sms_loop();
void sms_scan_now();
bool sms_send(String number, String message);

#endif // SMS_HANDLER_H