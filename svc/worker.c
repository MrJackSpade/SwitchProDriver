#include "worker.h"
#include "hid_io.h"

#include <ViGEm/Client.h>
#include <stdio.h>

#include "../driver/common.h"

// Forward decl from driver/mapping.c
VOID SwProMapToXusb(const SWPRO_PARSED_INPUT* In, XUSB_REPORT* Out);

static void ReportStatus(StatusCallback cb, void* ctx, const char* msg)
{
    if (cb) cb(msg, ctx);
}

static HANDLE WaitForController(volatile LONG* stop, StatusCallback cb, void* ctx)
{
    BOOL announced = FALSE;
    for (;;) {
        if (*stop) return INVALID_HANDLE_VALUE;
        HANDLE h = HidOpenSwitchPro(NULL, 0);
        if (h != INVALID_HANDLE_VALUE) {
            ReportStatus(cb, ctx, "Pro Controller found, opened HID device.");
            return h;
        }
        if (!announced) {
            ReportStatus(cb, ctx, "Waiting for Pro Controller (USB or Bluetooth)...");
            announced = TRUE;
        }
        Sleep(500);
    }
}

int WorkerRun(volatile LONG* StopFlag, StatusCallback cb, void* ctx)
{
    // ViGEm client — single lifetime for the whole service run.
    PVIGEM_CLIENT vigem = vigem_alloc();
    if (!vigem) {
        ReportStatus(cb, ctx, "vigem_alloc failed");
        return 1;
    }
    VIGEM_ERROR verr = vigem_connect(vigem);
    if (!VIGEM_SUCCESS(verr)) {
        ReportStatus(cb, ctx, "vigem_connect failed (is ViGEmBus installed?)");
        vigem_free(vigem);
        return 2;
    }

    while (!*StopFlag) {
        HANDLE dev = WaitForController(StopFlag, cb, ctx);
        if (dev == INVALID_HANDLE_VALUE) break;

        // Switch to full report mode 0x30.
        UCHAR arg = SWPRO_REPORT_ID_STANDARD_FULL;
        if (!HidSendSubcommand(dev, SWPRO_SUBCMD_SET_INPUT_REPORT_MODE, &arg, 1)) {
            ReportStatus(cb, ctx, "Failed to send set-input-mode subcommand. Reopening.");
            CloseHandle(dev);
            Sleep(500);
            continue;
        }

        // Attach virtual X360 pad.
        PVIGEM_TARGET pad = vigem_target_x360_alloc();
        verr = vigem_target_add(vigem, pad);
        if (!VIGEM_SUCCESS(verr)) {
            ReportStatus(cb, ctx, "vigem_target_add failed");
            vigem_target_free(pad);
            CloseHandle(dev);
            Sleep(1000);
            continue;
        }
        ReportStatus(cb, ctx, "Virtual Xbox 360 pad attached. Piping input.");

        ULONG inLen = 0, outLen = 0;
        HidGetReportLens(dev, &inLen, &outLen);
        if (inLen == 0) inLen = 64;  // sensible default for USB Pro Controller
        if (inLen > 4096) inLen = 4096;  // sanity ceiling

        UCHAR* buf = (UCHAR*)HeapAlloc(GetProcessHeap(), 0, inLen);
        OVERLAPPED ov = { 0 };
        ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

        BOOL disconnected = FALSE;
        while (!*StopFlag && !disconnected) {
            ResetEvent(ov.hEvent);
            if (!HidBeginRead(dev, buf, inLen, &ov)) {
                ReportStatus(cb, ctx, "HidBeginRead failed. Reopening.");
                disconnected = TRUE;
                break;
            }

            // Wait up to 500ms for a report; short timeout so we can check the stop flag.
            DWORD wr = WaitForSingleObject(ov.hEvent, 500);
            if (wr == WAIT_TIMEOUT) {
                // Cancel the pending read and loop.
                CancelIoEx(dev, &ov);
                DWORD tmp;
                GetOverlappedResult(dev, &ov, &tmp, TRUE);
                continue;
            }
            if (wr != WAIT_OBJECT_0) {
                disconnected = TRUE;
                break;
            }

            DWORD got = 0;
            if (!GetOverlappedResult(dev, &ov, &got, FALSE)) {
                // ERROR_DEVICE_NOT_CONNECTED or similar.
                disconnected = TRUE;
                break;
            }

            SWPRO_PARSED_INPUT parsed;
            // Pro Controller HID reports via ReadFile include the report ID as byte 0.
            if (got >= SWPRO_INPUT_REPORT_LEN && SwProParseReport30(buf, got, &parsed)) {
                XUSB_REPORT r;
                SwProMapToXusb(&parsed, &r);
                vigem_target_x360_update(vigem, pad, r);
            }
        }

        CloseHandle(ov.hEvent);
        HeapFree(GetProcessHeap(), 0, buf);
        vigem_target_remove(vigem, pad);
        vigem_target_free(pad);
        CloseHandle(dev);
        if (!*StopFlag) {
            ReportStatus(cb, ctx, "Controller disconnected. Waiting for reconnect.");
        }
    }

    vigem_disconnect(vigem);
    vigem_free(vigem);
    ReportStatus(cb, ctx, "Worker stopped.");
    return 0;
}
