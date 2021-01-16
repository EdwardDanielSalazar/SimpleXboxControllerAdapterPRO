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

// OLED display address
#define I2C_ADDRESS 0x3C

// Voltage checking
#define VCC_READ_PIN A0
#define ONE_VOLT 43 // Based on a 390KOhm / 100KOhm voltage divider
#define POWER_THRESHOLD 270

// Build Options
#define ENABLE_POWER_CHECK
#define ENABLE_OLED
#define ENABLE_RUMBLE
#define ENABLE_MOTION

/* prototypes */
void sendControllerHIDReport();

#endif /* MAIN_H_ */
