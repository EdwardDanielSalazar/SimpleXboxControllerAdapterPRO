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
#include "Wire.h"
#include "EEPROM.h"

#include <XBOXRECV.h>
#include <usbhub.h>
#include <XBOXONE.h>
#include <XBOXUSB.h>


uint8_t playerID; //playerID is set in the main program based on the slot the Arduino is installed.
USB_XboxGamepad_Data_t XboxOGDuke[4]; //Xbox gamepad data structure to store all button and actuator states for all four controllers.
uint8_t ConnectedXID = DUKE_CONTROLLER; //Default XID device to emulate
bool enumerationComplete=false; //Flag is set when the device has been successfully setup by the OG Xbox
uint32_t disconnectTimer=0; //Timer used to time disconnection between SB and Duke controller swapover

USB UsbHost;
USBHub Hub(&UsbHost);
XBOXRECV Xbox360Wireless(&UsbHost);
uint8_t getButtonPress(ButtonEnum b, uint8_t controller);
int16_t getAnalogHat(AnalogHatEnum a, uint8_t controller);
void setRumbleOn(uint8_t lValue, uint8_t rValue, uint8_t controller);
void setLedOn(LEDEnum led, uint8_t controller);
bool controllerConnected(uint8_t controller);

XBOXONE XboxOneWired1(&UsbHost);
XBOXONE XboxOneWired2(&UsbHost);
XBOXONE XboxOneWired3(&UsbHost);
XBOXONE XboxOneWired4(&UsbHost);
XBOXONE *XboxOneWired[4] = {&XboxOneWired1, &XboxOneWired2, &XboxOneWired3, &XboxOneWired4};

