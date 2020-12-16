/*
 * main.h
 *
 * Created: 9/02/2019 4:15:11 PM
 *  Author: Ryan
 */

#ifndef MAIN_H_
#define MAIN_H_
#include <inttypes.h>

#define USB_HOST_RESET_PIN 9
#define ARDUINO_LED_PIN 17

#define ENABLE_OLED
#define I2C_ADDRESS 0x3C

#define ENABLE_RUMBLE

/* prototypes */
void sendControllerHIDReport();

#endif /* MAIN_H_ */
