/*
This sketch is used for the ogx360 PCB (See https://github.com/Ryzee119/ogx360).
This program incorporates the USB Host Shield Library for the MAX3421 IC See https://github.com/felis/USB_Host_Shield_2.0.
The USB Host Shield Library is an Arduino library, consequently I have imported to necessary Arduino libs into this project.
This program also incorporates the AVR LUFA USB Library, See http://www.fourwalledcubicle.com/LUFA.php for the USB HID support.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.

In settings.h you can configure the following options:
1. Compile for MASTER of SLAVE (comment out #define MASTER) (Host is default)
2. Enable or disable Steel Battalion Controller Support via Wireless Xbox 360 Chatpad (Enabled by Default)
3. Enable or disable Xbox 360 Wired Support (Enabled by default)
4. Enable or disable Xbox One Wired Support (Disabled by default)
*/

#include "settings.h"
#include "xiddevice.h"
// #include "EEPROM.h" // ?? Remove this ??
#include <SPI.h>
#include <XBOXONE.h>
#include <XBOXUSB.h>
#include <PS3USB.h>
#include <PS4USB.h>

#ifdef ENABLE_OLED
#include <SSD1306Ascii.h>
#include <SSD1306AsciiAvrI2c.h>
#endif

//playerID is set in the main program based on the slot the Arduino is installed.
// uint8_t playerID;
//Xbox gamepad data structure to store all button and actuator states for all four controllers.
USB_XboxGamepad_Data_t XboxOGDuke;
//Default XID device to emulate
uint8_t ConnectedXID = DUKE_CONTROLLER;
//Flag is set when the device has been successfully setup by the OG Xbox
bool enumerationComplete = false;
//Timer used to time disconnection between SB and Duke controller swapover
uint32_t disconnectTimer = 0;

USB UsbHost;
uint8_t getButtonPress(ButtonEnum b);
int16_t getAnalogHat(AnalogHatEnum a);
void setRumbleOn(uint8_t lValue, uint8_t rValue);
void setLedOn(LEDEnum led); // TO DO - do something with this
uint8_t controllerConnected();
void checkControllerChange();

void getStatus();

XBOXONE XboxOneWired(&UsbHost);
XBOXUSB Xbox360Wired(&UsbHost);
PS3USB PS3Wired(&UsbHost); //defines EP_MAXPKTSIZE = 64. The change causes a compiler warning but doesn't seem to affect operation
PS4USB PS4Wired(&UsbHost);
uint8_t controllerType = 0;
uint8_t status = 0;

#ifdef ENABLE_RUMBLE
bool rumbleOn = false;
#endif

#ifdef ENABLE_MOTION
bool motionOn = false;
void getMotion();
void limitInputAngles();
// void startMotion();
// void startMotion();

uint16_t rollAngle = 0; // PS controllers provide 0 - 360
uint16_t pitchAngle = 0;
int16_t relativeRollAngle = 0; // Will be between [-45 and 45]
int16_t relativePitchAngle = 0;
// int16_t newMotionX = 180;
// int16_t newMotionY = 180;
// int16_t lastMotionX = 0;
// int16_t lastMotionY = 0;
// int16_t motionXMod = 180;
// int16_t motionYMod = 180;
float lookXAdjust_f = 0;
float lookYAdjust_f = 0;
// int16_t motionAdjustX = 0;
// int16_t motionAdjustY = 0;
// unsigned long motionStartMillis = 0;
// unsigned long motionCurrentMillis = 0;
// const uint8_t motionPeriod = 1;

// Use variables for the inital controller angle so these can be 
// calibrated by the user in future iterations
uint16_t initRollAngle = 180;
uint16_t initPitchAngle = 180;

// Set the max controller angles to be accepted as input
// Set separately as we may want different values for pitch and roll
uint16_t maxRollInputAngle = initRollAngle + MAX_INPUT_ANGLE; // 180 + 45 = 225
uint16_t minRollInputAngle = initRollAngle - MAX_INPUT_ANGLE; // 180 - 45 = 135
uint16_t maxPitchInputAngle = initPitchAngle + MAX_INPUT_ANGLE; // 180 + 45 = 225
uint16_t minPitchInputAngle = initPitchAngle - MAX_INPUT_ANGLE; // 180 - 45 = 135