XBOXUSB Xbox360Wired1(&UsbHost);
XBOXUSB Xbox360Wired2(&UsbHost);
XBOXUSB Xbox360Wired3(&UsbHost);
XBOXUSB Xbox360Wired4(&UsbHost);
XBOXUSB *Xbox360Wired[4] = {&Xbox360Wired1, &Xbox360Wired2, &Xbox360Wired3, &Xbox360Wired4};

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

	playerID = 0;

	//Init the XboxOG data arrays to zero.
	memset(&XboxOGDuke,0x00,sizeof(USB_XboxGamepad_Data_t)*4);

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

		int i = 0;
		UsbHost.Task();
		if (controllerConnected(i)) {
			//Button Mapping for Duke Controller
			if(ConnectedXID == DUKE_CONTROLLER || i != 0){

				//Read Digital Buttons
				XboxOGDuke[i].dButtons=0x0000;
				if (getButtonPress(UP, i))      XboxOGDuke[i].dButtons |= DUP;
				if (getButtonPress(DOWN, i))    XboxOGDuke[i].dButtons |= DDOWN;
				if (getButtonPress(LEFT, i))    XboxOGDuke[i].dButtons |= DLEFT;
				if (getButtonPress(RIGHT, i))   XboxOGDuke[i].dButtons |= DRIGHT;;
				if (getButtonPress(START, i))   XboxOGDuke[i].dButtons |= START_BTN;
				if (getButtonPress(BACK, i))    XboxOGDuke[i].dButtons |= BACK_BTN;
				if (getButtonPress(L3, i))      XboxOGDuke[i].dButtons |= LS_BTN;
				if (getButtonPress(R3, i))      XboxOGDuke[i].dButtons |= RS_BTN;

				//Read Analog Buttons - have to be converted to digital because x360 controllers don't have analog buttons
				getButtonPress(A, i)    ? XboxOGDuke[i].A = 0xFF      : XboxOGDuke[i].A = 0x00;
				getButtonPress(B, i)    ? XboxOGDuke[i].B = 0xFF      : XboxOGDuke[i].B = 0x00;
				getButtonPress(X, i)    ? XboxOGDuke[i].X = 0xFF      : XboxOGDuke[i].X = 0x00;
				getButtonPress(Y, i)    ? XboxOGDuke[i].Y = 0xFF      : XboxOGDuke[i].Y = 0x00;
				getButtonPress(L1, i)   ? XboxOGDuke[i].WHITE = 0xFF  : XboxOGDuke[i].WHITE = 0x00;
				getButtonPress(R1, i)   ? XboxOGDuke[i].BLACK = 0xFF  : XboxOGDuke[i].BLACK = 0x00;

				//Read Analog triggers
				XboxOGDuke[i].L = getButtonPress(L2, i); //0x00 to 0xFF
				XboxOGDuke[i].R = getButtonPress(R2, i); //0x00 to 0xFF

				//Read Control Sticks (16bit signed short)
				XboxOGDuke[i].leftStickX = getAnalogHat(LeftHatX, i);
				XboxOGDuke[i].leftStickY = getAnalogHat(LeftHatY, i);
				XboxOGDuke[i].rightStickX = getAnalogHat(RightHatX, i);
				XboxOGDuke[i].rightStickY = getAnalogHat(RightHatY, i);
			}

			//Anything that sends a command to the Xbox 360 controllers happens here.
			//(i.e rumble, LED changes, controller off command)
			static uint32_t commandTimer[4] ={0,0,0,0};
			static uint32_t xboxHoldTimer[4] ={0,0,0,0};
			if(millis()-commandTimer[i]>16){
				//If you hold the XBOX button for more than ~1second, turn off controller
				if (getButtonPress(XBOX, i)) {
					if(xboxHoldTimer[i]==0){
						xboxHoldTimer[i]=millis();
					}
					if((millis()-xboxHoldTimer[i])>1000 && (millis()-xboxHoldTimer[i])<1100){
						XboxOGDuke[i].dButtons = 0x00;
						setRumbleOn(0, 0, i);
						delay(10);
						Xbox360Wireless.disconnect(i);
						xboxHoldTimer[i]=0;
					}
				//START+BACK TRIGGERS is a standard soft reset command. 
				//We turn off the rumble motors here to prevent them getting locked on
				//if you happen to press this reset combo mid rumble.
				} else if (getButtonPress(START, i) && getButtonPress(BACK, i) && 
							getButtonPress(L2, i)>0x00 && getButtonPress(R2, i)>0x00) {
					//Turn off rumble on all controllers
					for(uint8_t j=0; j<4; j++){
						XboxOGDuke[j].left_actuator=0;
						XboxOGDuke[j].right_actuator=0;
						XboxOGDuke[j].rumbleUpdate=1;
					}
				//If Xbox button isnt held down, send the rumble commands
				} else {
					xboxHoldTimer[i]=0; //Reset the XBOX button hold time counter.
					if (XboxOGDuke[i].rumbleUpdate==1){
						setRumbleOn(XboxOGDuke[i].left_actuator, XboxOGDuke[i].right_actuator, i);
						XboxOGDuke[i].rumbleUpdate=0;
					}
				}
				commandTimer[i]=millis();
			}

			/*Check/send the Player 1 HID report every loop to minimise lag even more on the master*/
			sendControllerHIDReport();

		} else {
			//If the respective controller isn't synced, we instead send a disablePacket over the i2c bus
			//so that the slave device knows to disable its USB. I've arbitrarily made this 0xF0.
			if(i>0){
				static uint8_t disablePacket[1] = {0xF0};
				Wire.beginTransmission(i);
				Wire.write((char*)disablePacket,1);
				Wire.endTransmission(true);
			}
		}

		//Handle Player 1 controller connect/disconnect events.
		if (controllerConnected(0) && disconnectTimer==0){
			USB_Attach();
			if(enumerationComplete){
				digitalWrite(ARDUINO_LED_PIN, LOW);
			}
		} else if(millis()>7000){
			digitalWrite(ARDUINO_LED_PIN, HIGH);
			USB_Detach(); //Disconnect from the OG Xbox port.
			Xbox360Wireless.chatPadInitNeeded[0]=1;
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
				XboxOGDuke[0].left_actuator =  report[3];
				XboxOGDuke[0].right_actuator = report[5];
				XboxOGDuke[0].rumbleUpdate = 1;
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
uint8_t getButtonPress(ButtonEnum b, uint8_t controller){
	if(Xbox360Wireless.Xbox360Connected[controller])
	return Xbox360Wireless.getButtonPress(b, controller);

	#ifdef SUPPORTWIREDXBOX360
	if (Xbox360Wired[controller]->Xbox360Connected)
	return Xbox360Wired[controller]->getButtonPress(b);
	#endif

	#ifdef SUPPORTWIREDXBOXONE
	if (XboxOneWired[controller]->XboxOneConnected){
		if(b==L2 || b==R2){
			return (uint8_t)(XboxOneWired[controller]->getButtonPress(b)>>2); //Xbone one triggers are 10-bit, remove 2LSBs so its 8bit like OG Xbox
			} else {
			return (uint8_t)XboxOneWired[controller]->getButtonPress(b);
		}
	}
	#endif

	return 0;
}

//Parse analog stick requests for each type of controller.
int16_t getAnalogHat(AnalogHatEnum a, uint8_t controller){
	int32_t val=0;
	if(Xbox360Wireless.Xbox360Connected[controller])
		return Xbox360Wireless.getAnalogHat(a, controller);

	#ifdef SUPPORTWIREDXBOX360
	if (Xbox360Wired[controller]->Xbox360Connected){
		val = Xbox360Wired[controller]->getAnalogHat(a);
		if(val==-32512) //8bitdo range fix
			val=-32768;
		return val;
	}

	#endif

	#ifdef SUPPORTWIREDXBOXONE
	if (XboxOneWired[controller]->XboxOneConnected)
		return XboxOneWired[controller]->getAnalogHat(a);
	#endif

	return 0;
}

//Parse rumble activation requests for each type of controller.
void setRumbleOn(uint8_t lValue, uint8_t rValue, uint8_t controller){
	if(Xbox360Wireless.Xbox360Connected[controller])
	Xbox360Wireless.setRumbleOn(lValue, rValue, controller);

	#ifdef SUPPORTWIREDXBOX360
	if (Xbox360Wired[controller]->Xbox360Connected){
		Xbox360Wired[controller]->setRumbleOn(lValue, rValue); //If you have an externally power USB 2.0 hub you can uncomment this to enable rumble
	}
	#endif

	#ifdef SUPPORTWIREDXBOXONE
	if (XboxOneWired[controller]->XboxOneConnected){
		XboxOneWired[controller]->setRumbleOn(lValue/8, rValue/8, lValue/2, rValue/2);
	}
	#endif
}

//Parse LED activation requests for each type of controller.
void setLedOn(LEDEnum led, uint8_t controller){
	if(Xbox360Wireless.Xbox360Connected[controller])
	Xbox360Wireless.setLedOn(led,controller);

	#ifdef SUPPORTWIREDXBOX360
	if (Xbox360Wired[controller]->Xbox360Connected)
	Xbox360Wired[controller]->setLedOn(led);
	#endif

	#ifdef SUPPORTWIREDXBOXONE
	if (XboxOneWired[controller]->XboxOneConnected){
		//no LEDs on Xbox One Controller. I think it is possible to adjust brightness but this is not implemented.
	}
	#endif
}

bool controllerConnected(uint8_t controller){
	if (Xbox360Wireless.Xbox360Connected[controller])
		return 1;

	#ifdef SUPPORTWIREDXBOX360
	if (Xbox360Wired[controller]->Xbox360Connected)
		return 1;
	#endif

	#ifdef SUPPORTWIREDXBOXONE
	if (XboxOneWired[controller]->XboxOneConnected)
		return 1;
	#endif
	return 0;
}
