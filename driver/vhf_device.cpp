#include "driver/vhf_device.h"

#include "driver/hid_descriptor.h"

#if DSL_DRIVER_HAS_WDK

NTSTATUS DslCreateVirtualDevice(WDFDEVICE device) {
    auto* context = DslGetVhfContext(device);
    context->handle = nullptr;

    VHF_CONFIG config;
    VHF_CONFIG_INIT(
        &config,
        WdfDeviceWdmGetDeviceObject(device),
        static_cast<USHORT>(dsl::driver::kGamepadReportDescriptor.size()),
        const_cast<PUCHAR>(dsl::driver::kGamepadReportDescriptor.data()));

    config.VendorID = dsl::driver::kVendorId;
    config.ProductID = dsl::driver::kProductId;
    config.VersionNumber = dsl::driver::kVersionNumber;

    const NTSTATUS status = VhfCreate(&config, &context->handle);
    if(!NT_SUCCESS(status)) {
        return status;
    }
    VhfStart(context->handle);
    return STATUS_SUCCESS;
}

void DslDestroyVirtualDevice(WDFDEVICE device) {
    auto* context = DslGetVhfContext(device);
    if(context->handle != nullptr) {
        VhfDelete(context->handle, TRUE);
        context->handle = nullptr;
    }
}

NTSTATUS send_input_report(WDFDEVICE device, const std::uint8_t* report, const std::size_t report_size) {
    if(report == nullptr || report_size == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    auto* context = DslGetVhfContext(device);
    if(context->handle == nullptr) {
        return STATUS_DEVICE_NOT_READY;
    }

    VHF_XFER_PACKET packet;
    VHF_XFER_PACKET_INIT(&packet, const_cast<PUCHAR>(report), static_cast<ULONG>(report_size));
    return VhfReadReportSubmit(context->handle, &packet);
}

NTSTATUS receive_output_report(
    WDFDEVICE,
    std::uint8_t*,
    std::size_t,
    std::size_t*) {
    // TODO: hook EvtVhfAsyncOperationGetFeature / EvtVhfReadyForNextReadReport path.
    return STATUS_NOT_SUPPORTED;
}

#else

NTSTATUS DslCreateVirtualDevice(WDFDEVICE) {
    return STATUS_NOT_SUPPORTED;
}

void DslDestroyVirtualDevice(WDFDEVICE) {
}

NTSTATUS send_input_report(WDFDEVICE, const std::uint8_t*, std::size_t) {
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS receive_output_report(WDFDEVICE, std::uint8_t*, std::size_t, std::size_t*) {
    return STATUS_NOT_SUPPORTED;
}

#endif
