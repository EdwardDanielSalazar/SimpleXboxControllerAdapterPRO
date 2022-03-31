#include "settings.h"
#include "xiddevice.h"
// #include "EEPROM.h" // ?? Remove this ??
#include <SPI.h>

#ifdef ENABLE_XBOX360
#include <XBOXRECV.h>
#endif
#ifdef ENABLE_XBOXBT
#include <XBOXONESBT.h>
#endif

//#include <PS3USB.h>
#ifdef ENABLE_PS4BT
#include <PS4BT.h>
#endif
#ifdef ENABLE_XBOXONEUSB
#include <XBOXONE.h>
#endif
#ifdef ENABLE_SWITCHBT
#include <SwitchProBT.h>
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

USB Usb;


#ifdef ENABLE_PS4BT
BTD Btd(&Usb);
PS4BT PS4Wired(&Btd, PAIR);
#endif
#ifdef ENABLE_SWITCHBT
BTD Btd(&Usb);
SwitchProBT SwitchPro(&Btd, PAIR);
#endif
#ifdef ENABLE_XBOXONEUSB
XBOXONE XBOXONE(&Usb);
#endif
uint8_t getButtonPress(ButtonEnum b);
int16_t getAnalogHat(AnalogHatEnum a);
void setRumbleOn(uint8_t lValue, uint8_t rValue);
void setLedOn(LEDEnum led); // TO DO - do something with this
uint8_t controllerConnected();
void checkControllerChange();

void getStatus();

//XBOXONE XboxOneWired(&UsbHost);

#ifdef ENABLE_XBOX360
XBOXRECV Xbox360Wired(&Usb);
#endif
#ifdef ENABLE_XBOXBT
BTD Btd(&Usb);
XBOXONESBT Xbox(&Btd, PAIR);
#endif
//PS3USB PS3Wired(&UsbHost); //defines EP_MAXPKTSIZE = 64. The change causes a compiler warning but doesn't seem to affect operation

uint8_t controllerType = 0;
uint8_t status = 0;

