#include "common.h"
#include <ViGEm/Common.h>

VOID SwProDefaultCalibration(_Out_ PSWPRO_CALIBRATION Cal)
{
    Cal->Left.X_Center = SWPRO_STICK_CENTER;
    Cal->Left.Y_Center = SWPRO_STICK_CENTER;
    Cal->Left.X_Below  = SWPRO_DEFAULT_STICK_TRAVEL;
    Cal->Left.Y_Below  = SWPRO_DEFAULT_STICK_TRAVEL;
    Cal->Left.X_Above  = SWPRO_DEFAULT_STICK_TRAVEL;
    Cal->Left.Y_Above  = SWPRO_DEFAULT_STICK_TRAVEL;
    Cal->Right = Cal->Left;
}

// Factory left-stick calibration: 9 bytes, packed nibbles, ordered max, center, min.
VOID SwProParseFactoryLeftStickCal(_In_reads_bytes_(9) const UCHAR* d,
                                   _Out_ PSWPRO_STICK_CAL Cal)
{
    USHORT x_above = (USHORT)(((d[1] & 0x0F) << 8) | d[0]);
    USHORT y_above = (USHORT)((d[2] << 4) | (d[1] >> 4));
    USHORT x_cen   = (USHORT)(((d[4] & 0x0F) << 8) | d[3]);
    USHORT y_cen   = (USHORT)((d[5] << 4) | (d[4] >> 4));
    USHORT x_below = (USHORT)(((d[7] & 0x0F) << 8) | d[6]);
    USHORT y_below = (USHORT)((d[8] << 4) | (d[7] >> 4));
    Cal->X_Center = x_cen;
    Cal->Y_Center = y_cen;
    Cal->X_Below  = x_below;
    Cal->Y_Below  = y_below;
    Cal->X_Above  = x_above;
    Cal->Y_Above  = y_above;
}

// Factory right-stick calibration: 9 bytes, packed nibbles, ordered center, min, max.
VOID SwProParseFactoryRightStickCal(_In_reads_bytes_(9) const UCHAR* d,
                                    _Out_ PSWPRO_STICK_CAL Cal)
{
    USHORT x_cen   = (USHORT)(((d[1] & 0x0F) << 8) | d[0]);
    USHORT y_cen   = (USHORT)((d[2] << 4) | (d[1] >> 4));
    USHORT x_below = (USHORT)(((d[4] & 0x0F) << 8) | d[3]);
    USHORT y_below = (USHORT)((d[5] << 4) | (d[4] >> 4));
    USHORT x_above = (USHORT)(((d[7] & 0x0F) << 8) | d[6]);
    USHORT y_above = (USHORT)((d[8] << 4) | (d[7] >> 4));
    Cal->X_Center = x_cen;
    Cal->Y_Center = y_cen;
    Cal->X_Below  = x_below;
    Cal->Y_Below  = y_below;
    Cal->X_Above  = x_above;
    Cal->Y_Above  = y_above;
}

static SHORT ScaleAxis(USHORT raw, USHORT center, USHORT below, USHORT above)
{
    LONG centered = (LONG)raw - (LONG)center;
    LONG dz = (LONG)SWPRO_STICK_DEADZONE;

    if (centered > -dz && centered < dz) {
        return 0;
    }

    LONG range, axis;
    if (centered > 0) {
        range = (LONG)above - dz;
        axis  = centered - dz;
    } else {
        range = (LONG)below - dz;
        axis  = centered + dz;  // negative
    }
    if (range < 1) range = 1;  // guard against pathological calibration

    LONG scaled = axis * 32767 / range;
    if (scaled >  32767) scaled =  32767;
    if (scaled < -32768) scaled = -32768;
    return (SHORT)scaled;
}

VOID SwProMapToXusb(_In_ const SWPRO_PARSED_INPUT* In,
                    _In_ const SWPRO_CALIBRATION* Cal,
                    _Out_ XUSB_REPORT* Out)
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

    Out->sThumbLX = ScaleAxis(In->LeftStickX,  Cal->Left.X_Center,  Cal->Left.X_Below,  Cal->Left.X_Above);
    Out->sThumbLY = ScaleAxis(In->LeftStickY,  Cal->Left.Y_Center,  Cal->Left.Y_Below,  Cal->Left.Y_Above);
    Out->sThumbRX = ScaleAxis(In->RightStickX, Cal->Right.X_Center, Cal->Right.X_Below, Cal->Right.X_Above);
    Out->sThumbRY = ScaleAxis(In->RightStickY, Cal->Right.Y_Center, Cal->Right.Y_Below, Cal->Right.Y_Above);
}
