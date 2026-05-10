#pragma once

#include <windows.h>

#define SWPRO_USB_VID 0x057E
#define SWPRO_USB_PID 0x2009

#define SWPRO_REPORT_ID_STANDARD_FULL   0x30
#define SWPRO_REPORT_ID_SUBCMD_REPLY    0x21
#define SWPRO_REPORT_ID_OUTPUT_RUMBLE_SC 0x01

#define SWPRO_SUBCMD_SET_INPUT_REPORT_MODE 0x03
#define SWPRO_SUBCMD_ENABLE_VIBRATION      0x48
#define SWPRO_SUBCMD_ENABLE_IMU            0x40
#define SWPRO_SUBCMD_SET_PLAYER_LIGHTS     0x30
#define SWPRO_SUBCMD_SPI_FLASH_READ        0x10

#define SWPRO_INPUT_REPORT_LEN   64
#define SWPRO_OUTPUT_REPORT_LEN  64

#define SWPRO_STICK_MIN 0
#define SWPRO_STICK_MAX 4095
#define SWPRO_STICK_CENTER 2048
// Hardware noise on the Pro Controller is typically within ~50 12-bit units of
// rest. 160 silences that with a small "still" zone; games apply their own
// (much larger) deadzones on top of this.
#define SWPRO_STICK_DEADZONE 160

// SPI flash addresses (factory and user stick calibration).
// Factory: 18 bytes total — 9 for left stick (max/center/min) then 9 for
// right stick (center/min/max). See JC-Reverse-Engineering / hid-nintendo.
#define SWPRO_SPI_FACTORY_STICK_CAL_ADDR  0x603D
#define SWPRO_SPI_FACTORY_STICK_CAL_LEN   18

// User calibration: each block is 11 bytes (2 magic + 9 cal). Magic 0xA1 0xB2
// at the start indicates the user has calibrated the stick on the console.
#define SWPRO_SPI_USER_LSTICK_CAL_ADDR    0x8010
#define SWPRO_SPI_USER_RSTICK_CAL_ADDR    0x801B
#define SWPRO_SPI_USER_STICK_CAL_LEN      11
#define SWPRO_SPI_USER_CAL_MAGIC0         0xA1
#define SWPRO_SPI_USER_CAL_MAGIC1         0xB2

// Per-stick calibration: center, plus positive offsets describing how far the
// stick travels above and below center on each axis.
typedef struct _SWPRO_STICK_CAL {
    USHORT X_Center;
    USHORT Y_Center;
    USHORT X_Below;   // travel from center toward 0 (X)
    USHORT Y_Below;   // travel from center toward 0 (Y)
    USHORT X_Above;   // travel from center toward 4095 (X)
    USHORT Y_Above;   // travel from center toward 4095 (Y)
} SWPRO_STICK_CAL, *PSWPRO_STICK_CAL;

typedef struct _SWPRO_CALIBRATION {
    SWPRO_STICK_CAL Left;
    SWPRO_STICK_CAL Right;
} SWPRO_CALIBRATION, *PSWPRO_CALIBRATION;

// Reasonable defaults if SPI calibration can't be read. Travel of ~1100 in each
// direction is on the conservative side of typical Pro Controller hardware, so
// most sticks will saturate XInput at full deflection rather than fall short.
#define SWPRO_DEFAULT_STICK_TRAVEL 1100

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

// Populate a calibration struct with conservative defaults (used if SPI read fails).
VOID SwProDefaultCalibration(_Out_ PSWPRO_CALIBRATION Cal);

// Parse a 9-byte factory left-stick calibration block (layout: max, center, min).
VOID SwProParseFactoryLeftStickCal(_In_reads_bytes_(9) const UCHAR* Data,
                                   _Out_ PSWPRO_STICK_CAL Cal);

// Parse a 9-byte factory right-stick calibration block (layout: center, min, max).
VOID SwProParseFactoryRightStickCal(_In_reads_bytes_(9) const UCHAR* Data,
                                    _Out_ PSWPRO_STICK_CAL Cal);
