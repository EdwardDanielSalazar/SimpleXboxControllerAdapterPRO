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


#define MASTER
#define SUPPORTWIREDXBOXONE
#define SUPPORTWIREDXBOX360

#define ENABLE_OLED
#define I2C_ADDRESS 0x3C

/* prototypes */
void sendControllerHIDReport();

#endif /* MAIN_H_ */
