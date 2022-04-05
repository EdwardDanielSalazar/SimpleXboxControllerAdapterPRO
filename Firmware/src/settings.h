/*
 * main.h
 *
 * Created: 9/02/2019 4:15:11 PM
 *  Author: Ryan
 */

#ifndef MAIN_H_
#define MAIN_H_
#include <inttypes.h>

#define USB_HOST_RESET_PIN 10
#define ARDUINO_LED_PIN 17
#define I2C_ADDRESS 0x3C
#define VCC_READ_PIN A0

// Build Options
//#define ENABLE_OLED
#define ENABLE_RUMBLE
//#define ENABLE_XBOX360 //para xbox 360 con adaptador inalambrico
//#define ENABLE_XBOXBT
//#define ENABLE_PS4BT
//#define ENABLE_MOTION
//#define ENABLE_SWITCHBT
//#define ENABLE_XBOXONEUSB
//#define ENABLE_XBOXUSB //para xbox 360 cableado
#define ENABLE_XBOXBT
/* prototypes */
void sendControllerHIDReport();

#endif /* MAIN_H_ */
