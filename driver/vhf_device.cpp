#include "driver/vhf_device.h"

#include "driver/hid_descriptor.h"

#if DSL_DRIVER_HAS_WDK

#include <algorithm>

NTSTATUS DslCreateVirtualDevice(WDFDEVICE device) {
    auto* context = DslGetVhfContext(device);
    context->handle = nullptr;
    context->output_lock = nullptr;
    context->queue_head = 0;
    context->queue_tail = 0;
    context->queue_count = 0;

    WDF_OBJECT_ATTRIBUTES lock_attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&lock_attributes);
    lock_attributes.ParentObject = device;
    NTSTATUS status = WdfSpinLockCreate(&lock_attributes, &context->output_lock);
    if(!NT_SUCCESS(status)) {
        return status;
    }

    VHF_CONFIG config;
    VHF_CONFIG_INIT(
        &config,
        WdfDeviceWdmGetDeviceObject(device),
        static_cast<USHORT>(dsl::driver::kGamepadReportDescriptor.size()),
        const_cast<PUCHAR>(dsl::driver::kGamepadReportDescriptor.data()));

    config.VendorID = dsl::driver::kVendorId;
    config.ProductID = dsl::driver::kProductId;
    config.VersionNumber = dsl::driver::kVersionNumber;

    status = VhfCreate(&config, &context->handle);
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
    WDFDEVICE device,
    std::uint8_t* report,
    const std::size_t report_size,
    std::size_t* bytes_copied) {
    if(report == nullptr || bytes_copied == nullptr || report_size == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    auto* context = DslGetVhfContext(device);
    if(context->output_lock == nullptr) {
        return STATUS_DEVICE_NOT_READY;
    }

    WdfSpinLockAcquire(context->output_lock);
    if(context->queue_count == 0) {
        WdfSpinLockRelease(context->output_lock);
        *bytes_copied = 0;
        return STATUS_NO_MORE_ENTRIES;
    }

    auto* packet = &context->output_queue[context->queue_head];
    const auto copy_size = (std::min)(packet->report_size, report_size);
    RtlCopyMemory(report, packet->report, copy_size);
    packet->report_size = 0;
    context->queue_head = (context->queue_head + 1) % kDslOutputQueueCapacity;
    --context->queue_count;
    WdfSpinLockRelease(context->output_lock);

    *bytes_copied = copy_size;
    return STATUS_SUCCESS;
}

NTSTATUS queue_output_report(WDFDEVICE device, const std::uint8_t* report, const std::size_t report_size) {
    auto* context = DslGetVhfContext(device);
    if(report == nullptr || report_size == 0 || report_size > kDslMaxOutputReportSize) {
        return STATUS_INVALID_PARAMETER;
    }
    if(context->output_lock == nullptr) {
        return STATUS_DEVICE_NOT_READY;
    }

    WdfSpinLockAcquire(context->output_lock);
    if(context->queue_count >= kDslOutputQueueCapacity) {
        WdfSpinLockRelease(context->output_lock);
        return STATUS_BUFFER_OVERFLOW;
    }

    auto* packet = &context->output_queue[context->queue_tail];
    RtlCopyMemory(packet->report, report, report_size);
    packet->report_size = report_size;
    context->queue_tail = (context->queue_tail + 1) % kDslOutputQueueCapacity;
    ++context->queue_count;
    WdfSpinLockRelease(context->output_lock);
    return STATUS_SUCCESS;
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

NTSTATUS queue_output_report(WDFDEVICE, const std::uint8_t*, std::size_t) {
    return STATUS_NOT_SUPPORTED;
}

#endif
