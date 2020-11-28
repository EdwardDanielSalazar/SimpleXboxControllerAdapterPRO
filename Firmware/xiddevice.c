/*
LUFA Library
Copyright (C) Dean Camera, 2013.

dean [at] fourwalledcubicle [dot] com
www.lufa-lib.org

Modified to XID device emulation by Ryzee119
*/

/*
Permission to use, copy, modify, distribute, and sell this
software and its documentation for any purpose is hereby granted
without fee, provided that the above copyright notice appear in
all copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of the author not be used in
advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

The author disclaims all warranties with regard to this
software, including all implied warranties of merchantability
and fitness.  In no event shall the author be liable for any
special, indirect or consequential damages or any damages
whatsoever resulting from loss of use, data or profits, whether
in an action of contract, negligence or other tortious action,
arising out of or in connection with the use or performance of
this software.
*/

#include "settings.h"
#include "xiddevice.h"
#include "dukecontroller.h"

extern bool enumerationComplete;
extern uint8_t ConnectedXID;

USB_XboxGamepad_Data_t PrevDukeHIDReportBuffer;

/** LUFA HID Class driver interface configuration and state information. This structure is
passed to all HID Class driver functions, so that multiple instances of the same class
within a device can be differentiated from one another.
*/
USB_ClassInfo_HID_Device_t DukeController_HID_Interface = {
	.Config = {
		.InterfaceNumber          = 0x00,
		.ReportINEndpoint         =	{
			.Address              = 0x81,
			.Size                 = 20,
			.Banks                = 1,
		},
		.PrevReportINBuffer           = &PrevDukeHIDReportBuffer,
		.PrevReportINBufferSize       = sizeof(PrevDukeHIDReportBuffer),
	},
};

/** Configures the board hardware and chip peripherals */
void SetupHardware(void){
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
	clock_prescale_set(clock_div_1);
	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void){

}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void){
	enumerationComplete=false;
	digitalWrite(ARDUINO_LED_PIN, HIGH);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void){
	bool ConfigSuccess = true;
	// switch (ConnectedXID){
	// 	case DUKE_CONTROLLER:
		ConfigSuccess &= HID_Device_ConfigureEndpoints(&DukeController_HID_Interface);
		ConfigSuccess &= Endpoint_ConfigureEndpoint(0x02, EP_TYPE_INTERRUPT, 6, 1); //Host Out endpoint opened manually for Duke.
		// break;
	// }
	USB_Device_EnableSOFEvents();
	enumerationComplete=ConfigSuccess;
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void){
	//The Xbox Controller is a HID device, however it has some custom vendor requests
	//These are caught and processed here before going into the standard HID driver.
	//These are required for the controller to actually work on the console. Some games are more picky than others.
	//See http://xboxdevwiki.net/Xbox_Input_Devices under GET_DESCRIPTOR and GET_CAPABILITIES
	//The actual responses were obtained from a USB analyser when communicating with an OG Xbox console.

	if (USB_ControlRequest.bmRequestType == 0xC1){
			if (USB_ControlRequest.bRequest == 0x06 && USB_ControlRequest.wValue == 0x4200) {
				Endpoint_ClearSETUP();
				// switch (ConnectedXID){
				// 	case DUKE_CONTROLLER:
					Endpoint_Write_Control_Stream_LE(&DUKE_HID_DESCRIPTOR_XID, 16);
					// break;
				// }
				Endpoint_ClearOUT();
				return;
			}
			else if (USB_ControlRequest.bRequest == 0x01 && USB_ControlRequest.wValue == 0x0100) {
				// Endpoint_ClearSETUP();
				// switch (ConnectedXID){
					// case DUKE_CONTROLLER:
					Endpoint_Write_Control_Stream_LE(&DUKE_HID_CAPABILITIES_IN, 20);
					// break;
				// }

				Endpoint_ClearOUT();
				return;
			}
			else if (USB_ControlRequest.bRequest == 0x01 && USB_ControlRequest.wValue == 0x0200) {
				Endpoint_ClearSETUP();
				// switch (ConnectedXID){
				// 	case DUKE_CONTROLLER:
					Endpoint_Write_Control_Stream_LE(&DUKE_HID_CAPABILITIES_OUT, 6);
					// break;
				// }
				Endpoint_ClearOUT();
				return;
			}
	}

	//If the request is a standard HID control request, jump into the LUFA library to handle it for us.
	// switch (ConnectedXID){
	// 	case DUKE_CONTROLLER:
		HID_Device_ProcessControlRequest(&DukeController_HID_Interface);
	// 	break;
	// }

}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void){
	// switch (ConnectedXID){
	// 	case DUKE_CONTROLLER:
		HID_Device_MillisecondElapsed(&DukeController_HID_Interface);
	// 	break;
	// }


}

