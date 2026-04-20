#include "device.h"

NTSTATUS SwProEvtDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);

    // Upper filter on the HID collection. HIDClass is the function driver below us.
    WdfFdoInitSetFilter(DeviceInit);

    WDF_PNPPOWER_EVENT_CALLBACKS pnp;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnp);
    pnp.EvtDevicePrepareHardware = SwProEvtPrepareHardware;
    pnp.EvtDeviceReleaseHardware = SwProEvtReleaseHardware;
    pnp.EvtDeviceD0Entry         = SwProEvtD0Entry;
    pnp.EvtDeviceD0Exit          = SwProEvtD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnp);

    WDF_OBJECT_ATTRIBUTES attrs;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attrs, SWPRO_DEVICE_CONTEXT);

    WDFDEVICE device;
    NTSTATUS status = WdfDeviceCreate(&DeviceInit, &attrs, &device);
    if (!NT_SUCCESS(status)) return status;

    PSWPRO_DEVICE_CONTEXT ctx = SwProGetDeviceContext(device);
    RtlZeroMemory(ctx, sizeof(*ctx));
    ctx->Device = device;

    return STATUS_SUCCESS;
}

NTSTATUS SwProEvtPrepareHardware(_In_ WDFDEVICE Device,
                                 _In_ WDFCMRESLIST ResourcesRaw,
                                 _In_ WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PSWPRO_DEVICE_CONTEXT ctx = SwProGetDeviceContext(Device);
    ctx->HidTarget = WdfDeviceGetIoTarget(Device);

    NTSTATUS status = SwProVigemAttach(ctx);
    if (!NT_SUCCESS(status)) {
        // ViGEmBus missing or unreachable. Per issue spec: fail gracefully, do not crash.
        return status;
    }

    status = SwProSendSetInputReportMode30(ctx);
    if (!NT_SUCCESS(status)) {
        SwProVigemDetach(ctx);
        return status;
    }
    ctx->ModeSwitched = TRUE;

    return STATUS_SUCCESS;
}

NTSTATUS SwProEvtReleaseHardware(_In_ WDFDEVICE Device, _In_ WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(ResourcesTranslated);
    PSWPRO_DEVICE_CONTEXT ctx = SwProGetDeviceContext(Device);
    SwProVigemDetach(ctx);
    return STATUS_SUCCESS;
}

NTSTATUS SwProEvtD0Entry(_In_ WDFDEVICE Device, _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    UNREFERENCED_PARAMETER(PreviousState);
    PSWPRO_DEVICE_CONTEXT ctx = SwProGetDeviceContext(Device);
    return SwProStartReadLoop(ctx);
}

NTSTATUS SwProEvtD0Exit(_In_ WDFDEVICE Device, _In_ WDF_POWER_DEVICE_STATE TargetState)
{
    UNREFERENCED_PARAMETER(TargetState);
    PSWPRO_DEVICE_CONTEXT ctx = SwProGetDeviceContext(Device);
    if (ctx->PendingReadReq) {
        WdfRequestCancelSentRequest(ctx->PendingReadReq);
    }
    return STATUS_SUCCESS;
}
