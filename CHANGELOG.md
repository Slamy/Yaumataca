# Changelog

## 0.2

* Added support for "Xbox 360 Wireless Receiver" with either 1 or 2 controllers
  attached to a single receiver. (thanks to quadflyer8 for providing the hardware)
* Added Micromys wheel support for the C1351 mode
* Added generic HID parser for Mice.<br>
  Should fix support for all types of mice even without the boot mode workaround.
* Added "Final Cartridge III" workaround for fixing applications which are opening both muxes to the SID POT line to detect a mouse either on port 1 or 2. This breaks when 2 mice are attached at the same time. As joysticks with 2. and 3. fire button are using the POT lines, these are now disabled if a mouse is moved.
* Initial state after power cycle is now Mouse + Joystick even if no USB device is attached.<br>
  Fixes initial configuration of "Final Cartridge III"
* Added support for DragonInc Hizue pad (thanks to spacepilot3000 for providing hardware)
* If 1 device out of 3 is removed and the remaining 2 devices are sharing a single port, those two will be separated.
* Added additional VID/PID combos for already supported controllers
* Support for third fire button
* Altered build system to generate a zip file using a script.<br>
  Multiple different firmwares are now provided in binary form
    * Standard: Like intended by me
    * 2 button swap: For the tank mouse, when no third mouse button exists. Pressing left and right together for some time will swap the ports.
    * No WheelBusMouse: Deactivates one port affecting the other. Originally used for Amiga wheel support.