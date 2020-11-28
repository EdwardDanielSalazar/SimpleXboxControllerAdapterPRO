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
#include "EEPROM.h"
#include <usbhub.h> // TO DO Take this out?
#include <XBOXONE.h> // TO DO Look again at rumble settings in XBOXONE.cpp
#include <XBOXUSB.h>


// uint8_t playerID; //playerID is set in the main program based on the slot the Arduino is installed.

//*********************
// TO DO - reduce size of this array before turning into var
USB_XboxGamepad_Data_t XboxOGDuke; //Xbox gamepad data structure to store all button and actuator states for all four controllers.
uint8_t ConnectedXID = DUKE_CONTROLLER; //Default XID device to emulate
bool enumerationComplete=false; //Flag is set when the device has been successfully setup by the OG Xbox
uint32_t disconnectTimer=0; //Timer used to time disconnection between SB and Duke controller swapover

USB UsbHost;
USBHub Hub(&UsbHost); // TO DO Take this out if header successfully removed
uint8_t getButtonPress(ButtonEnum b);
int16_t getAnalogHat(AnalogHatEnum a);
void setRumbleOn(uint8_t lValue, uint8_t rValue);
void setLedOn(LEDEnum led);
bool controllerConnected();

// XBOXONE XboxOneWired1(&UsbHost);
// XBOXONE *XboxOneWired[1] = {&XboxOneWired1};

// XBOXUSB Xbox360Wired1(&UsbHost);
// XBOXUSB *Xbox360Wired[1] = {&Xbox360Wired1};

XBOXONE XboxOneWired(&UsbHost);
XBOXUSB Xbox360Wired(&UsbHost);

int main(void)
{
	//Init the Arduino Library
	init();

	//Init IO
	pinMode(USB_HOST_RESET_PIN, OUTPUT);
	pinMode(ARDUINO_LED_PIN, OUTPUT);
	pinMode(PLAYER_ID1_PIN, INPUT_PULLUP);
	pinMode(PLAYER_ID2_PIN, INPUT_PULLUP);
	digitalWrite(USB_HOST_RESET_PIN, LOW);
	digitalWrite(ARDUINO_LED_PIN, HIGH);

	//Init the LUFA USB Device Library
	SetupHardware();
	GlobalInterruptEnable();

	// playerID = 0;


	//*********************
	// TO DO - CHANGE THIS ONCE XboxOGDuke is no longer array
	//Init the XboxOG data arrays to zero.
	memset(&XboxOGDuke,0x00,sizeof(USB_XboxGamepad_Data_t));

	/* MASTER DEVICE USB HOST CONTROLLER INIT */

	//Init Usb Host Controller
	digitalWrite(USB_HOST_RESET_PIN, LOW);
	delay(20);//wait 20ms to reset the IC. Reseting at startup improves reliability in my experience.
	digitalWrite(USB_HOST_RESET_PIN, HIGH);
	delay(20); //Settle
	while (UsbHost.Init() == -1) {
		digitalWrite(ARDUINO_LED_PIN, !digitalRead(ARDUINO_LED_PIN));
		delay(500);
	}

	while (1){

		/*** MASTER TASKS ***/
		UsbHost.busprobe();

		// uint8_t i = 0;
		UsbHost.Task();
		if (controllerConnected()) {
			//Button Mapping for Duke Controller
			// if(ConnectedXID == DUKE_CONTROLLER || i != 0){ // TO DO Remove this

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
			// }

			//Anything that sends a command to the Xbox 360 controllers happens here.
			//(i.e rumble, LED changes, controller off command)
			static uint32_t commandTimer = 0;
			static uint32_t xboxHoldTimer = 0;
			if(millis()-commandTimer>16){
				//If you hold the XBOX button for more than ~1second, turn off controller
				if (getButtonPress(XBOX)) {
					if(xboxHoldTimer==0){
						xboxHoldTimer=millis();
					}
					if((millis()-xboxHoldTimer)>1000 && (millis()-xboxHoldTimer)<1100){
						XboxOGDuke.dButtons = 0x00;
						setRumbleOn(0, 0);
						delay(10);
						// Xbox360Wireless.disconnect(i);
						xboxHoldTimer=0;
					}
				//START+BACK TRIGGERS is a standard soft reset command. 
				//We turn off the rumble motors here to prevent them getting locked on
				//if you happen to press this reset combo mid rumble.
				} else if (getButtonPress(START) && getButtonPress(BACK) && 
							getButtonPress(L2)>0x00 && getButtonPress(R2)>0x00) {
					//Turn off rumble on all controllers
					// for(uint8_t j=0; j<4; j++){
						// uint8_t j = 0;
						XboxOGDuke.left_actuator=0;
						XboxOGDuke.right_actuator=0;
						XboxOGDuke.rumbleUpdate=1;
					// }
				//If Xbox button isnt held down, send the rumble commands
				} else {
					xboxHoldTimer=0; //Reset the XBOX button hold time counter.
					if (XboxOGDuke.rumbleUpdate==1){
						setRumbleOn(XboxOGDuke.left_actuator, XboxOGDuke.right_actuator);
						XboxOGDuke.rumbleUpdate=0;
					}
				}
				commandTimer=millis();
			}

			/*Check/send the Player 1 HID report every loop to minimise lag even more on the master*/
			sendControllerHIDReport();

		}
		//Handle Player 1 controller connect/disconnect events.
		if (controllerConnected() && disconnectTimer==0){
			USB_Attach();
			if(enumerationComplete){
				digitalWrite(ARDUINO_LED_PIN, LOW);
			}
		} else if(millis()>7000){
			digitalWrite(ARDUINO_LED_PIN, HIGH);
			USB_Detach(); //Disconnect from the OG Xbox port.
			// Xbox360Wireless.chatPadInitNeeded[0]=1; // Why is this not within its own if block?
		} else {
			USB_Attach();
			sendControllerHIDReport();
		}

		/***END MASTER TASKS ***/
		// #endif

		//THPS 2X is the only game I know that sends rumble commands to the USB OUT pipe
		//instead of the control pipe. So unfortunately need to manually read the out pipe
		//and update rumble values as needed!
		uint8_t ep = Endpoint_GetCurrentEndpoint();
		static uint8_t report[6];
		Endpoint_SelectEndpoint(0x02); //0x02 is the out endpoint address for the Duke Controller
		if (Endpoint_IsOUTReceived()){
			Endpoint_Read_Stream_LE(report, 6, NULL);
			Endpoint_ClearOUT();
			if(report[1]==0x06){
				XboxOGDuke.left_actuator =  report[3];
				XboxOGDuke.right_actuator = report[5];
				XboxOGDuke.rumbleUpdate = 1;
			}
			report[1]=0x00;
		}
		Endpoint_SelectEndpoint(ep); //set back to the old endpoint.
	}
}

