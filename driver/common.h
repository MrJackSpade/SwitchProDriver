#pragma once

#include <windows.h>

#define SWPRO_USB_VID 0x057E
#define SWPRO_USB_PID 0x2009

#define SWPRO_REPORT_ID_STANDARD_FULL   0x30
#define SWPRO_REPORT_ID_OUTPUT_RUMBLE_SC 0x01

#define SWPRO_SUBCMD_SET_INPUT_REPORT_MODE 0x03
#define SWPRO_SUBCMD_ENABLE_VIBRATION      0x48
#define SWPRO_SUBCMD_ENABLE_IMU            0x40
#define SWPRO_SUBCMD_SET_PLAYER_LIGHTS     0x30

#define SWPRO_INPUT_REPORT_LEN   64
#define SWPRO_OUTPUT_REPORT_LEN  64

#define SWPRO_STICK_MIN 0
#define SWPRO_STICK_MAX 4095
#define SWPRO_STICK_CENTER 2048
#define SWPRO_STICK_DEADZONE 512

// Byte offsets within report ID 0x30 body (after the 1-byte report ID).
// Layout: [timer][battery+conn][btnHi][btnMid][btnLo][lx0][lx1][lx2][rx0][rx1][rx2][vibReport]...
#define SWPRO_OFS_BTN_RIGHT  3   // Y, X, B, A, SR-R, SL-R, R, ZR
#define SWPRO_OFS_BTN_SHARED 4   // Minus, Plus, RStickClick, LStickClick, Home, Capture, -, -
#define SWPRO_OFS_BTN_LEFT   5   // Down, Up, Right, Left, SR-L, SL-L, L, ZL
#define SWPRO_OFS_LSTICK     6
#define SWPRO_OFS_RSTICK     9

// Right-side byte (offset 3) bitmasks.
#define SWPRO_BTN_Y   0x01
#define SWPRO_BTN_X   0x02
#define SWPRO_BTN_B   0x04
#define SWPRO_BTN_A   0x08
#define SWPRO_BTN_R   0x40
#define SWPRO_BTN_ZR  0x80

// Shared byte (offset 4) bitmasks.
#define SWPRO_BTN_MINUS        0x01
#define SWPRO_BTN_PLUS         0x02
#define SWPRO_BTN_RSTICK_CLICK 0x04
#define SWPRO_BTN_LSTICK_CLICK 0x08
#define SWPRO_BTN_HOME         0x10
#define SWPRO_BTN_CAPTURE      0x20

// Left-side byte (offset 5) bitmasks.
#define SWPRO_BTN_DOWN  0x01
#define SWPRO_BTN_UP    0x02
#define SWPRO_BTN_RIGHT 0x04
#define SWPRO_BTN_LEFT  0x08
#define SWPRO_BTN_L     0x40
#define SWPRO_BTN_ZL    0x80

typedef struct _SWPRO_PARSED_INPUT {
    USHORT Buttons;          // Raw aggregated across the 3 button bytes (hi=right, mid=shared, lo=left).
    UCHAR  ButtonsRight;
    UCHAR  ButtonsShared;
    UCHAR  ButtonsLeft;
    USHORT LeftStickX;       // 0..4095
    USHORT LeftStickY;       // 0..4095
    USHORT RightStickX;      // 0..4095
    USHORT RightStickY;      // 0..4095
} SWPRO_PARSED_INPUT, *PSWPRO_PARSED_INPUT;

// Parse a Pro Controller 0x30 report body (pointer at report ID byte, length = report length incl. ID).
// Returns FALSE if length is too short or report ID isn't 0x30.
BOOLEAN SwProParseReport30(_In_reads_bytes_(Length) const UCHAR* Report,
                           _In_ ULONG Length,
                           _Out_ PSWPRO_PARSED_INPUT Parsed);
