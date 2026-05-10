#include "device.h"

NTSTATUS SwProVigemAttach(_In_ PSWPRO_DEVICE_CONTEXT Ctx)
{
    Ctx->Vigem = vigem_alloc();
    if (!Ctx->Vigem) return STATUS_INSUFFICIENT_RESOURCES;

    VIGEM_ERROR err = vigem_connect(Ctx->Vigem);
    if (!VIGEM_SUCCESS(err)) {
        vigem_free(Ctx->Vigem);
        Ctx->Vigem = NULL;
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    Ctx->Pad = vigem_target_x360_alloc();
    err = vigem_target_add(Ctx->Vigem, Ctx->Pad);
    if (!VIGEM_SUCCESS(err)) {
        vigem_target_free(Ctx->Pad);
        Ctx->Pad = NULL;
        vigem_disconnect(Ctx->Vigem);
        vigem_free(Ctx->Vigem);
        Ctx->Vigem = NULL;
        return STATUS_UNSUCCESSFUL;
    }
    Ctx->PadAttached = TRUE;
    return STATUS_SUCCESS;
}

VOID SwProVigemDetach(_In_ PSWPRO_DEVICE_CONTEXT Ctx)
{
    if (Ctx->Pad) {
        if (Ctx->PadAttached) {
            vigem_target_remove(Ctx->Vigem, Ctx->Pad);
            Ctx->PadAttached = FALSE;
        }
        vigem_target_free(Ctx->Pad);
        Ctx->Pad = NULL;
    }
    if (Ctx->Vigem) {
        vigem_disconnect(Ctx->Vigem);
        vigem_free(Ctx->Vigem);
        Ctx->Vigem = NULL;
    }
}

NTSTATUS SwProVigemSubmit(_In_ PSWPRO_DEVICE_CONTEXT Ctx, _In_ const SWPRO_PARSED_INPUT* In)
{
    if (!Ctx->PadAttached) return STATUS_DEVICE_NOT_READY;
    // Kernel driver path doesn't yet read SPI calibration; use conservative defaults.
    SWPRO_CALIBRATION cal;
    SwProDefaultCalibration(&cal);
    XUSB_REPORT rep;
    SwProMapToXusb(In, &cal, &rep);
    VIGEM_ERROR err = vigem_target_x360_update(Ctx->Vigem, Ctx->Pad, rep);
    return VIGEM_SUCCESS(err) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}
