# Adding a new HID gamepad

## Prerequisites

If you don't feel confident with programming in C, stop reading now to avoid unnecessary pain.

You need some basic knowledge about C++, CMake and cross compilation.
It would also be a good thing to have some knowledge about "Segger RTT" as this is the primary method for debugging output.

You need a SWD debugger which is compatible with the RP Pico. If unsure, get another
RP Pico and use [this firmware](https://github.com/raspberrypi/picoprobe/) to build a very cheap CMSIS-DAP debugger.
Ensure it is the picoprobe with CMSIS support.

I recommend VS Code as IDE for development and debugging.

## Enabling Debugging messages

Edit the CMake Cache and enable `CONFIG_DEBUG_PRINT` to activate RTT output.
If you run the application inside VS Code, it will activate the RTT console and you can see your output.

If you don't want to use the VSCode internal RTT Console, you can use rtthost too:

    rtthost -c rp2040

Or pyocd:

	pyocd rtt --target rp2040

A possible output might be this:

    Yaumataca says hello!
    JoystickMouseSwitcher +
    JoystickMouseSwitcher +
    Found configuration byte 0x2 at offset 0x19
    Pipeline +
    GPIOs set for joystick mode on left port
    GPIOs set for joystick mode on right port
    PortSwitcher + 20005040
    PortSwitcher + 20005070
    MouseModeSwitcher +
    MouseModeSwitcher +
    GamePadFeatures +
    GamePadFeatures +
    Set mouse mode 2
    C1351Converter +
    Set mouse mode 2
    C1351Converter +
    C1351 calibration previously stored. Use it!
    C1351 calibration -1546 -1301 -1406 -1164
    C1351 calibration -1555 -1316 -1412 -1177
    R 0000 011
    L 0000 011

## Checking for standard HID reports

You need to know the VID and PID of the pad to continue.
Also, we would like to know whether the gamepad is transmitting some report data.
With the debugging output at hand, connect a gamepad:

    HID device address = 1, instance = 0 is mounted
    VID = 046d, PID = c20c
    HID has 1 reports 0 4 1
    Device attached, address = 1
    HID device address = 1 is mounted
    VID = 046d, PID = c20c

This is good! We now have knowledge about the VID and the PID.
Press some buttons on your gamepad now.

    Report: 00 80 80
    Report: 04 80 80
    Report: 00 80 80
    Report: 04 80 80
    Report: 00 80 80
    Report: 01 80 80
    Report: 03 80 80

This is a good thing! This is an indication that your gamepad is providing reports of some sort without requesting
it to do so via non-standard methods.
The output you see is a hex representation of the HID report. Bits are flipping when you press buttons.

## Starting with a template

Make a duplicate of [this rather simple and usable handler](../src/handlers/hid_impact.cpp).

`hid_impact.cpp` is for a rather old gamepad, I had around when this project was started.
It might make sense to use this as inspiration.

Rename the class `ImpactHidHandler` to something which makes sense for your device.
This gamepad has "impact" written on it, so I thought it made sense to call it that.

Look for the `HidHandlerBuilder` at the bottom of the file and replace the VID and PID.

Don't forget to add this file to the [CMakeLists.txt](../CMakeLists.txt) next to the other handlers.

The class has a method which needs to be implemented. It is called on every report, the input delivers to us:

```c++
void process_report(std::span<const uint8_t> d) {
}
```

It makes sense to start with some debugging messages to analyze the reports for their structure.
Do you have activated the PRINTF inside your handler? Something like this?

    PRINTF("XYZ: ");
    for (uint8_t i : d) {
        PRINTF(" %02x", i);
    }
    PRINTF("\n");

If yes, press some buttons! What does happen?
Something like this?

    XYZ:  80 80 80 80 05 00
    XYZ:  80 80 80 80 06 00
    XYZ:  80 80 80 80 07 00
    XYZ:  80 80 80 80 00 00
    XYZ:  80 80 80 80 03 00

## Mapping the buttons of the gamepad

What to do with that data? Let's use `hid_impact.cpp` as an example.
Every line in the `Report` structure marks a set of bits, giving a name to them which we can later use for parsing.

```c++
struct __attribute__((packed)) Report {
    // Byte 0 to 3
    uint8_t joy_left_x;
    uint8_t joy_left_y;
    uint8_t joy_right_y;
    uint8_t joy_right_x;

    // Byte 4
    uint8_t coolie_hat : 4;
    uint8_t button_left : 1;
    uint8_t button_top : 1;
    uint8_t button_bottom : 1;
    uint8_t button_right : 1;

    // Byte 5
    uint8_t trigger_l1 : 1;
    uint8_t trigger_l2 : 1;
    uint8_t trigger_r1 : 1;
    uint8_t trigger_r2 : 1;
    uint8_t button_select : 1;
    uint8_t button_start : 1;
    uint8_t stick_click_left : 1;
    uint8_t stick_click_right : 1;
};
```

With this controller, the first 4 bytes are an unsigned representation of both analog sticks with 0x80 being the center state.

By moving the left stick, the software will print this:

    Impact: 0 0000 80 81 80 80 00 00
    Impact: 0 0000 80 81 80 80 00 00
    Impact: 0 0000 74 ff 80 80 00 00
    Impact: 0 0000 6e af 80 80 00 00
    Impact: 0 0000 80 80 80 80 00 00

By clicking the left button on the front 4 buttons (would be square on PS1 controller) I get this

    Impact: 0 1000 80 80 80 80 10 00
    Impact: 0 0000 80 80 80 80 00 00

Now - by observing the change - I know the bit position of this specific button!

Keep in mind that some gamepads don't implement the D-Pad as 4 buttons but instead as a Coolie-Hat with one value for each direction.
`hid_impact.cpp` is one example for such a case.

There are also gamepads around which are implementing the D-Pad as virtual analog axes.
The DragonRise Controller `(0079:0011)` is one of those examples.

You now need to fill out
```c++
struct __attribute__((packed)) Report {
}
```

Get yourself some inspiration from the other handlers. Get creative!
The `GamepadReport` must be filled with button states:

```c++
bool fire : 1;          ///< Fire1
bool sec_fire : 1;      ///< Fire2
bool third_fire : 1;    ///< Fire3
bool auto_fire : 1;     ///< Turbo Fire1
bool up : 1;            ///< D-Pad Up
bool down : 1;          ///< D-Pad Down
bool left : 1;          ///< D-Pad Left
bool right : 1;         ///< D-Pad Right
bool joystick_swap : 1; ///< If pressed for a second, both controller ports swap
```

Please use [hid_impact](../src/handlers/hid_impact.cpp) and [hid_ps4.cpp](../src/handlers/hid_ps4.cpp)
as examples for input devices with a simple handling.

For the impact gamepad, the layout is dictated by these lines:
```c++
GamepadReport aj;

aj.update_from_coolie_hat(dat->coolie_hat); // Coolie Hat D-Pad

aj.fire = dat->button_left || dat->button_right;
aj.sec_fire = dat->button_bottom;
aj.auto_fire = dat->button_top;

aj.joystick_swap = dat->button_select;
```

## Did I do it right?

Without connecting the Yaumataca to your homecomputer, you can already check if things are implemented correctly.

The current Controller state is printed to the output as well.

    XYZ:  80 80 80 80 05 00
    L 0010 011
    XYZ:  80 80 80 80 00 00
    L 0000 011

`L` refers to the left controller, the primary joystick.
The first 4 digits are the directional states of the controller port to your homecomputer.
The last 3 digits are the fire buttons.
By pressing buttons you can see whether these match your expectations.

## What can I do if a button press doesn't produce a HID report?

Some controllers require a non-standard protocol. You may need to investigate now, why this happens.

The project https://github.com/felis/USB_Host_Shield_2.0 might provide you with some insight as this library is rather feature-complete.

## What to do if everything works?

Please make a Pull Request on Github. Other gamers might also like your contribution to this project.



