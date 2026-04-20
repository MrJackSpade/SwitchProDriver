#include "common.h"

BOOLEAN SwProParseReport30(
    _In_reads_bytes_(Length) const UCHAR* Report,
    _In_ ULONG Length,
    _Out_ PSWPRO_PARSED_INPUT Parsed)
{
    if (Length < SWPRO_INPUT_REPORT_LEN) {
        return FALSE;
    }
    if (Report[0] != SWPRO_REPORT_ID_STANDARD_FULL) {
        return FALSE;
    }

    const UCHAR btnR = Report[SWPRO_OFS_BTN_RIGHT];
    const UCHAR btnM = Report[SWPRO_OFS_BTN_SHARED];
    const UCHAR btnL = Report[SWPRO_OFS_BTN_LEFT];

    Parsed->ButtonsRight  = btnR;
    Parsed->ButtonsShared = btnM;
    Parsed->ButtonsLeft   = btnL;
    Parsed->Buttons       = (USHORT)((btnR << 8) | btnL); // shared packed separately

    const UCHAR* ls = &Report[SWPRO_OFS_LSTICK];
    Parsed->LeftStickX = (USHORT)(ls[0] | ((ls[1] & 0x0F) << 8));
    Parsed->LeftStickY = (USHORT)((ls[1] >> 4) | (ls[2] << 4));

    const UCHAR* rs = &Report[SWPRO_OFS_RSTICK];
    Parsed->RightStickX = (USHORT)(rs[0] | ((rs[1] & 0x0F) << 8));
    Parsed->RightStickY = (USHORT)((rs[1] >> 4) | (rs[2] << 4));

    return TRUE;
}
