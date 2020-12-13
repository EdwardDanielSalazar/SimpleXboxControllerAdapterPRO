# Simple Xbox Controller Adapter

## Functionality

Following the instructions below and using the [latest release](https://github.com/jimnarey/SimpleXboxControllerAdapter/releases) of this project you can create a simple Arduino-based adapter to use Xbox 360, Xbox One, PS3 and PS4 controllers with an original Xbox console.

## Background

This is an adaptation of Ryzee119's excellent [four-way wired/wireless controller adapter](https://github.com/Ryzee119/ogx360/) to allow the use of Xbox One and Xbox 360 controllers (wired and wireless) on an original Xbox console. 

The objectives of this project are:
* The simplest possible hardware build, using only off-the-shelf parts (at present, an Arduino Leonardo paired with a generic USB Host Shield). 
* Simple soldering (through-hole/jumper pad only). *A no solder version is possible if using the right choice of USB Host Shield*. 
* Reduced, simpler code to make playing around with it as straightforward as possible.
* Reduced functionality, reflecting the simpler code. Only wired controllers are supported and Steel Battalion emulation is not. 
* Support for additional controllers (so far, PS3 & PS4 controller support has been added).
* To provide full, foolproof instructions to enable anyone who's broadly comfortable with an Arduino to make an adapter and anyone who is broadly comfortable with the command line and IDEs to edit and build the code. So this document is a key part of the project.

This started when I dusted off my old Xbox and wanted to make something to use newer controllers with it. There's plenty of previous work on this and as I develop this document I'll link to more and more of it.

### Status

Release 1.2 differs from ogx360 as follows:
* Support for Xbox One and Xbox 360 *wireless* controllers has been removed.
* Support for multiple controllers has been removed. It supports a single controller connected via USB.
* Steel Battalion support has been removed, resulting in a smaller binary (and space for more controllers to be supported).
* Rumble has been disabled via settings.h. Significant rumble caused the device to hang. Future versions will address this though it can probably handle rumble if an => 6v power supply is connected to the Leonardo's power connector.
* PS3 controller support (via USB) has been added. Rumble isn't implemented at all for the time being, as opposed to being disabled in the case of the Xbox One/360 controllers.
* PS4 controller support (via USB) has been added. As with the PS3 controller, rumble isn't implemented (for now) and nor is support for doing anything with the LED.

### Build System

I initially chose to base this project on ogx360 because of the various existing projects out there (see below) it was the easiest to build and extend using free/open-source tools (VSCode with the CMake extension installed) and the instructions below cover this. However, Ryzee119 recently transitioned ogx360 to VSCode/PlatformIO which makes things even easier and has some nice touches such as build-time stats on RAM/flash usage. It also means it uses the latest Arduino core, which has some [improvements to RAM usage](https://github.com/Ryzee119/ogx360/issues/41).

I'm in the process of transitioning to the new build system in the pio_master branch. This builds and works with support for all four controllers but there's some tidying up to do before I merge this into master.

All the releases to date are based on the master branch, using the old build system.

### Future Work

These are broadly the priorities for now, not necessarily in order:
* Add instructions for building with an Arduino Pro Micro and mini USB Host Shield. This won't be a no-solder option but will be more compact. 
* Add STL files for 3D printable cases for both hardware builds.
* Investigate power/rumble issue.
* Improving this document. The current build instructions refer to the old build system and need an overhaul (but still work with the current master branch).
* Adding some scripts to make flashing the binary easier.
* Possibly add some user input/output on the device itself via an I2C OLED screen. Not really in the spirit of keeping things simple, but fun.
* Possibly adding more controllers. The binary is now up to more than 26K and adding in anything involving bluetooth support puts it way over 32K. So I need to do a bit of digging to see what's possible here.

## Making One

For now, these instructions cover using a 'full size' Arduino Leonardo and USB Host Shield. You could get it working with an Arduino Micro, Pro Micro (set to run on 5V at 16Mhz) with a little adaptation and I'll add instructions for this in due course. Using a Leonardo is the simplest way, however.

### You will need

* An Arduino Leonardo/clone. Make sure it has the ICSP pin header (the separate, 6 pin header in the rear/centre). All those I could see on a quick skim of Amazon and eBay seemed to have one but I've seen references online to clones which do not.

![Arduino Leonardo with ICSP header highlighted](../media/re_leonardo_corner_icsp_highlighted.jpeg?raw=true)

* A standard size USB Host Shield (i.e. not one designed for a Pro Micro/Mini). Again, make sure this has the ICSP socket/header on the underside:

![Underside of host shield with ICSP header/plug](../media/re_usb_host_shield_icsp_highlighted_cropped.png?raw=true)

* If you'd prefer to avoid soldering altogether, also ensure your choice of shield comes with the power supply pads pre-soldered. Mine, made by ARCELI, did not but it's the work of minutes if you're comfortable with soldering iron. Amazon (in the UK, at least) has an unbranded alternative (for about twice the price!) with the pads pre-soldered. These are the three pads:

![Arduino USB host shield with power supply pads highlighted](../media/re_usb_host_shield_pads_highlighted.jpeg?raw=true)

* A USB-A female to Xbox Controller male/plug adapter. These are a few £/$ on eBay and [a better choice than hacking apart an Xbox peripheral](https://xboxdevwiki.net/Xbox_Input_Devices#USB_Adapters)
* A total of two micro USB cables. Most Leonardos come with one.
* (Optional) Another Arduino (Uno/Leonardo/Micro/Pro Micro) and some male to female jumper wires to re-flash the bootloader onto the Leonardo if something goes wrong when using Avrdude. If you follow the instructions and - in particular - are very careful to (re-)produce the Avrdude command correctly, it won't. You could of course use a dedicated AVR hardware programmer for this if you have one.

### Hardware build

* Check whether the pads need soldering on your USB Host Shield using the image above.
* If they're not, [this](https://learn.sparkfun.com/tutorials/how-to-work-with-jumper-pads-and-pcb-traces/all) is a decent tutorial on soldering jumper pads though there are several on YouTube which may be better if you're not confident with a soldering iron.
* Plug the USB Host Shield into the Leonardo.

### Flashing the firmware

Notes:
* This section covers the process for Ubuntu/Debian and will be expanded with more detail and MacOS instructions soon. I'll get round to Windows eventually but for now the process is well explained in [one of the ogx360 readmes](https://github.com/Ryzee119/ogx360/tree/master/Firmware).
* The flash script under /Firmware was written for my own convenience and may need adapting to your own setup - don't run it without checking. Use the method below.

We need to use Avrdude for this and we need to make sure that the bootloader/firmware section of the Arduino's program memory is flashed rather than the normal 'sketch' space addressed by the Arduino IDE. This is essential to enabling the device to appear as a HID(-like) device rather than a serial device. 

Generally when you flash code onto an Arduino with the IDE it appears to a connected host (e.g. a PC) as a serial device, as this is a feature of the pre-flashed bootloader. Some Leonardos come with a version of the bootloader which makes it appear as a keyboard/mouse device and there are plenty of Leonardo-based projects which make use of this. In any case, neither of these is what we need and the Arduino IDE can't help us.

No special hardware (e.g. a programmer) is needed.

* Open two terminal windows/panes
* In one command prompt type ```sudo apt-get update && sudo apt-get install avrdude```
* Get the [latest version of the binary](https://github.com/jimnarey/SimpleXboxControllerAdapter/releases) (sxbca.bin)
* In one terminal type in ```dmesg -wH```
* Plug in the Leonardo via USB and you should see some dmesg output about it. If the Leonardo has the standard bootloader installed it will appear as a composite keyboard and mouse. If it has the alternative Caterina bootloader installed it will appear as an Arduino device. It doesn't matter which.
* Note the serial port the Arduino is connected to. In my case it's always /dev/ttyACM0
* In the second terminal window, navigate to where the sxbca.bin file is stored and - *without pressing Enter* - type in the following line, changing the name of the serial port if needed: 
```
avrdude -p atmega32u4 -P /dev/ttyACM0 -c avr109 -U flash:w:sxbca.bin -C /etc/avrdude.conf'
```
* Press the reset button on the Leonardo and *when you see the dmesg output register a new USB device* hit 'Enter'. Avrdude will then flash the firmware. You should have eight seconds from pressing the reset button before the Arduino reverts to its normal state. When this happens, you'll see it again in dmesg. You can just press the reset button again if this happens.
* All done. Plug an Xbox One/360 controller into the USB Host Shield and the Leonardo into the Xbox console and everything should work.

Note - the correct command for MacOS is probably the following but this will be tested/confirmed soon.
```
avrdude -p atmega32u4 -P /dev/ttyACM0 -c avr109 -U flash:w:sxbca.bin -C /usr/local/Cellar/avrdude/6.3_1/etc/avrdude.conf'
```

### Reflashing the standard Arduino bootloader

If you want to get the Leonardo back to its stock state for whatever reason, just follow the same process as above but point Avrdude to the bootloader file which comes with the Arduino IDE. Where this is will depend on where the Arduino IDE is installed. Within the Arduino IDE folder on my (Ubuntu) system it was here:

```
XXXXX/arduino-1.8.12/hardware/arduino/avr/bootloaders/caterina/Leonardo-prod-firmware-2012-12-10.hex
```

My Arduino IDE folder contains an older version of the firmware (Leonardo-prod-firmware-2012-04-26.hex) - I'm not sure what the difference is. It also contains an alternative firmware - Caterina-Leonardo.hex - which when loaded on the Arduino makes it appear as an Arduino device rather than a keyboard/mouse.

This time, when you first plug the Arduino in it will appear in the dmesg output as a gamepad device. When you press the reset button it will appear again as an Arduino device.

## What to do if something goes wrong

Something 'going wrong' could mean:

* You didn't press the reset button before flashing the firmware with Avrdude, or did and then ran the Avrdude command after the device had reverted (after more than eight seconds).
* You made a typo in the Avrdude command. E.g. Avrdude erases the flash before looking for the hex/bin file you point it towards. So if you make a typo in 'sxbca.bin' it will cheerfully erase your Arduino and leave it at that. It's hard to imagine anyone being daft enough to do this and then getting into a mild panic when they connect the device to the Xbox, it doesn't work, and they assume it's a consequence of refactoring C++ at 3am but it could happen...

In these and other cases you won't be able to flash the firmware properly with Avrdude (it either doesn't run, or does run but the device doesn't work). You also won't be able to load sketches with the Arduino IDE. You'll therefore need either another Arduino to use as an ISP (In System Programmer) or a hardware programmer. If you own a hardware programmer you probably don't need any help with this.

### Reflashing the Leonardo with another Arduino configured as an ISP

You can use either an Uno or any Arduino based on the atmega32u4 (Leonardo, Micro, Pro Micro - probably others). [This page](https://www.arduino.cc/en/Tutorial/BuiltInExamples/ArduinoISP) has a comprehensive explantion of how to do this. *Although I refer below to soldering an ISP, you can make one perfecly well with jumper leads*

Recognising the likelihood that I'd screw up the flashing process at some point, before I started this project I soldered together a permanent ISP based on an Arduino Pro Micro clone, based on the instructions [here](https://www.instructables.com/Mini-Breadboard-Pro-Micro-ISP-for-DIP8-ATTiny-Seri/) (these actually cover doing it with a breadboard but the principles are the same).

The main takeaway from this tutorial is that when using a Pro Micro rather than an Uno you need to change the value of the RESET pin in the ISP sketch. Otherwise it's the pretty much the same. If you plan to play around with the code in this project or related ones (see below) making a permanent ISP is well worth doing. Using a Pro Micro and by soldering the LED resistors to the LED legs you can make something very compact:

![Arduino Pro Micro ISP](../media/re_pro_micro_isp.jpeg?raw=true)

## Alternatives & other projects to look at

* [Ryzee119's ogx360 project](https://github.com/Ryzee119/ogx360/), on which this is based.
    * Uses up to four Arduino Pro Micros to provide up to four emulated original Xbox controllers, connected via a custom PCB to a MAX3421 USB Host Controller IC (the same IC used in Arduino USB Host Shields).
    * Unfortunately, the MAX3421 is not (to the very best of my knowledge) available in a through-hole package making it tough work for a hobby project. Some people laugh in the face of surface mount soldering; I am not amongst them.
    * *Note:* You can actually flash the [ogx360 master binary](https://github.com/Ryzee119/ogx360/releases) to a Leonardo/Pro Micro following the instructions above and it just works with a wired controller. This doesn't seem to be documented in the otherwise comprehensive readmes. As this firmware has rumble enabled you may well find it hangs the device when rumble is activated for any length of time if you apply it to this hardware build rather than Ryzee119's.
* [Another Ryzee199 project](https://github.com/Ryzee119/ogx360_t4) pretty much meets the aims of this one (simple, no soldering, other controller support) but relies on the [Teensy 4.1](https://www.pjrc.com/store/teensy41.html). This works with the Arduino IDE, PlatformIO etc but is a more expensive option, reflecting the fact the board has an integral USB host, SD card, ethernet support etc. and a 32bit processor. But very straightforward.
* [The XBOXPadMicro project](https://github.com/bootsector/XBOXPadMicro) allows for a customised controller to be built to work with a classic Xbox Console, using an Arduino's digital input pins. 
    * Doesn't include any USB host functionality, so you can't plug another controller into it. It has everything you need to build/adapt a custom controller, however. I plan to use this to get a [TAC-2](https://en.wikipedia.org/wiki/TAC-2) working with PC/Xbox. Nobody needs more than one button, as the Amiga port of Street Fighter 2 demonstrated...
    * It's written in C and would need some work to adapt it for use with the broader Arduino ecosystem, allowing it to be adapted for use with USB controllers.
* [The Arduino XInput library](https://github.com/dmadison/ArduinoXInput) has everything needed to build a project to make an Arduino appear as an Xinput controller (the protocol used by the Xbox, successor consoles and Windows) except the vendor-specific HID challenge/response code needed to work with an actual Xbox console. So it will work with Windows/Linux/Mac but not the console itself.
* This project and both of Ryzee119's use the [Arduino USB Host Shield Library](https://github.com/felis/USB_Host_Shield_2.0) to handle communication between the device and the Xbox One/360 controller. The speed with which you can pick up an Arduino, a shield and have a PS4/Xbox One/360/etc. controller talking to Linux/Windows/Mac via serial is really impressive. It's well supported and has support for a wide range of USB controllers, including great example sketches.

# Development/Building the Binary

This section of the document is still at an early stage...more/better to follow.

## Install Command Line Tools

### Ubuntu etc.

* From a command prompt run: 
```
sudo apt-get install avrdude gcc-avr gdb-avr binutils-avr avr-libc git build-essential cmake
```

### MacOS
* From a command prompt run:
```
xcode-select —-install
brew tap osx-cross/avr
brew install avr-gcc avrdude cmake
```
## Install VS Code

The project uses CMake to build itself from the three libraries (Arduino, LUFA, USB Host Shield) and project code. VS Code's CMake extension can run the right CMake tasks automatically every time the project is opened.

* Download and install Visual Studio Code here: https://code.visualstudio.com/download
* Open VS Code, click on the 'extensions' icon on the left hand side and install the 'C/C++' and 'CMake Tools' extensions.
* Close VS Code.
* Clone this project to an appropriate location with 'git clone https://github.com/jimnarey/SimpleXboxControllerAdapter'
* Open VS Code again and select 'File -> Open Folder'. Point it to the cloned repo.
* If asked about which compiler to use, via a drop-down box, select gcc-avr
* The CMake Tools extension should now create a 'build' directory within the 'Firmware' directory. Click 'yes' if it asks for permission to do this and click 'yes' if it asks about running Cmake each time a project/folder is opened.
* Open main.cpp. If there are red squiggles under any of the header file/other names (meaning VS Code doesn't know where the files are located):
    * Open the VS Code command palette with Ctrl-Shift-P and type 'edit'. 
    * An option entitled 'C/C++ Edit Configurations UI' should appear. Select it. 
    * Scroll down to the 'Include Path' section and in the box below add the path to the AVR libraries. 
        * On Ubuntu 20 this is '/usr/lib/avr/include' but running ```dpkg -L avr-libc | grep include``` will give you the right path on any Debian-based system.
        * On MacOS, this will depend on the version of the AVR compiler installed by brew. On Catalina and avr-gcc 9.3.0, the correct path is '/usr/local/Cellar/avr-gcc/9.3.0/avr/include/'.
* You should now have everything up and running to play around with the code and build it.

## Build

* Enter the 'build' directory created by VS Code/CMake Tools.
* Type `make` to build the project. It will create .elf file, containing the firmware to be burned onto an Arduino Leonardo or Pro Micro but not yet in the correct format.
* MacOS/Brew, which uses a newer version of gcc-avr than Ubuntu, will likely throw some warnings when compiling the LUFA library. These can be ignored.
* Enter: 
```
avr-objcopy -O binary sxbca.elf ./sxbca.bin
```

## Develop/Extend

When adding PS3 controller support I used the Leonardo's second UART/Serial port ('Serial1' in the Arduino library) for debugging. This can be accessed on pins 20 and 21. You'll need an inexpensive USB to serial module to connect it to a PC/Mac. You can then open the Arduino IDE, point it to the correct serial port (in my case /dev/ttyUSB0) and start the Serial Monitor. On Ubuntu (at least) using 'screen' is more convenient - install with ```sudo apt-get install screen```.

The lines for initialising Serial1 are in main.cpp and commented out in most commits. Assuming you uncomment these and don't change the baud from 9600, you'd then start screen with the following, amending the name of the serial port if necessary:

```
screen /dev/ttyUSB0 9600
```

As noted above, there isn't a huge amount of space left on the ATmega32u4's flash with the current build so it may be necessary to remove support for one or more controllers in order to add in support for others.

### Steps for adding additional controllers

Adding PS3 and PS4 controller support involved the following:
* Adding an #include for the PS3USB/PS4USB library
* Instantiating the PS3USB/PS4USB class using the same format as the existing Xbox controller classes (pass it a pointer to the 'USBHost' object)
* Adding a check for whether a PS3/PS4 controller is connected to controllerConnected()
* Reproducing the calls to the two Xbox controller classes in getButtonPress() and getAnalogHat()
* When only some buttons worked, using the output from Serial1 (via the USB to serial module) to see what was coming back from the controller by adding temporary code to the main loop, e.g.
```
if (PS3Wired.PS3Connected) {
    Serial1.println(PS3Wired.getButtonPress(R2));
}
```
* Adding lines like this for one button at a time makes it much easier to make sense of the serial output
* Making necessary changes to the values returned from the controller. For example, the Xbox expects a value from each axis on each analog stick between -32768 and 32767 - so a signed 16bit int. The PS3 and PS4 controller each provides an unsigned 8bit int instead: 0 - 255. The lines of code which deal with this are commented. The face buttons also needed remapping to their physical counterparts on the Xbox controller.

Thanks to Ryzee119 for helping resolve some (to me) incomprehensible linker errors when trying to import the PS4 library for the first time.

I have since tested importing and instantiating the classes for Bluetooth, Wii controllers and general HID controllers so making use of these should be possible without any additional dependencies being used. However, all of these together with the current functionality resulted in a 43K binary, so they can't all be used together.




