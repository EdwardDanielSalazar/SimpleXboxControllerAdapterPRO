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
#define I2C_ADDRESS 0x3C
#define VCC_READ_PIN A0

// Build Options
#define ENABLE_OLED
// #define ENABLE_RUMBLE
#define ENABLE_MOTION

/* prototypes */
void sendControllerHIDReport();

#endif /* MAIN_H_ */
