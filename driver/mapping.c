#include "common.h"
#include <ViGEm/Common.h>

static SHORT ScaleStickAxis(USHORT raw)
{
    // Pro Controller raw: 0..4095, center ~2048. XInput: -32768..32767.
    LONG centered = (LONG)raw - SWPRO_STICK_CENTER;
    if (centered > -SWPRO_STICK_DEADZONE && centered < SWPRO_STICK_DEADZONE) {
        return 0;
    }
    // Scale (~ 12-bit centered -> 16-bit signed). Using 16 preserves headroom for deadzone subtraction.
    LONG scaled = centered * 16;
    if (scaled > 32767) scaled = 32767;
    if (scaled < -32768) scaled = -32768;
    return (SHORT)scaled;
}

VOID SwProMapToXusb(_In_ const SWPRO_PARSED_INPUT* In, _Out_ XUSB_REPORT* Out)
{
    XUSB_REPORT_INIT(Out);

    USHORT b = 0;

    // Face buttons: map by physical position. Pro's bottom-face (B) -> XInput A, etc.
    if (In->ButtonsRight & SWPRO_BTN_B) b |= XUSB_GAMEPAD_A;
    if (In->ButtonsRight & SWPRO_BTN_A) b |= XUSB_GAMEPAD_B;
    if (In->ButtonsRight & SWPRO_BTN_Y) b |= XUSB_GAMEPAD_X;
    if (In->ButtonsRight & SWPRO_BTN_X) b |= XUSB_GAMEPAD_Y;

    // Shoulders.
    if (In->ButtonsLeft  & SWPRO_BTN_L)  b |= XUSB_GAMEPAD_LEFT_SHOULDER;
    if (In->ButtonsRight & SWPRO_BTN_R)  b |= XUSB_GAMEPAD_RIGHT_SHOULDER;

    // D-pad.
    if (In->ButtonsLeft & SWPRO_BTN_UP)    b |= XUSB_GAMEPAD_DPAD_UP;
    if (In->ButtonsLeft & SWPRO_BTN_DOWN)  b |= XUSB_GAMEPAD_DPAD_DOWN;
    if (In->ButtonsLeft & SWPRO_BTN_LEFT)  b |= XUSB_GAMEPAD_DPAD_LEFT;
    if (In->ButtonsLeft & SWPRO_BTN_RIGHT) b |= XUSB_GAMEPAD_DPAD_RIGHT;

    // Shared: -, +, stick clicks, Home -> Guide. Capture has no XInput equivalent.
    if (In->ButtonsShared & SWPRO_BTN_MINUS)        b |= XUSB_GAMEPAD_BACK;
    if (In->ButtonsShared & SWPRO_BTN_PLUS)         b |= XUSB_GAMEPAD_START;
    if (In->ButtonsShared & SWPRO_BTN_LSTICK_CLICK) b |= XUSB_GAMEPAD_LEFT_THUMB;
    if (In->ButtonsShared & SWPRO_BTN_RSTICK_CLICK) b |= XUSB_GAMEPAD_RIGHT_THUMB;
    if (In->ButtonsShared & SWPRO_BTN_HOME)         b |= XUSB_GAMEPAD_GUIDE;

    Out->wButtons = b;

    // ZL / ZR are digital on Pro Controller; drive XInput triggers to 0 or 255.
    Out->bLeftTrigger  = (In->ButtonsLeft  & SWPRO_BTN_ZL) ? 255 : 0;
    Out->bRightTrigger = (In->ButtonsRight & SWPRO_BTN_ZR) ? 255 : 0;

    Out->sThumbLX = ScaleStickAxis(In->LeftStickX);
    Out->sThumbLY = ScaleStickAxis(In->LeftStickY);
    Out->sThumbRX = ScaleStickAxis(In->RightStickX);
    Out->sThumbRY = ScaleStickAxis(In->RightStickY);
}
