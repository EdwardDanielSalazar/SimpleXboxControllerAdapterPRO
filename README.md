# Simple Xbox Controller Adapter - Overview

A WIP adaptation of https://github.com/Ryzee119/ogx360/ to make for a simpler build, ideally involving no soldering and off-the-shelf parts, ideally an Arduino Leonardo/Pro Micro paired with a generic USB Host Shield. As things stand, burning 'ogx360_32u4_master.hex' from https://github.com/Ryzee119/ogx360/releases onto a Leonardo with a properly configured USB Host Shield (bus set to 5V and 5V supply to the shield provided by soldering relevant pads) will allow you to use an Xbox One controller (tested) and 360 controller (not tested yet) on the classic Xbox.

See also https://github.com/Ryzee119/ogx360_t4 which achieves this already using the Teensy 4.1 board.

See https://github.com/bootsector/XBOXPadMicro for a solution which allows for a customised controller to be built to work with a classic Xbox Console, using an Arduino's digital-in pins but doesn't include any USB host functionality (so you can't plug another controller into it). In C, would need some work to adapt to the broader Arduino ecosystem.

See https://github.com/dmadison/ArduinoXInput, a well-supported library for emulating an Xbox(-like) controller on a PC but which would need some modification (addition of vendor-specific HID challenge/response) to work with an Xbox console.

Like the ogx360 variants, this uses (parts of) the comprehensive and well-supported Arduino USB Host Library, here: https://github.com/felis/USB_Host_Shield_2.0. 