#include "device.h"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_OBJECT_CONTEXT_CLEANUP SwProEvtDriverCleanup;

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    WDF_OBJECT_ATTRIBUTES attrs;

    WDF_OBJECT_ATTRIBUTES_INIT(&attrs);
    attrs.EvtCleanupCallback = SwProEvtDriverCleanup;

    WDF_DRIVER_CONFIG_INIT(&config, SwProEvtDeviceAdd);
    config.DriverPoolTag = 'rPwS';

    return WdfDriverCreate(DriverObject, RegistryPath, &attrs, &config, WDF_NO_HANDLE);
}

VOID SwProEvtDriverCleanup(_In_ WDFOBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
}
