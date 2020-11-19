#ifndef _SRC_WIRELESS_H_
#define _SRC_WIRELESS_H_

#include <Arduino.h>
#include "header.h"

#define _BLE_CONNECTION_STATUS_PIN_     _LED_LEFT_PIN_
#define _WIFI_CONNECTION_STATUS_PIN_    _LED_RIGHT_PIN_

void wireless_setup();
void wireless_loop();
void wireless_start();
void wireless_stop();
#endif