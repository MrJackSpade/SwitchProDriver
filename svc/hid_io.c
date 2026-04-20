#include "hid_io.h"

#include <setupapi.h>
#include <hidsdi.h>
#include <hidclass.h>
#include <initguid.h>
#include <devguid.h>
#include <stdio.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

#define SWPRO_VID 0x057E
#define SWPRO_PID 0x2009
#define SWPRO_OUTPUT_LEN 64

static const UCHAR kRumbleIdle[8] = { 0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40 };
static UCHAR s_PacketCounter = 0;

HANDLE HidOpenSwitchPro(wchar_t* OutPath, size_t PathCap)
{
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO di = SetupDiGetClassDevsW(&hidGuid, NULL, NULL,
                                       DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (di == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    HANDLE found = INVALID_HANDLE_VALUE;
    SP_DEVICE_INTERFACE_DATA ifd = { sizeof(ifd) };

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(di, NULL, &hidGuid, i, &ifd); ++i) {
        DWORD needed = 0;
        SetupDiGetDeviceInterfaceDetailW(di, &ifd, NULL, 0, &needed, NULL);
        if (!needed) continue;

        PSP_DEVICE_INTERFACE_DETAIL_DATA_W detail =
            (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)HeapAlloc(GetProcessHeap(), 0, needed);
        if (!detail) continue;
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        if (SetupDiGetDeviceInterfaceDetailW(di, &ifd, detail, needed, NULL, NULL)) {
            // Open non-overlapped first just to read attributes (probe).
            HANDLE h = CreateFileW(detail->DevicePath,
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL, OPEN_EXISTING, 0, NULL);
            if (h != INVALID_HANDLE_VALUE) {
                HIDD_ATTRIBUTES attrs = { sizeof(attrs) };
                if (HidD_GetAttributes(h, &attrs) &&
                    attrs.VendorID == SWPRO_VID && attrs.ProductID == SWPRO_PID) {
                    CloseHandle(h);
                    // Re-open with overlapped IO.
                    found = CreateFileW(detail->DevicePath,
                                        GENERIC_READ | GENERIC_WRITE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL, OPEN_EXISTING,
                                        FILE_FLAG_OVERLAPPED, NULL);
                    if (OutPath && PathCap) {
                        wcsncpy_s(OutPath, PathCap, detail->DevicePath, _TRUNCATE);
                    }
                    HeapFree(GetProcessHeap(), 0, detail);
                    break;
                }
                CloseHandle(h);
            }
        }
        HeapFree(GetProcessHeap(), 0, detail);
    }

    SetupDiDestroyDeviceInfoList(di);
    return found;
}

BOOL HidGetReportLens(HANDLE dev, ULONG* InputLen, ULONG* OutputLen)
{
    PHIDP_PREPARSED_DATA pp = NULL;
    if (!HidD_GetPreparsedData(dev, &pp)) return FALSE;
    HIDP_CAPS caps = { 0 };
    NTSTATUS st = HidP_GetCaps(pp, &caps);
    HidD_FreePreparsedData(pp);
    if (st != HIDP_STATUS_SUCCESS) return FALSE;
    if (InputLen) *InputLen = caps.InputReportByteLength;
    if (OutputLen) *OutputLen = caps.OutputReportByteLength;
    return TRUE;
}

static ULONG QueryOutputReportLen(HANDLE dev)
{
    ULONG o = 0;
    return HidGetReportLens(dev, NULL, &o) ? o : 0;
}

BOOL HidSendSubcommand(HANDLE dev, UCHAR subcmd, const UCHAR* arg, size_t arglen)
{
    if (dev == INVALID_HANDLE_VALUE) return FALSE;
    UCHAR pkt[SWPRO_OUTPUT_LEN] = { 0 };
    pkt[0] = 0x01;  // report ID: rumble + subcommand
    pkt[1] = (s_PacketCounter++ & 0x0F);
    memcpy(&pkt[2], kRumbleIdle, sizeof(kRumbleIdle));
    pkt[10] = subcmd;
    if (arg && arglen) {
        if (arglen > sizeof(pkt) - 11) arglen = sizeof(pkt) - 11;
        memcpy(&pkt[11], arg, arglen);
    }

    ULONG reportLen = QueryOutputReportLen(dev);
    if (reportLen == 0 || reportLen > sizeof(pkt)) reportLen = sizeof(pkt);

    OVERLAPPED ov = { 0 };
    ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    DWORD written = 0;
    BOOL ok = WriteFile(dev, pkt, reportLen, &written, &ov);
    DWORD werr = ok ? 0 : GetLastError();
    if (!ok && werr == ERROR_IO_PENDING) {
        ok = GetOverlappedResult(dev, &ov, &written, TRUE);
        werr = ok ? 0 : GetLastError();
    }
    CloseHandle(ov.hEvent);

    if (ok && written == reportLen) return TRUE;

    // Fallback (e.g. some BT stacks reject WriteFile for output reports).
    fprintf(stderr, "[hid_io] WriteFile partial/failed (err=%lu, written=%lu/%lu); fallback to HidD_SetOutputReport.\n",
            werr, written, reportLen);
    if (HidD_SetOutputReport(dev, pkt, reportLen)) return TRUE;
    fprintf(stderr, "[hid_io] HidD_SetOutputReport failed (err=%lu).\n", GetLastError());
    return FALSE;
}

BOOL HidBeginRead(HANDLE dev, UCHAR* buf, DWORD buflen, OVERLAPPED* ov)
{
    if (dev == INVALID_HANDLE_VALUE || !buf || !ov) return FALSE;
    DWORD got = 0;
    BOOL ok = ReadFile(dev, buf, buflen, &got, ov);
    if (ok) return TRUE;
    DWORD e = GetLastError();
    if (e == ERROR_IO_PENDING) return TRUE;
    fprintf(stderr, "[hid_io] ReadFile failed: err=%lu buflen=%lu\n", e, buflen);
    return FALSE;
}