// HID class driver callback function for the creation of HID reports to the host.
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
													uint8_t* const ReportID, const uint8_t ReportType,
													void* ReportData,	uint16_t* const ReportSize){

	USB_XboxGamepad_Data_t* DukeReport = (USB_XboxGamepad_Data_t*)ReportData;

	// switch (ConnectedXID){
	// 	case DUKE_CONTROLLER:
		DukeReport->startByte = 0x00;
		DukeReport->bLength = 20;
		DukeReport->dButtons = XboxOGDuke.dButtons;
		DukeReport->reserved = 0x00;
		DukeReport->A = XboxOGDuke.A;
		DukeReport->B = XboxOGDuke.B;
		DukeReport->X = XboxOGDuke.X;
		DukeReport->Y = XboxOGDuke.Y;
		DukeReport->BLACK = XboxOGDuke.BLACK;
		DukeReport->WHITE = XboxOGDuke.WHITE;
		DukeReport->L = XboxOGDuke.L;
		DukeReport->R = XboxOGDuke.R;
		DukeReport->leftStickX = XboxOGDuke.leftStickX;
		DukeReport->leftStickY = XboxOGDuke.leftStickY;
		DukeReport->rightStickX = XboxOGDuke.rightStickX;
		DukeReport->rightStickY = XboxOGDuke.rightStickY;
		*ReportSize = DukeReport->bLength;
	// 	break;
	// }

	return false;
}


/* HID class driver callback for the user processing of a received HID OUT report. This callback may fire in response to
*  either HID class control requests from the host, or by the normal HID endpoint polling procedure. Inside this callback
*  the user is responsible for the processing of the received HID output report from the host.*/
void CALLBACK_HID_Device_ProcessHIDReport(
	USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
	const uint8_t ReportID,	const uint8_t ReportType,
	const void* ReportData,	const uint16_t ReportSize){
	//Only expect one HID report from the host and this is the actuator levels. The command is always 6 bytes long.
	//bit 3 is the left actuator value, bit 5 is the right actuator level.
	//See http://euc.jp/periphs/xbox-controller.en.html - Output Report
	if (ConnectedXID == DUKE_CONTROLLER && ReportSize == 0x06) {
		XboxOGDuke.left_actuator =  ((uint8_t *)ReportData)[3];
		XboxOGDuke.right_actuator = ((uint8_t *)ReportData)[5];
		XboxOGDuke.rumbleUpdate = 1;
	}

}

//USB callback function for the processing of the device descriptors from the device.
//The standard HID descriptor requests are handled here.
uint16_t CALLBACK_USB_GetDescriptor(
	const uint16_t wValue, const uint16_t wIndex,
	const void** const DescriptorAddress) {

	const uint8_t  DescriptorType   = (wValue >> 8);
	uint8_t nullString[1] = {0};
	const void* Address = NULL;
	uint16_t    Size    = NO_DESCRIPTOR;

	switch (DescriptorType)
	{
		case DTYPE_Device:
		// switch (ConnectedXID){
		// 	case DUKE_CONTROLLER:
			Address = &DUKE_USB_DESCRIPTOR_DEVICE;
			Size    = 18;
		// 	break;
		// }
		break;
		case DTYPE_Configuration:

		// switch (ConnectedXID){
		// 	case DUKE_CONTROLLER:
			Address = &DUKE_USB_DESCRIPTOR_CONFIGURATION;
			Size    = 32;
		// 	break;
		// }
		break;
		case DTYPE_String:
		Address = &nullString; //OG Xbox controller doesn't use these.
		Size    = NO_DESCRIPTOR;
		break;
	}
	*DescriptorAddress = Address;
	return Size;
}
