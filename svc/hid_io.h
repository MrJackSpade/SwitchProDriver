#pragma once

#include <windows.h>
#include "../driver/common.h"

// Enumerate HID devices matching VID_057E / PID_2009 (USB or Bluetooth).
// On success, returns an opened HANDLE with overlapped IO, FILE_SHARE_READ|WRITE,
// and fills *OutPathLen bytes of *OutPath (up to PathCap) with the device path.
// Returns INVALID_HANDLE_VALUE if no matching device found.
HANDLE HidOpenSwitchPro(wchar_t* OutPath, size_t PathCap);

// Query input / output report sizes the OS expects for this device.
// Either pointer may be NULL. Returns FALSE on failure.
BOOL HidGetReportLens(HANDLE dev, ULONG* InputLen, ULONG* OutputLen);

// Send Pro Controller output report 0x01 with subcommand `subcmd` and optional arg bytes.
// `arg` may be NULL if `arglen` == 0. Internally builds the 64-byte packet with idle rumble.
// Returns TRUE on full write.
BOOL HidSendSubcommand(HANDLE dev, UCHAR subcmd, const UCHAR* arg, size_t arglen);

// Start an overlapped ReadFile. Caller provides buf (>= 64 bytes) and OVERLAPPED.
// Returns TRUE if issued (either completed synchronously or pending).
BOOL HidBeginRead(HANDLE dev, UCHAR* buf, DWORD buflen, OVERLAPPED* ov);

// Synchronous SPI flash read via subcommand 0x10. Sends the request, then reads
// input reports for up to ~1 second waiting for a matching 0x21 reply.
// `length` is capped at 0x1D (29). Returns TRUE on success.
BOOL HidReadSpiFlash(HANDLE dev, UINT32 address, UCHAR length, UCHAR* outData);

// Read both factory and user stick calibration from SPI flash and merge into
// `Cal`. User calibration (when present, indicated by magic 0xA1 0xB2) takes
// precedence over factory. Returns TRUE if at least the factory block was read.
// On failure, `Cal` is filled with conservative defaults.
BOOL HidReadSwitchProCalibration(HANDLE dev, SWPRO_CALIBRATION* Cal);