#endif

#ifdef ENABLE_OLED
void updateOled();
SSD1306AsciiAvrI2c oled;
#endif

int main(void)
{
    //Init the Arduino Library
    init();

    //Init IO
    pinMode(USB_HOST_RESET_PIN, OUTPUT);
    pinMode(ARDUINO_LED_PIN, OUTPUT);
    digitalWrite(USB_HOST_RESET_PIN, LOW);
    digitalWrite(ARDUINO_LED_PIN, HIGH);

    //Init the LUFA USB Device Library
    SetupHardware();
    GlobalInterruptEnable();

    // Initialise the Serial Port
    Serial1.begin(500000);

    //Init the XboxOG data arrays to zero.
    memset(&XboxOGDuke, 0x00, sizeof(USB_XboxGamepad_Data_t));

    digitalWrite(USB_HOST_RESET_PIN, LOW);
    delay(20); //wait 20ms to reset the IC. Reseting at startup improves reliability in my experience.
    digitalWrite(USB_HOST_RESET_PIN, HIGH);
    delay(20); //Settle
    while (UsbHost.Init() == -1)
    {
        digitalWrite(ARDUINO_LED_PIN, !digitalRead(ARDUINO_LED_PIN));
        delay(500);
    }

    // Record init supply voltage
    // recordVoltage();

    // Setup OLED
    #ifdef ENABLE_OLED
    oled.begin(&Adafruit128x32, I2C_ADDRESS);
    oled.setFont(SystemFont5x7);
    oled.displayRemap(true);
    // oled.clear();
    updateOled();
    #endif

    #ifdef ENABLE_MOTION

    #endif

    while (1)
    {
        // checkVoltage();
        UsbHost.busprobe();
        UsbHost.Task();

        checkControllerChange();
        if (controllerType)
        {
        
            //Read Digital Buttons
            XboxOGDuke.dButtons=0x0000;
            if (getButtonPress(UP))      XboxOGDuke.dButtons |= DUP;
            if (getButtonPress(DOWN))    XboxOGDuke.dButtons |= DDOWN;
            if (getButtonPress(LEFT))    XboxOGDuke.dButtons |= DLEFT;
            if (getButtonPress(RIGHT))   XboxOGDuke.dButtons |= DRIGHT;;
            if (getButtonPress(START))   XboxOGDuke.dButtons |= START_BTN;
            if (getButtonPress(BACK))    XboxOGDuke.dButtons |= BACK_BTN;
            if (getButtonPress(L3))      XboxOGDuke.dButtons |= LS_BTN;
            if (getButtonPress(R3))      XboxOGDuke.dButtons |= RS_BTN;

            //Read Analog Buttons - have to be converted to digital because x360 controllers don't have analog buttons
            getButtonPress(A)    ? XboxOGDuke.A = 0xFF      : XboxOGDuke.A = 0x00;
            getButtonPress(B)    ? XboxOGDuke.B = 0xFF      : XboxOGDuke.B = 0x00;
            getButtonPress(X)    ? XboxOGDuke.X = 0xFF      : XboxOGDuke.X = 0x00;
            getButtonPress(Y)    ? XboxOGDuke.Y = 0xFF      : XboxOGDuke.Y = 0x00;
            getButtonPress(L1)   ? XboxOGDuke.WHITE = 0xFF  : XboxOGDuke.WHITE = 0x00;
            getButtonPress(R1)   ? XboxOGDuke.BLACK = 0xFF  : XboxOGDuke.BLACK = 0x00;

            //Read Analog triggers
            XboxOGDuke.L = getButtonPress(L2); //0x00 to 0xFF
            XboxOGDuke.R = getButtonPress(R2); //0x00 to 0xFF

            //Read Control Sticks (16bit signed short)
            XboxOGDuke.leftStickX = getAnalogHat(LeftHatX);
            XboxOGDuke.leftStickY = getAnalogHat(LeftHatY);
            XboxOGDuke.rightStickX = getAnalogHat(RightHatX);
            XboxOGDuke.rightStickY = getAnalogHat(RightHatY);
            

            if (motionOn) {
                if (controllerType == 3 || controllerType == 4) {
                getMotion();
                limitInputAngles();
                // TO DO - change to accomodate a max angle set in settings.h (or by user?)
                relativeRollAngle = rollAngle - 180; // Shifts value between 135 & 225 to between -45 & 45
                relativePitchAngle = pitchAngle - 180;
                lookXAdjust_f = (float)relativeRollAngle / 45; // A proportion of the maximum
			    lookYAdjust_f = (float)relativePitchAngle / 45;
                XboxOGDuke.rightStickX = lookXAdjust_f * 32767; // Multiply by max possible value of normal analog stick output 
				XboxOGDuke.rightStickY = lookYAdjust_f * 32767;
                }
            }

            // if (motionOn == true) {
			// 	if (controllerType == 3 || controllerType == 4) {
            //         // motionCurrentMillis = millis();
            //         // if (motionCurrentMillis - motionStartMillis >= motionPeriod) {
            //             getMotion();
                        // if (currentMotionX != lastMotionX) {
                            // motionAdjustX = currentMotionX - lastMotionX;
                            // lastMotionX = currentMotionX;
                            // Serial1.print("X: ");
                            // Serial1.println(motionAdjustX);
                            // Serial1.print("X: Stick: ");
                            // Serial1.println(XboxOGDuke.rightStickX);
                        // }
                        // if (currentMotionY != lastMotionY) {
                            // motionAdjustY = currentMotionY - lastMotionY;
                            // lastMotionY = currentMotionY;
                            // Serial1.print("Y: ");
                            // Serial1.println(motionAdjustY);
                            // Serial1.print("Y Stick: ");
                            // Serial1.println(XboxOGDuke.rightStickY);
                            // Serial1.println("\n\n");
                        // }
                        // motionStartMillis = motionCurrentMillis;
                        // XboxOGDuke.rightStickX += motionAdjustX * 1000;
                        // XboxOGDuke.rightStickY += motionAdjustY * 1000;
                    
					// newMotionX = (int16_t)PS3Wired.getAngle(Roll);
					// newMotionY = (int16_t)PS3Wired.getAngle(Pitch);
					// currentMotionX = (currentMotionX > 225) ? 225 : currentMotionX;
					// currentMotionX = (currentMotionX < 135) ? 135 : currentMotionX;
					// currentMotionY = (currentMotionY > 225) ? 225 : currentMotionY;
					// currentMotionY = (currentMotionY < 135) ? 135 : currentMotionY;
					// currentMotionX = (currentMotionX > initMotionX + 45) ? initMotionX + 45 : currentMotionX + motionXMod;
					// currentMotionX = (currentMotionX < initMotionX - 45) ? initMotionX - 45 : currentMotionX + motionXMod;
					// currentMotionY = (currentMotionY > initMotionY + 45) ? initMotionY + 45 : currentMotionY + motionYMod;
					// currentMotionY = (currentMotionY < initMotionY - 45) ? initMotionY - 45 : currentMotionY + motionYMod;
					// currentMotionX = currentMotionX - 180; // Value between -45 and 45
					// currentMotionY = currentMotionY - 180;
					// motionAdjustXf = (float)currentMotionX / 45;
					// motionAdjustYf = (float)currentMotionY / 45;
					// motionAdjustX = (int16_t)motionAdjustXf * 32767;
					// motionAdjustY = (int16_t)motionAdjustYf * 32767;
					// XboxOGDuke.rightStickX = motionAdjustX;
					// XboxOGDuke.rightStickY = motionAdjustY;
                    // }

				// }

			// }


            //Anything that sends a command to the Xbox 360 controllers happens here.
            //(i.e rumble, LED changes, controller off command)
            static uint32_t commandTimer = 0;
            static uint32_t xboxHoldTimer = 0;
            if (millis() - commandTimer > 16)
            {
                //If you hold the XBOX button for more than ~1second, turn off controller
                if (getButtonPress(XBOX) && getButtonPress(R2) > 0x00)
                {
                    if (xboxHoldTimer == 0)
                    {
                        xboxHoldTimer = millis();
                    }
                    if ((millis() - xboxHoldTimer) > 1000 && (millis() - xboxHoldTimer) < 1100)
                    {

                        xboxHoldTimer = 0;
                        motionOn = !motionOn;
                        #ifdef ENABLE_MOTION
                        if (motionOn) {
                            // startMotion();
                            // getMotion();
                            // oldMotionX = currentMotionX;
                            // oldMotionY = currentMotionY;
                        }
                        #endif
                        #ifdef ENABLE_OLED
                        updateOled();
                        #endif
                    }
                }
                else if (getButtonPress(XBOX) && getButtonPress(L2) > 0x00)
                {
                    if (xboxHoldTimer == 0)
                    {
                        xboxHoldTimer = millis();
                    }
                    if ((millis() - xboxHoldTimer) > 1000 && (millis() - xboxHoldTimer) < 1100)
                    {

                        xboxHoldTimer = 0;
                        rumbleOn = !rumbleOn;
                        #ifdef ENABLE_OLED
                        updateOled();
                        #endif
                    }
                }
                #ifdef ENABLE_RUMBLE
                //START+BACK TRIGGERS is a standard soft reset command.
                //We turn off the rumble motors here to prevent them getting locked on
                //if you happen to press this reset combo mid rumble.
                else if (getButtonPress(START) && getButtonPress(BACK) &&
                            getButtonPress(L2) > 0x00 && getButtonPress(R2) > 0x00)
                {       
                    //Turn off rumble
                    XboxOGDuke.left_actuator = 0;
                    XboxOGDuke.right_actuator = 0;
                    XboxOGDuke.rumbleUpdate = 1;
                    
                }
                #endif
                else
                {   
                    xboxHoldTimer = 0; //Reset the XBOX button hold time counter.
                    #ifdef ENABLE_RUMBLE 
                    if (XboxOGDuke.rumbleUpdate == 1)
                    {   
                        setRumbleOn(XboxOGDuke.left_actuator, XboxOGDuke.right_actuator);
                        XboxOGDuke.rumbleUpdate = 0;
                    }
                    #endif
                }
                commandTimer = millis();
            }

            sendControllerHIDReport();
        }


        //Handle Player 1 controller connect/disconnect events.
        if (controllerConnected() && disconnectTimer == 0)
        {
            USB_Attach();
            if (enumerationComplete)
            {
                digitalWrite(ARDUINO_LED_PIN, LOW);
            }
        }
        else if (millis() > 7000)
        {
            digitalWrite(ARDUINO_LED_PIN, HIGH);
            USB_Detach(); //Disconnect from the OG Xbox port.
        }
        else
        {
            USB_Attach();
            sendControllerHIDReport();
        }


        //THPS 2X is the only game I know that sends rumble commands to the USB OUT pipe
        //instead of the control pipe. So unfortunately need to manually read the out pipe
        //and update rumble values as needed!
        // uint8_t ep = Endpoint_GetCurrentEndpoint();
        // static uint8_t report[6];
        // Endpoint_SelectEndpoint(0x02); //0x02 is the out endpoint address for the Duke Controller
        // if (Endpoint_IsOUTReceived())
        // {
        //     Endpoint_Read_Stream_LE(report, 6, NULL);
        //     Endpoint_ClearOUT();
        //     if (report[1] == 0x06)
        //     {
        //         XboxOGDuke.left_actuator = report[3];
        //         XboxOGDuke.right_actuator = report[5];
        //         XboxOGDuke.rumbleUpdate = 1;
        //     }
        //     report[1] = 0x00;
        // }
        // Endpoint_SelectEndpoint(ep); //set back to the old endpoint.

        // checkVoltage();

    }
}

