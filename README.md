# Yet another USB mouse and joystick to Amiga, Atari ST &amp; C64 Adapter

Or in short Yaumataca. This device is one of many, that are allowing to use modern USB input devices
on 80s style home computer controller ports. Being mechanical in nature, old gamepads and mouses
degrade over time and replacements using modern variants might be required.

This solution makes use of the Raspberry Pi Pico board (RP2040) which offers a dedicated USB host controller
and can be bought at a rather low price (5-8€ per unit).
Cheap NPN transistors are used for level shifting as the RP2040 is not 5V tolerant.

## Example hardware design

Here an example, how it could be build using BC547 transistors and 10k resistors.
To operate the unit, one OTG Hub is recommended which provides multiple USB A ports attached to a USB micro port.
If only one USB device is to be attached, a simple USB micro to USB A adapter will also work, but I suggest to go for the Hub.

![Photo of Yaumataca](doc/20230731_223820.jpg)
![Photo of Yaumataca from other angle](doc/20230731_223837.jpg)

## How to use

The Yaumataca acts as both Joystick and Mouse. Plug both output ports into both controller ports of the target machine.
Plug a USB OTG Hub into the Micro-USB port of the Pico board. Insert USB gamepad and USB mouse.

## How to build the firmware

Ensure that the submodules are also cloned.

	git submodule update --init --recursive
	mkdir build
	cd build
	cmake ..
	make

## How to flash

The RP2040 can be flashed using SWD or using the integrated USB Bootloader.
As not everyone might have a debugger lying around, flash the Pico as so:

* Don't have the Pico board powered and connected.
* Press and hold the BOOTSEL button
* Connect the Pico to your PC while holding the button
* The Pico should be detected as mass storage device
* Copy `yaumataca.uf2` from the `build` folder to the device
* The LED of the Pico should start blinking

The Yaumataca is now ready for operation

## Features
* Cheap to build
* Uses "off the shelf" hardware
* Multiple USB devices on 2 Atari type controller ports using a single unit
* Joystick on left port
* Amiga Mouse on right port
* Only dedicated HID Joysticks are supported.

## TODO
* Auto Fire
* PS3 Dual Shock
* Emulation of Commodore 1351 Proportional Mode
* Support for multiple mouses (useful for Lemmings and Marble Madness)
* Support for mouse wheel
* Support for multiple joysticks
* Support for Atari ST
* Automatic switch between mouses and joysticks
* Swap of Joysticks (useful for C64 games)
* HID Descriptor Parsing (generic USB joystick support)
* Bluetooth (eventually)

## FAQ

### Why is this not written in Rust?

Because USB Host Support for the RP2040 is not yet awesome

### Isn't this yet another USB mouse and joystick to Amiga, Atari ST & C64 adapter

Exactly

## Sources of Inspiration

My old project this here needs to surpass:<br>
http://slamyslab.blogspot.com/2013/02/pcbs-for-usb-datasette-and.html

Infos about Mouse Wheels on the Amiga:<br>
http://bax.comlab.uni-rostock.de/en/hardware/amiga-usb-mouse/

For the pinout:<br>
https://www.c64-wiki.de/wiki/Maus

Information about the Commodore 1351:<br>
http://sensi.org/~svo/%5Bm%5Douse/

