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

// VCC Read Resistor Values in KOhm
// #define LARGE_RESISTOR 390
// #define SMALL_RESISTOR 100
// #define RAW_VOLTAGE_DIVISOR 43
// #define HIGH_POWER_THRESHOLD 240

// Build Options
#define ENABLE_OLED
// #define ENABLE_RUMBLE
#define ENABLE_MOTION

/* prototypes */
void sendControllerHIDReport();

#endif /* MAIN_H_ */
