#include "device.h"

// Pro Controller output report 0x01 layout:
//   [0]   0x01 (report ID)
//   [1]   packet counter (low nibble; increments per output)
//   [2..9] rumble data (8 bytes — idle pattern used for MVP since rumble is deferred)
//   [10]  subcommand
//   [11+] subcommand arguments
//
// For the mode-switch "set input report mode" we send subcommand 0x03 with arg 0x30
// (standard full 64-byte report).

#define SWPRO_OUTPUT_SC_HDR_LEN 11

static const UCHAR g_RumbleIdle[8] = { 0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40 };

static UCHAR g_PacketCounter = 0;

NTSTATUS SwProSendSetInputReportMode30(_In_ PSWPRO_DEVICE_CONTEXT Ctx)
{
    UCHAR payload[SWPRO_OUTPUT_REPORT_LEN];
    RtlZeroMemory(payload, sizeof(payload));
    payload[0] = SWPRO_REPORT_ID_OUTPUT_RUMBLE_SC;
    payload[1] = (g_PacketCounter++ & 0x0F);
    RtlCopyMemory(&payload[2], g_RumbleIdle, sizeof(g_RumbleIdle));
    payload[10] = SWPRO_SUBCMD_SET_INPUT_REPORT_MODE;
    payload[11] = SWPRO_REPORT_ID_STANDARD_FULL;

    WDF_MEMORY_DESCRIPTOR md;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&md, payload, sizeof(payload));

    // IOCTL_HID_SET_OUTPUT_REPORT routes the buffer as an output report to the lower stack.
    return WdfIoTargetSendIoctlSynchronously(
        Ctx->HidTarget,
        NULL,
        IOCTL_HID_SET_OUTPUT_REPORT,
        &md,
        NULL,
        NULL,
        NULL);
}
