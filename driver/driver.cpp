#include "driver/driver.h"

#include "driver/vhf_device.h"

#if DSL_DRIVER_HAS_WDK

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, DslEvtDeviceAdd);
    return WdfDriverCreate(driver_object, registry_path, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
}

NTSTATUS DslEvtDeviceAdd(WDFDRIVER driver, PWDFDEVICE_INIT device_init) {
    UNREFERENCED_PARAMETER(driver);

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DSL_VHF_CONTEXT);
    attributes.EvtCleanupCallback = DslEvtDeviceContextCleanup;

    WDFDEVICE device = nullptr;
    const NTSTATUS status = WdfDeviceCreate(&device_init, &attributes, &device);
    if(!NT_SUCCESS(status)) {
        return status;
    }
    return DslCreateVirtualDevice(device);
}

void DslEvtDeviceContextCleanup(WDFOBJECT object) {
    DslDestroyVirtualDevice(reinterpret_cast<WDFDEVICE>(object));
}

#else

// This translation unit keeps a valid target when WDK headers are unavailable.
int dualsense_link_driver_stub_symbol = 0;

#endif
