#include "device.h"

EVT_WDF_REQUEST_COMPLETION_ROUTINE SwProReadComplete;

NTSTATUS SwProStartReadLoop(_In_ PSWPRO_DEVICE_CONTEXT Ctx)
{
    NTSTATUS status;

    if (Ctx->ReadBuffer == NULL) {
        WDF_OBJECT_ATTRIBUTES attrs;
        WDF_OBJECT_ATTRIBUTES_INIT(&attrs);
        attrs.ParentObject = Ctx->Device;
        status = WdfMemoryCreate(&attrs, NonPagedPoolNx, 'rPwS',
                                 SWPRO_INPUT_REPORT_LEN, &Ctx->ReadBuffer, NULL);
        if (!NT_SUCCESS(status)) return status;
    }

    if (Ctx->PendingReadReq == NULL) {
        WDF_OBJECT_ATTRIBUTES ra;
        WDF_OBJECT_ATTRIBUTES_INIT(&ra);
        ra.ParentObject = Ctx->Device;
        status = WdfRequestCreate(&ra, Ctx->HidTarget, &Ctx->PendingReadReq);
        if (!NT_SUCCESS(status)) return status;
    } else {
        WdfRequestReuse(Ctx->PendingReadReq, WDF_NO_HANDLE);
    }

    status = WdfIoTargetFormatRequestForRead(
        Ctx->HidTarget, Ctx->PendingReadReq, Ctx->ReadBuffer, NULL, NULL);
    if (!NT_SUCCESS(status)) return status;

    WdfRequestSetCompletionRoutine(Ctx->PendingReadReq, SwProReadComplete, Ctx);

    BOOLEAN sent = WdfRequestSend(Ctx->PendingReadReq, Ctx->HidTarget, WDF_NO_SEND_OPTIONS);
    if (!sent) {
        return WdfRequestGetStatus(Ctx->PendingReadReq);
    }
    return STATUS_SUCCESS;
}

VOID SwProReadComplete(_In_ WDFREQUEST Request,
                       _In_ WDFIOTARGET Target,
                       _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
                       _In_ WDFCONTEXT Context)
{
    UNREFERENCED_PARAMETER(Target);
    PSWPRO_DEVICE_CONTEXT ctx = (PSWPRO_DEVICE_CONTEXT)Context;
    NTSTATUS status = Params->IoStatus.Status;

    if (NT_SUCCESS(status)) {
        size_t len = 0;
        PUCHAR buf = (PUCHAR)WdfMemoryGetBuffer(ctx->ReadBuffer, &len);
        SWPRO_PARSED_INPUT parsed;
        if (SwProParseReport30(buf, (ULONG)len, &parsed)) {
            (void)SwProVigemSubmit(ctx, &parsed);
        }
    }
    // Kick off the next read regardless. On D0Exit / surprise-remove the
    // target will fail the request with STATUS_CANCELLED or similar and
    // we'll stop naturally after WdfRequestCancelSentRequest.
    UNREFERENCED_PARAMETER(Request);
    if (status != STATUS_CANCELLED && status != STATUS_DEVICE_NOT_CONNECTED) {
        (void)SwProStartReadLoop(ctx);
    }
}