/* Send the HID report to the OG Xbox */
void sendControllerHIDReport()
{
    if (USB_Device_GetFrameNumber() - DukeController_HID_Interface.State.PrevFrameNum >= 4)
    {
        HID_Device_USBTask(&DukeController_HID_Interface); //Send OG Xbox HID Report
    }
    USB_USBTask();
}

//Parse button presses for each type of controller
uint8_t getButtonPress(ButtonEnum b)
{

    uint8_t psVal = 0;


    if (Xbox360Wired.Xbox360Connected)
        return Xbox360Wired.getButtonPress(b);

    if (XboxOneWired.XboxOneConnected)
    {
        if (b == L2 || b == R2)
        {
            //Xbone one triggers are 10-bit, remove 2LSBs so its 8bit like OG Xbox
            return (uint8_t)(XboxOneWired.getButtonPress(b) >> 2); 
        }
        else
        {
            return (uint8_t)XboxOneWired.getButtonPress(b);
        }
    }

    if (PS3Wired.PS3Connected) {
		switch (b) {
			// Remap the PS3 controller face buttons to their Xbox counterparts
			case A:
				psVal = (uint8_t)PS3Wired.getButtonPress(CROSS); // TO DO - are these casts needed now? And in PS4 code.
				break;
			case B:
				psVal = (uint8_t)PS3Wired.getButtonPress(CIRCLE);
				break;
			case X:
				psVal = (uint8_t)PS3Wired.getButtonPress(SQUARE);
				break;
			case Y:
				psVal = (uint8_t)PS3Wired.getButtonPress(TRIANGLE);
				break;
			// Call a different function from the PS3USB library to get the level of
			// pressure applied to the L2 and R2 triggers, not just 'on' or 'off
			case L2:
				psVal = (uint8_t)PS3Wired.getAnalogButton(L2);
				break;
			case R2:
				psVal = (uint8_t)PS3Wired.getAnalogButton(R2);
				break;
			// Requests for the start, select, R1, L1 and the D-pad buttons can be called normally
			default:
				psVal = (uint8_t)PS3Wired.getButtonPress(b);
		}
		return psVal;
	}

	if (PS4Wired.connected()) {
		switch (b) {
			// Remap the PS4 controller face buttons to their Xbox counterparts
			case A:
				psVal = (uint8_t)PS4Wired.getButtonPress(CROSS);
				break;
			case B:
				psVal = (uint8_t)PS4Wired.getButtonPress(CIRCLE);
				break;
			case X:
				psVal = (uint8_t)PS4Wired.getButtonPress(SQUARE);
				break;
			case Y:
				psVal = (uint8_t)PS4Wired.getButtonPress(TRIANGLE);
				break;
			// Call a different function from the PS4USB library to get the level of
			// pressure applied to the L2 and R2 triggers, not just 'on' or 'off
			case L2:
				psVal = (uint8_t)PS4Wired.getAnalogButton(L2);
				break;
			case R2:
				psVal = (uint8_t)PS4Wired.getAnalogButton(R2);
				break;
			default:
				psVal = (uint8_t)PS4Wired.getButtonPress(b);
		}
		return psVal;
	}

    return 0;
}