/* Send the HID report to the OG Xbox */
void sendControllerHIDReport(){
	USB_USBTask();
	switch (ConnectedXID){
		case DUKE_CONTROLLER:
		if(USB_Device_GetFrameNumber()-DukeController_HID_Interface.State.PrevFrameNum>=4){
			HID_Device_USBTask(&DukeController_HID_Interface); //Send OG Xbox HID Report
		}
		break;
	}
}


// #ifdef MASTER
//Parse button presses for each type of controller
uint8_t getButtonPress(ButtonEnum b){

	if (Xbox360Wired.Xbox360Connected)
	return Xbox360Wired.getButtonPress(b);

	if (XboxOneWired.XboxOneConnected){
		if(b==L2 || b==R2){
			return (uint8_t)(XboxOneWired.getButtonPress(b)>>2); //Xbone one triggers are 10-bit, remove 2LSBs so its 8bit like OG Xbox
			} else {
			return (uint8_t)XboxOneWired.getButtonPress(b);
		}
	}

	return 0;
}

//Parse analog stick requests for each type of controller.
int16_t getAnalogHat(AnalogHatEnum a){
	int32_t val=0;

	if (Xbox360Wired.Xbox360Connected){
		val = Xbox360Wired.getAnalogHat(a);
		if(val==-32512) //8bitdo range fix
			val=-32768;
		return val;
	}

	if (XboxOneWired.XboxOneConnected)
		return XboxOneWired.getAnalogHat(a);

	return 0;
}

//Parse rumble activation requests for each type of controller.
void setRumbleOn(uint8_t lValue, uint8_t rValue){

	if (Xbox360Wired.Xbox360Connected){
		Xbox360Wired.setRumbleOn(lValue, rValue); //If you have an externally power USB 2.0 hub you can uncomment this to enable rumble
	}

	if (XboxOneWired.XboxOneConnected){
		XboxOneWired.setRumbleOn(lValue/8, rValue/8, lValue/2, rValue/2);
	}
}

//Parse LED activation requests for each type of controller.
void setLedOn(LEDEnum led){

	if (Xbox360Wired.Xbox360Connected)
	Xbox360Wired.setLedOn(led);

	if (XboxOneWired.XboxOneConnected){
		//no LEDs on Xbox One Controller. I think it is possible to adjust brightness but this is not implemented.
	}
}

bool controllerConnected(){

	if (Xbox360Wired.Xbox360Connected)
		return 1;

	if (XboxOneWired.XboxOneConnected)
		return 1;

	return 0;
}
