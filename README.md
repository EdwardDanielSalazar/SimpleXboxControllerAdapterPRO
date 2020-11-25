# Simple Xbox Controller Adapter - Overview

## Purpose

This is a 'work in progress' adaptation of Ryzee119's excellent four-way wired/wireless controller adapter (https://github.com/Ryzee119/ogx360/) to allow the use of Xbox One and Xbox 360 controllers on an original Xbox console. 

The purpose is to allow for a simpler build, using only off-the-shelf parts, ideally an Arduino Leonardo/Pro Micro paired with a generic USB Host Shield. Soldering will be limited to connecting pads on the USB Host Shield, which may not be necessary at all depending on the manufacturer/model used. 

It will be wired-only, not offer Steel Batallion support and eventually (I hope) allow for use of a wider range of controllers. 

It also has full instructions for building/burning which aren't dependent on Windows, Atmel Studio or other things which are closed source/specialist subjects in their own right. A significant factor in basing this project on ogx360 rather than XBOXPadMicro/XInput was that it was possible (for me) to get everything up and running in VS Code/Linux. As I update this doc, a proper explantion will follow as to why none of this is as simple as loading a project into the Arduino IDE (on any OS) and uploading it in the normal way. In short, any project uploaded via the IDE will always make the Arduino act like a serial device and we need it to act like a HID device. 

## Alternatives / Comparisons

As above, this is based on Ryzee119's ogx360 project. This uses up to four Arduino Pro Micros to provide up to four emulated original Xbox controllers, connected via a custom PCB to a MAX3421 USB Host Controller IC (the same IC used in Arduino USB Host Shields). A big motivation for this project is that the MAX3421 is not (to the very best of my knowledge) available in a through-hole package making it tough work for a hobby project. Some people laugh in the face of surface mount soldering; I am not amongst them. 

Two asides: 
* I'd emotionally committed to this before I realised the MAX3421 is used in all Arduino USB Host Shields and therefore the original ogc360 code could be made to work without the custom PCB (see below). So there's a huge shortcut available if you just want the quickest and easiest way to use a more modern controller on an original Xbox.
* Ultimately, I'd like something which doesn't need an Arduino shield and have purchased several PIC24FJ64GB002 ICs, which have integrated USB, to play around with. I'll update this if I get anywhere.

