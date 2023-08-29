# Yaumataca User Manual

The Yaumataca acts as both Joystick and Mouse. Plug both output ports into both controller ports of the target machine.
The device is intended for usage with the Commodore Amiga, C64 and Atari ST systems. Do not plug this device into an Amstrad CPC as the joystick ports work differently.

## Gamepad Layout

It is assumed that most gamepads nowadays have a layout similiar to a Super Nintendo Controller. From 1993 to 2023, this statement seems valid and unbroken. We use a PlayStation controller as reference here.

![Button layout](button_layout.svg)

## Startup behaviour

When the machine boots up, it will enumerate the attached USB devices.
Every found device will be acknowledged by a short flash of the user LED.

The first detected joystick will be attached to the left port.

The first detected mouse will be attached to the right port.

For the Amiga and the Atari ST this seems to be the best approach, as the controller ports have dedicated roles and most games will follow them.
For the C64 this is a different story as no norm exists on which controller port shall be used for which purpose.

## Automatic swapping of second joystick and mouse

With both the Amiga and the Atari ST a second joystick is attached to the mouse port and a second mouse has to be attached to the joystick port. This is cumbersome as constant replugging of devices is required.

The Yaumataca detects changes on the USB devices and automatically swaps between first mouse / second joystick and first joystick / second mouse.
This way you can have your primary joystick and primary mouse always attached to the system and just add additional devices.

## Swapping the controller ports

This is important for the C64.
To trigger the controller port swap via an attached gamepad, press and hold the SELECT button. The user LED of the Pico board will flash, indicating that the change was performed.
To trigger the swap via mouse, press and hold all 3 mouse buttons until the LED is flashing.

## Changing the type of emulated mouse

The device can act as an Amiga mouse, an Atari ST mouse and as a C1351 in proportional mode.
It should be noted that none of these systems are damaged by the wrong type of mouse. It just won't work.

To cycle through the types of mouses, press the user button of the Raspberry Pi Pico board. The user led will indicate all 3 variants via LED flashing patterns.

* 3x short -> Amiga
* short long short -> Atari ST
* 2x long -> C1351

The configuration is stored permanently and is not required to be performed everytime.
