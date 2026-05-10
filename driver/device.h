#pragma once

#include <windows.h>
#include <wdf.h>
#include <hidsdi.h>
#include <hidclass.h>
#include <ViGEm/Client.h>

#include "common.h"

typedef struct _SWPRO_DEVICE_CONTEXT {
    WDFDEVICE        Device;
    WDFIOTARGET      HidTarget;       // Lower HIDClass stack.
    WDFREQUEST       PendingReadReq;  // Outstanding IOCTL_HID_READ_REPORT.
    WDFMEMORY        ReadBuffer;      // Backing memory for the pended read.

    PVIGEM_CLIENT    Vigem;
    PVIGEM_TARGET    Pad;
    BOOLEAN          PadAttached;
    BOOLEAN          ModeSwitched;
} SWPRO_DEVICE_CONTEXT, *PSWPRO_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SWPRO_DEVICE_CONTEXT, SwProGetDeviceContext)

EVT_WDF_DRIVER_DEVICE_ADD        SwProEvtDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE  SwProEvtPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE  SwProEvtReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY          SwProEvtD0Entry;
EVT_WDF_DEVICE_D0_EXIT           SwProEvtD0Exit;

NTSTATUS SwProStartReadLoop(_In_ PSWPRO_DEVICE_CONTEXT Ctx);
NTSTATUS SwProSendSetInputReportMode30(_In_ PSWPRO_DEVICE_CONTEXT Ctx);

// From vigem.c
NTSTATUS SwProVigemAttach(_In_ PSWPRO_DEVICE_CONTEXT Ctx);
VOID     SwProVigemDetach(_In_ PSWPRO_DEVICE_CONTEXT Ctx);
NTSTATUS SwProVigemSubmit(_In_ PSWPRO_DEVICE_CONTEXT Ctx, _In_ const SWPRO_PARSED_INPUT* In);

// From mapping.c (pure logic, linked in).
VOID SwProMapToXusb(_In_ const SWPRO_PARSED_INPUT* In,
                    _In_ const SWPRO_CALIBRATION* Cal,
                    _Out_ struct _XUSB_REPORT* Out);
