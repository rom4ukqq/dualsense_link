#include "driver/driver.h"

#include "driver/vhf_device.h"
#include "shared/driver_protocol.h"

#if DSL_DRIVER_HAS_WDK

namespace proto = dsl::shared::driver_protocol;

namespace {

NTSTATUS HandleSubmitInputIoctl(const WDFDEVICE device, WDFREQUEST request, const size_t input_buffer_length) {
    void* in_buffer = nullptr;
    NTSTATUS status = WdfRequestRetrieveInputBuffer(request, sizeof(proto::InputReportRequest), &in_buffer, nullptr);
    if(!NT_SUCCESS(status) || input_buffer_length < sizeof(proto::InputReportRequest)) {
        return status;
    }

    const auto* packet = static_cast<const proto::InputReportRequest*>(in_buffer);
    if(packet->report_size == 0 || packet->report_size > proto::kMaxReportSize) {
        return STATUS_INVALID_PARAMETER;
    }
    return send_input_report(device, packet->report, packet->report_size);
}

NTSTATUS HandlePushOutputIoctl(const WDFDEVICE device, WDFREQUEST request, const size_t input_buffer_length) {
    void* in_buffer = nullptr;
    NTSTATUS status =
        WdfRequestRetrieveInputBuffer(request, sizeof(proto::OutputReportPushRequest), &in_buffer, nullptr);
    if(!NT_SUCCESS(status) || input_buffer_length < sizeof(proto::OutputReportPushRequest)) {
        return status;
    }

    const auto* packet = static_cast<const proto::OutputReportPushRequest*>(in_buffer);
    if(packet->report_size == 0 || packet->report_size > proto::kMaxReportSize) {
        return STATUS_INVALID_PARAMETER;
    }
    return queue_output_report(device, packet->report, packet->report_size);
}

NTSTATUS HandleGetVersionIoctl(WDFREQUEST request, size_t* bytes) {
    void* out_buffer = nullptr;
    NTSTATUS status =
        WdfRequestRetrieveOutputBuffer(request, sizeof(proto::DriverVersionResponse), &out_buffer, nullptr);
    if(!NT_SUCCESS(status)) {
        return status;
    }

    auto* response = static_cast<proto::DriverVersionResponse*>(out_buffer);
    response->interface_version = proto::kInterfaceVersion;
    response->flags = 0;
    *bytes = sizeof(proto::DriverVersionResponse);
    return STATUS_SUCCESS;
}

NTSTATUS HandleGetOutputIoctl(
    const WDFDEVICE device,
    WDFREQUEST request,
    const size_t output_buffer_length,
    size_t* bytes) {
    void* out_buffer = nullptr;
    NTSTATUS status =
        WdfRequestRetrieveOutputBuffer(request, sizeof(proto::OutputReportResponse), &out_buffer, nullptr);
    if(!NT_SUCCESS(status) || output_buffer_length < sizeof(proto::OutputReportResponse)) {
        return status;
    }

    auto* response = static_cast<proto::OutputReportResponse*>(out_buffer);
    size_t copied = 0;
    status = receive_output_report(device, response->report, sizeof(response->report), &copied);
    if(status == STATUS_NO_MORE_ENTRIES) {
        response->report_size = 0;
        *bytes = sizeof(proto::OutputReportResponse);
        return STATUS_SUCCESS;
    }
    if(NT_SUCCESS(status)) {
        response->report_size = static_cast<std::uint32_t>(copied);
        *bytes = sizeof(proto::OutputReportResponse);
    }
    return status;
}

} // namespace

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

    UNICODE_STRING symbolic_link{};
    RtlInitUnicodeString(&symbolic_link, proto::kDosDeviceName);
    const NTSTATUS symlink_status = WdfDeviceCreateSymbolicLink(device, &symbolic_link);
    if(!NT_SUCCESS(symlink_status)) {
        return symlink_status;
    }

    const NTSTATUS queue_status = DslCreateDefaultQueue(device);
    if(!NT_SUCCESS(queue_status)) {
        return queue_status;
    }

    return DslCreateVirtualDevice(device);
}

void DslEvtDeviceContextCleanup(WDFOBJECT object) {
    DslDestroyVirtualDevice(reinterpret_cast<WDFDEVICE>(object));
}

NTSTATUS DslCreateDefaultQueue(WDFDEVICE device) {
    WDF_IO_QUEUE_CONFIG queue_config;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queue_config, WdfIoQueueDispatchSequential);
    queue_config.EvtIoDeviceControl = DslEvtIoDeviceControl;
    return WdfIoQueueCreate(device, &queue_config, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
}

void DslEvtIoDeviceControl(
    WDFQUEUE queue,
    WDFREQUEST request,
    size_t output_buffer_length,
    size_t input_buffer_length,
    ULONG io_control_code) {
    UNREFERENCED_PARAMETER(output_buffer_length);

    const WDFDEVICE device = WdfIoQueueGetDevice(queue);
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    size_t bytes = 0;

    if(io_control_code == proto::IOCTL_DSL_SUBMIT_INPUT_REPORT) {
        status = HandleSubmitInputIoctl(device, request, input_buffer_length);
    } else if(io_control_code == proto::IOCTL_DSL_PUSH_OUTPUT_REPORT) {
        status = HandlePushOutputIoctl(device, request, input_buffer_length);
    } else if(io_control_code == proto::IOCTL_DSL_GET_VERSION) {
        status = HandleGetVersionIoctl(request, &bytes);
    } else if(io_control_code == proto::IOCTL_DSL_GET_OUTPUT_REPORT) {
        status = HandleGetOutputIoctl(device, request, output_buffer_length, &bytes);
    }

    WdfRequestCompleteWithInformation(request, status, bytes);
}

#else

// This translation unit keeps a valid target when WDK headers are unavailable.
int dualsense_link_driver_stub_symbol = 0;

#endif
