#ifndef LCD_ENDPOINT_H
#define LCD_ENDPOINT_H

#include <WebServer.h>

// Register LCD/provision UI endpoints on the provided WebServer instance.
void lcd_register(WebServer &server);

#endif // LCD_ENDPOINT_H