//Parse analog stick requests for each type of controller.
int16_t getAnalogHat(AnalogHatEnum a)
{

    if (Xbox360Wired.Xbox360Connected)
    {
        int16_t val;
        val = Xbox360Wired.getAnalogHat(a);
        if (val == -32512) //8bitdo range fix
            val = -32768;
        return val;
    }

    if (XboxOneWired.XboxOneConnected)
        return XboxOneWired.getAnalogHat(a);

    if (PS3Wired.PS3Connected) {
		// Scale up the unsigned 8bit values produced by the PS3 analog sticks to the
		// signed 16bit values expected by the Xbox. In the case of the Y axes, invert the result
		if (a == RightHatY || a == LeftHatY) {
			return (PS3Wired.getAnalogHat(a) - 127) * -255;
		} else {
			return (PS3Wired.getAnalogHat(a) - 127) * 255;
		}
	}

	if (PS4Wired.connected()) {
		if (a == RightHatY || a == LeftHatY) {
			return (PS4Wired.getAnalogHat(a) - 127) * -255;
		} else {
			return (PS4Wired.getAnalogHat(a) - 127) * 255;
		}
	}

    return 0;
}

#ifdef ENABLE_RUMBLE
//Parse rumble activation requests for each type of controller.
void setRumbleOn(uint8_t lValue, uint8_t rValue)
{
    if (rumbleOn) {
        if (Xbox360Wired.Xbox360Connected)
        {
            Xbox360Wired.setRumbleOn(lValue, rValue); 
        }

        if (XboxOneWired.XboxOneConnected)
        {
            XboxOneWired.setRumbleOn(lValue / 8, rValue / 8, lValue / 2, rValue / 2);
        }

        // TO DO - add left and right values
        // TO DO - consider separate rumbleOff function
        if (PS3Wired.PS3Connected)
        {   
            if (lValue == 0 && rValue == 0) {
                PS3Wired.setRumbleOff();
            } else {
                PS3Wired.setRumbleOn(RumbleLow);
            }
        }

        if (PS4Wired.connected())
        {   
            if (lValue == 0 && rValue == 0) {
                PS4Wired.setRumbleOff();
            } else {
                PS4Wired.setRumbleOn(RumbleLow);
            }
        }
    }
}
#endif

