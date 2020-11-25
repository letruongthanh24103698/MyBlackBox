#ifndef _SRC_WIRELESS_H_
#define _SRC_WIRELESS_H_

#include <Arduino.h>
#include "header.h"

#define _BLE_CONNECTION_STATUS_PIN_     _LED_RIGHT_PIN_
#define _WIFI_CONNECTION_STATUS_PIN_    _LED_LEFT_PIN_

void wireless_setup();

void wirelessWIFI_loop();
void wirelessWIFI_start();
void wirelessWIFI_stop();
bool wirelessWIFI_CheckTimeout();

void wirelessBLE_loop();
void wirelessBLE_start();
void wirelessBLE_stop();
bool wirelessBLE_CheckTimeout();
#endif