#ifdef ENABLE_RUMBLE
bool rumbleOn = false;
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
     //Serial1.begin(115200);

    //Init the XboxOG data arrays to zero.
    memset(&XboxOGDuke, 0x00, sizeof(USB_XboxGamepad_Data_t));

    digitalWrite(USB_HOST_RESET_PIN, LOW);
    delay(20); //wait 20ms to reset the IC. Reseting at startup improves reliability in my experience.
    digitalWrite(USB_HOST_RESET_PIN, HIGH);
    delay(20); //Settle
    while (Usb.Init() == -1)
    {
        //while (1); // Halt
        digitalWrite(ARDUINO_LED_PIN, !digitalRead(ARDUINO_LED_PIN));
       // delay(500);
     
    }
     
    while (1)
    {
        Usb.busprobe();
        Usb.Task();

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
            if ((controllerType) == 1||(controllerType) == 2||(controllerType) == 4||(controllerType) == 5){
            XboxOGDuke.L = getButtonPress(L2); //0x00 to 0xFF
            XboxOGDuke.R = getButtonPress(R2);
            }
            
            if ((controllerType) == 3){
            XboxOGDuke.L = getButtonPress(ZLl); //0x00 to 0xFF
            XboxOGDuke.R = getButtonPress(ZR);
            }
            
             //0x00 to 0xFF
            //Read Control Sticks (16bit signed short)
            XboxOGDuke.leftStickX = getAnalogHat(LeftHatX);
            XboxOGDuke.leftStickY = getAnalogHat(LeftHatY);
            XboxOGDuke.rightStickX = getAnalogHat(RightHatX);
            XboxOGDuke.rightStickY = getAnalogHat(RightHatY);
                      
            //Anything that sends a command to the Xbox 360 controllers happens here.
            //(i.e rumble, LED changes, controller off command)
            static uint32_t commandTimer = 0;
            static uint32_t xboxHoldTimer = 0;
            if (millis() - commandTimer > 16)
            {
                // Enable motion controls
                // TO DO - only turn on motion when compatible controller connected
                                
                // Enable rumble
                 if ((getButtonPress(XBOX) && getButtonPress(L2)) > 0x00 || (getButtonPress(HOME) && getButtonPress(ZLl)) > 0x00)
                {
                    if (xboxHoldTimer == 0)
                    {
                        xboxHoldTimer = millis();
                    }
                    if ((millis() - xboxHoldTimer) > 1000 && (millis() - xboxHoldTimer) < 1100)
                    {
                        xboxHoldTimer = 0;
                        #ifdef ENABLE_RUMBLE
                        rumbleOn = !rumbleOn;
                        
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

// TO DO - remove the separate controller connected checks in these functions
//Parse button presses for each type of controller
uint8_t getButtonPress(ButtonEnum b)
{
    
    uint8_t psVal = 0;
    
    #ifdef ENABLE_XBOX360
    if (Xbox360Wired.XboxReceiverConnected){
        return Xbox360Wired.getButtonPress(b);
    }
    #endif
    #ifdef ENABLE_XBOXBT
    if (Xbox.connected())
    {
        if (b == L2 || b == R2)
        {
            //Xbone one triggers are 10-bit, remove 2LSBs so its 8bit like OG Xbox
            return (uint8_t)(Xbox.getButtonPress(b) >> 2); 
        }
        else
        {
            return (uint8_t)Xbox.getButtonPress(b);
        }
    }
    #endif
    #ifdef ENABLE_XBOXONEUSB
    if (XBOXONE.XboxOneConnected)
    {
        if (b == L2 || b == R2)
        {
            //Xbone one triggers are 10-bit, remove 2LSBs so its 8bit like OG Xbox
            return (uint8_t)(XBOXONE.getButtonPress(b) >> 2); 
        }
        else
        {
            return (uint8_t)XBOXONE.getButtonPress(b);
        }
    }
    #endif
    

    #ifdef ENABLE_SWITCHBT
    if (SwitchPro.connected())
    {            
           switch (b) {
			// Remap the PS4 controller face buttons to their Xbox counterparts
			case A:
				psVal = (uint8_t)SwitchPro.getButtonPress(B);
				break;
			case B:
				psVal = (uint8_t)SwitchPro.getButtonPress(A);
				break;
			case X:
				psVal = (uint8_t)SwitchPro.getButtonPress(Y);
				break;
			case Y:
				psVal = (uint8_t)SwitchPro.getButtonPress(X);
				break;
            case BACK:
				psVal = (uint8_t)SwitchPro.getButtonPress(MINUS);
				break;
            case R3:
				psVal = (uint8_t)SwitchPro.getButtonPress(R3);
				break;  
            case L3:
				psVal = (uint8_t)SwitchPro.getButtonPress(L3);
				break; 
            case START:
				psVal = (uint8_t)SwitchPro.getButtonPress(PLUS);
				break;  
            case UP:
				psVal = (uint8_t)SwitchPro.getButtonPress(UP);
				break; 
            case DOWN:
				psVal = (uint8_t)SwitchPro.getButtonPress(DOWN);
				break; 
            case LEFT:
				psVal = (uint8_t)SwitchPro.getButtonPress(LEFT);
				break; 
            case RIGHT:
				psVal = (uint8_t)SwitchPro.getButtonPress(RIGHT);
				break; 
            case R1:
				psVal = (uint8_t)SwitchPro.getButtonPress(R);
				break; 
            case L1:
				psVal = (uint8_t)SwitchPro.getButtonPress(L);
				break;
                  
            case ZLl:
				if(SwitchPro.getButtonPress(ZLl)){
                psVal = 0xff;
                }else{
                psVal = 0x00;
                }
              	break; 
            case ZR:
				if(SwitchPro.getButtonPress(ZR)){
                psVal = 0xff;
                }else{
                psVal = 0x00;
                }
				break;          
                        
			// Call a different function from the PS4USB library to get the level of
			// pressure applied to the L2 and R2 triggers, not just 'on' or 'off
			default:
				psVal = (uint8_t)SwitchPro.getButtonPress(b);
		}
		return psVal;
    }
    #endif
    
	#ifdef ENABLE_PS4BT
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
    #endif

    return 0;
}

//Parse analog stick requests for each type of controller.
int16_t getAnalogHat(AnalogHatEnum a)
{
#ifdef ENABLE_XBOX360
    if (Xbox360Wired.XboxReceiverConnected)
    {
        int16_t val;
        val = Xbox360Wired.getAnalogHat(a);
        if (val == -32512) //8bitdo range fix
            val = -32768;
        return val;
    }
    #endif
    #ifdef ENABLE_XBOXBT
    if (Xbox.connected())
        return Xbox.getAnalogHat(a);
    #endif
    #ifdef ENABLE_XBOXONEUSB
    if (XBOXONE.XboxOneConnected)
    return XBOXONE.getAnalogHat(a);
    #endif
    #ifdef ENABLE_SWITCHBT
   if (SwitchPro.connected()) {
        
		if (a == RightHatY || a == LeftHatY) {
			return (SwitchPro.getAnalogHat(a) - 127) * -20;
		} else {
			return (SwitchPro.getAnalogHat(a) - 127) * 20;
		}
    }
    #endif
    
	#ifdef ENABLE_PS4BT
    if (PS4Wired.connected()) {
		if (a == RightHatY || a == LeftHatY) {
			return (PS4Wired.getAnalogHat(a) - 127) * -255;
		} else {
			return (PS4Wired.getAnalogHat(a) - 127) * 255;
		}
	}
    #endif
    return 0;
}

#ifdef ENABLE_RUMBLE
//Parse rumble activation requests for each type of controller.
void setRumbleOn(uint8_t lValue, uint8_t rValue)
{
    if (rumbleOn) {
        #ifdef ENABLE_XBOX360
        if (Xbox360Wired.XboxReceiverConnected)
        {
            Xbox360Wired.setRumbleOn(lValue, rValue); 
        }
        #endif
        #ifdef ENABLE_XBOXBT
        if (Xbox.connected())
        {
            //Xbox.setRumbleOn(lValue / 8, rValue / 8, lValue / 2, rValue / 2);
        }
        #endif
        #ifdef ENABLE_XBOXONEUSB
        if (XBOXONE.XboxOneConnected)
        XBOXONE.setRumbleOn(lValue / 8, rValue / 8, lValue / 2, rValue / 2);
        #endif

        #ifdef ENABLE_SWITCHBT
        if (SwitchPro.connected())
        {
            if (lValue == 0 && rValue == 0) {
            SwitchPro.setRumbleRight(false);
            SwitchPro.setRumbleLeft(false);
            } else {
            SwitchPro.setRumbleRight(true);
            SwitchPro.setRumbleLeft(true);
            }
        
        }
         #endif
         
        #ifdef ENABLE_PS4BT
        if (PS4Wired.connected())
        {   
            if (lValue == 0 && rValue == 0) {
                PS4Wired.setRumbleOff();
            } else {
                PS4Wired.setRumbleOn(RumbleLow);
            }
        }
        #endif
    }
}
#endif

//Parse LED activation requests for each type of controller.
void setLedOn(LEDEnum led)
{
    #ifdef ENABLE_XBOX360
    if (Xbox360Wired.XboxReceiverConnected)
        Xbox360Wired.setLedOn(led);  
    #endif
    
}


// TO DO - merge these two functions
uint8_t controllerConnected()
{
    uint8_t controllerType = 0;
    #ifdef ENABLE_XBOX360
    if (Xbox360Wired.XboxReceiverConnected)
       {
        controllerType =  1;
       }
    #endif
    #ifdef ENABLE_XBOXBT
    if (Xbox.connected())
       {
        controllerType =  2;
       }
    #endif
	#ifdef ENABLE_SWITCHBT
    if (SwitchPro.connected())
        controllerType =  3;
    #endif   
	#ifdef ENABLE_PS4BT
    if (PS4Wired.connected())
		controllerType =  4;
    #endif
    #ifdef ENABLE_XBOXONEUSB
    if (XBOXONE.XboxOneConnected)
		controllerType =  5;
    #endif

    return controllerType;
}

void checkControllerChange() {
    Usb.Task();
    uint8_t currentController = controllerConnected();
    if (currentController != controllerType) {
        controllerType = currentController;
    }
}
