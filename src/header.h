#ifndef _SRC_HEADER_H_
#define _SRC_HEADER_H_

#include <Arduino.h>

#define GPIOInit(_PIN_,_MODE_)              pinMode(_PIN_,_MODE_)
#define GPIOWrite(_PIN_,_STATE_)            digitalWrite(_PIN_,_STATE_)
#define GPIORead(_PIN_)                     digitalRead(_PIN_)
#define GPIOToggle(_PIN_)                   GPIOWrite(_PIN_,1-GPIORead(_PIN_))
#define GPIOAttachINT(_PIN_,_FUNC_,_MODE_)  attachInterrupt(_PIN_,_FUNC_,_MODE_)

#define _GPS_EN_PIN_              4 //GPIO4
#define _SD_CS_PIN_               5 //GPIO5
#define _GPS_PPS_PIN              13 //GPIO13
#define _ONBOARD_LED_PIN_         2
#define _BUTTON_LEFT_PIN_         35
#define _BUTTON_RIGHT_PIN_        34
#define _LED_LEFT_PIN_            33
#define _LED_RIGHT_PIN_           32

#endif