//Parse LED activation requests for each type of controller.
void setLedOn(LEDEnum led)
{

    if (Xbox360Wired.Xbox360Connected)
        Xbox360Wired.setLedOn(led);

    if (XboxOneWired.XboxOneConnected)
    {
        //no LEDs on Xbox One Controller. I think it is possible to adjust brightness but this is not implemented.
    }

    if (PS3Wired.PS3Connected)
	PS3Wired.setLedOn(led);

    // if (PS3Wired.PS3Connected)
	// PS3Wired.setLedOn(led);

}

uint8_t controllerConnected()
{
    uint8_t controllerType = 0;

    if (Xbox360Wired.Xbox360Connected)
        controllerType =  1;

    if (XboxOneWired.XboxOneConnected)
        controllerType =  2;

    if (PS3Wired.PS3Connected)
		controllerType =  3;

	if (PS4Wired.connected())
		controllerType =  4;

    return controllerType;
}

void checkControllerChange() {
    uint8_t currentController = controllerConnected();
    if (currentController != controllerType) {
        controllerType = currentController;
        #ifdef ENABLE_OLED
        updateOled();
        #endif
    }
}

#ifdef ENABLE_OLED
void updateOled() {
    oled.clear();
    if (controllerType == 1) {
        oled.println("Xbox 360");
    } else if (controllerType == 2) {
        oled.println("Xbox One");
    } else if (controllerType == 3) {
        oled.println("PS3");
    } else if (controllerType == 4) {
        oled.println("PS4");
    } else {
        oled.println("N/C");
    }

    oled.print("Rumble ");
    if (rumbleOn == true) {
        oled.println("On");
    } else {
        oled.println("Off");
    }

    oled.print("Motion ");
    if (motionOn == true) {
        oled.println("On");
    } else {
        oled.println("Off");
    }

    getStatus();
    oled.println(status);
}
#endif

