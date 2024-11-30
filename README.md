# Yet another USB mouse and joystick to Amiga, Atari ST &amp; C64 Adapter

Or in short Yaumataca. This device is one of many, that are allowing to use modern USB input devices
on 80s style home computer controller ports. Being mechanical in nature, old gamepads and mouses
degrade over time and replacements using modern variants might be required.

This solution makes use of the Raspberry Pi Pico board (RP2040) which offers a dedicated USB host controller
and can be bought at a rather low price (5-8€ per unit).
Cheap NPN transistors are used for level shifting as the RP2040 is not 5V tolerant.

## How to use

The Yaumataca acts as both Joystick and Mouse. Plug both output ports into both controller ports of the target machine.
Plug a USB OTG Hub into the Micro-USB port of the Pico board. Insert USB gamepad and USB mouse.

As multiple modes are supported, the [user manual](doc/user_manual.md) provides more detail.

## Features
* Cheap to build
* Uses "off the shelf" hardware
* Multiple USB devices on 2 Atari type controller ports using a single unit
* Primary joystick on left port
* Primary mouse on right port
* Amiga mouse with wheel using [WheelBusMouse](http://aminet.net/package/util/mouse/WheelBusMouse) driver
* Commodore 1351 in proportional mode
* Atari ST mouse
* Automatic switch between mouses and joysticks
* Swap of controller ports (useful for C64 games)
* Supports 2 mouses and 2 joysticks (useful for Lemmings and Marble Madness)
* Supports secondary fire button (Amiga and C64 style)
* Auto fire
* Configured mouse type is saved in flash

## Restrictions
* Only dedicated Joysticks are supported.
	* PS3 DualShock
	* PS4 DualShock
	* Nintendo Switch Pro Controller
	* Xbox One Controller
	* Xbox Series S/X Controller
	* Mega World International - USB Game Controllers (07b5:0314)

## TODO
* Add mouse wheel for C64
* Add 3. Fire button for more gamepads
* HID Descriptor Parsing (generic USB joystick support, eventually)
* Bluetooth (eventually)
* Explain C1351 calibration using C64 tool

## How to construct

The project is constructable via perfboard or PCB.

[Manual for construction on a perfboard](doc/construction_perfboard.md)

For the PCB, please use the [KiCad files](schematic/yaumataca/).
If you order your parts from [Reichelt](https//reichelt.de), you may use the
attached [ordering lists](doc/reichelt.csv) which can be imported on the website.

In case the PCB is chosen, there is also a [3D model of a case](housing/) available, which can be used for 3D printing.

To operate the unit, one OTG Hub is recommended which provides multiple USB A ports attached to a USB micro port.
If only one USB device is to be attached, a simple USB micro to USB A adapter will also work, but I suggest to go for the Hub.

### Example hardware design

Here an example of the PCB inside the case.
![Photo of Yaumataca in 3d printed case](doc/housing_v1.jpg)

Here the PCB side by side with the prototype.
![Photo of Yaumataca build from PCB and perfboard](doc/pcb_vs_perfboard.jpg)

## How to build the firmware

Ensure that the submodules are also cloned.

	git submodule update --init --recursive
	mkdir build
	cd build
	cmake ..
	make

Alternatively there is also a small script which builds and packages the software as a zip file for upload.

	./scripts/build_release.sh

## How to flash

### Integrated USB bootloader

The RP2040 can be flashed using SWD or using the integrated USB bootloader.
As not everyone might have a debugger lying around, flash the Pico as so:

* Don't have the Pico board powered and connected.
* Press and hold the BOOTSEL button
* Connect the Pico to your PC while holding the button
* The Pico should be detected as mass storage device
* Copy `yaumataca.uf2` from the `build` folder to the device
* The LED of the Pico should start blinking

The Yaumataca is now ready for operation

### With SWD

This assumes that you are using a CMSIS-DAP compatible programmer. It is suggested to use a second Pico board as one. This is possible using [the Picoprobe firmware](https://github.com/raspberrypi/picoprobe/releases).
Keep in mind that an st-link cannot be used with the RP2040, even so both speak SWD.

	openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 20000; program build/yaumataca.elf verify reset exit"

## Recommended IDE

Please use vscode and open the root folder with it.
This gives access to the already existing debugging targets.

## System architecture

![uml diagram](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Slamy/Yaumataca/develop/doc/pipeline.plantuml)

## Doxygen and Unittests

It is my goal to have everything properly documented and tested

	scripts/check_doxygen.sh
	scripts/unittest.sh

## FAQ

### I can see PRINTF all over the place. Where is the output going to?

This project makes use of "Segger RTT" which stores printed characters
inside a small buffer which can be displayed over SWD.
It has advantages compared to the UART as no additional GPIOs
have to be reserved for this.

I don't recommend to use rtthost as it is slow. Please use VSCode instead.

	rtthost -c rp2040

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

