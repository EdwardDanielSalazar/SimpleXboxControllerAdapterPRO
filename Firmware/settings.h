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
#define PLAYER_ID1_PIN 19
#define PLAYER_ID2_PIN 20

//Settings - Note the Atmega 32U4 has only 32KB of flash!
//The Arduino bootloader takes up about 15% of this.
//if the Program Memory Usage is >85% or so it may fail
//programming the device.

#define MASTER
#define SUPPORTWIREDXBOXONE
#define SUPPORTWIREDXBOX360

/* prototypes */
void sendControllerHIDReport();

#endif /* MAIN_H_ */