#ifdef ENABLE_MOTION
void getMotion() {

    if (controllerType == 3) {
        rollAngle = (int16_t)PS3Wired.getAngle(Roll);
        pitchAngle = (int16_t)PS3Wired.getAngle(Pitch);
    } else if (controllerType == 4) {
        rollAngle = (int16_t)PS4Wired.getAngle(Roll);
        pitchAngle = (int16_t)PS4Wired.getAngle(Pitch);
    }
}

// void startMotion() {
//     getMotion();
//     lastMotionX = motionX;
//     lastMotionY = motionY;
//     motionStartMillis = millis();
// }

// void startMotion() {
//     getMotion();
//     initMotionX = motionX;
//     initMotionY = motionY;
//     motionStartMillis = millis();
// }

void limitInputAngles() {
    rollAngle = (rollAngle > maxRollInputAngle) ? maxRollInputAngle : rollAngle;
    rollAngle = (rollAngle < minRollInputAngle) ? minRollInputAngle : rollAngle;
    pitchAngle = (pitchAngle > maxPitchInputAngle) ? maxPitchInputAngle : pitchAngle;
    pitchAngle = (pitchAngle < minPitchInputAngle) ? minPitchInputAngle : pitchAngle;
}

#endif

void getStatus() {
    if (controllerType == 3) {
        if (PS3Wired.getStatus(Full)) {
            status = 15;
        } else if (PS3Wired.getStatus(High)) {
            status = 10;
        } else if (PS3Wired.getStatus(Low)) {
            status = 5;
        } else if (PS3Wired.getStatus(Dying)) {
            status = 1;
        }
        
    } else if (controllerType == 4) {
        status = PS4Wired.getBatteryLevel();
    }
}