There are several other projects it's worth taking a look at:
* While helping me with a question on the source, Ryzee119 pointed me to this, https://github.com/Ryzee119/ogx360_t4, which pretty much meets the aims of this project (simpler, no soldering, other controller support) but relies on the Teensy 4.1 (https://www.pjrc.com/store/teensy41.html). This works with the Arduino IDE, PlatformIO etc but is a more expensive option, reflecting the fact the board has an integral USB host, SD card, ethernet support etc and a 32bit processor. But very straightforward.
* The XBOXPadMicro project https://github.com/bootsector/XBOXPadMicro allows for a customised controller to be built to work with a classic Xbox Console, using an Arduino's digital input pins but doesn't include any USB host functionality (so you can't plug another controller into it). It's written in C and needs some work to adapt it for use with the broader Arduino ecosystem (which provides the easiest way to code USB host support via a shield/MAX3421).
* The Arduino XInput library (https://github.com/dmadison/ArduinoXInput) has everything needed to build a project to make an Arduino appear as an Xinput controller (the protocol used by the Xbox, successor consoles and Windows) except the vendor-specific HID challenge/response code needed to work with an actual Xbox console. So it will work with Windows/Linux/Mac but not the console itself.
* This project and both of Ryzee119's use the Arduino USB Host Shield library to handle communication between the device and the Xbox One/360 controller. This can be found here: https://github.com/felis/USB_Host_Shield_2.0. The speed with which you can pick up an Arduino, a shield and have a PS4 (or other) controller talking to Linux/Windows/Mac via serial is very impressive. It's well supported and very comprehensive.

## Quickest and easiest way to get an Xbox One/360 pad working with an original Xbox right now

The aim of this project is to make this foolproof including full instructions but, for now...

* Buy an Arduino Leonardo and USB Host Shield. Make sure the Shield is set up for 5V on the bus (it may be already) and plug them together.
* Download 'ogx360_32u4_master.hex' from https://github.com/Ryzee119/ogx360/releases
* Burn the hex to the Leonardo using the method under 'Programming' here: https://github.com/Ryzee119/ogx360/releases (this is for Windows, Linux/Mac instructions will be added to this document in due course, when I make my first release). You can't just use the normal Arduino IDE method but don't be put off. It's easy and no special hardware is needed.

There are variations on this (a Pro Micro can stand in for a Leonardo, you can use a shield designed for the Pro Mini etc) but this is the simplest.

# Getting Started

There isn't a release yet though using the instructions below to build commit 9d2a5b2 works (tested in Unreal 2, on an original Xbox, with an Xbox One controller).

These instructions are for Ubuntu. Others will follow but I've started here because it's what I use and it's easy (and free) to setup an Ubuntu virtual machine on Windows/Mac if that's what you're working with.

Further explanation as to why you have to do all this rather than just use the Arduino IDE/PlatformIO will also follow. 

What you'll need:
* An Ubuntu installation
* An Arduino Leonardo/Pro Micro (explanation to follow on why not an Uno/Mini/other) and USB cable to connect it
* An Arduino USB Host Shield, connected to the Arduino (...you guessed it...details to follow)

Assuming you have Ubuntu up and running:
* From a command prompt run: 
```
sudo apt-get install avrdude gcc-avr gdb-avr binutils-avr avr-libc git build-essential cmake xinput
``` 
If you're using a fresh install of Ubuntu or one with which you previously haven't done any development you'll probably see a lot of dependencies installed, but these should be handled automatically.
* Download and install Visual Studio Code here: https://code.visualstudio.com/download
* Open VS Code, click on the 'extensions' icon on the left hand side and install the 'C/C++' and 'CMake Tools' extensions.
* Close VS Code.
* Clone this project to an appropriate location with 'git clone https://github.com/jimnarey/SimpleXboxControllerAdapter'
* Open VS Code again and select 'File -> Open Folder'. Point it to the cloned repo.
* The CMake Tools extension should now create a 'build' directory within the 'Firmware' directory. Click 'yes' if it asks for permission to do this.
* Open main.cpp. If there are red squiggles under any of the header file/other names (meaning VS Code doesn't know where the files are located), open the VS Code command palette with Ctrl-Shift-P and type 'edit'. An option entitled 'C/C++ Edit Configurations UI' should appear. Select it. Scroll down to the 'Include Path' section and in the box below add the path to the AVR libraries. On Ubuntu 20 this is '/usr/lib/avr/include' but running 'dpkg -L avr-libc | grep include' will give you the right path on any Debian-based system.
* You should now have everything up and running to play around with the code and build it.

To build:
* Enter the 'build' directory created by CMake Tools.
* Type `make` to build the project. It will create .elf file, containing the firmware to be burned onto an Arduino Leonardo or Pro Micro but not yet in the correct format. As things stand this is called 'ogx360_32u4_master.elf'.
* Enter 
```
avr-objcopy -O binary ogx360_32u4_master.elf ./ogx360_32u4_master.bin
```
to create file we can burn to the Arduino
* Open another terminal window and enter 'dmesg -wH' to provide dmesg output on a rolling basis.
* In the original terminal window (which you just used to run avr-objcopy) enter the following command, but don't hit 'Enter': 
```
avrdude -p atmega32u4 -P /dev/ttyACM0 -c avr109 -U flash:w:ogx360_32u4_master.bin -C /etc/avrdude.conf'
```
* Plug in the Arduino as you would normally. You'll see some dmesg output about it being connected. 
* Press the reset button on the Arduino or if using a Pro Micro, short the RST and GND pins twice in immediate sucession. When the dmesg output shows the Arduino as disconnected, then immediately reconnected, hit 'Enter' in the other window. If you get this wrong you'll need another Arduino or hardware programmer to fix it (search for burning Ardunio bootloaders using an Arduino as an ISP).

## Additional Bits

This section contains notes to self for future updating of the readme...


