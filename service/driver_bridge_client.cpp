#include "service/driver_bridge_client.h"

#include "shared/driver_protocol.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>
#include <utility>

namespace dsl::service {

namespace proto = dsl::shared::driver_protocol;

DriverBridgeClient::~DriverBridgeClient() {
    Stop();
}

bool DriverBridgeClient::Start() {
    if(running_.exchange(true)) {
        return IsConnected();
    }
    if(!OpenDevice() || !ValidateInterfaceVersion()) {
        running_.store(false);
        CloseDevice();
        return false;
    }

    output_thread_ = std::jthread([this](std::stop_token) { ReadOutputLoop(); });
    return true;
}

void DriverBridgeClient::Stop() {
    if(!running_.exchange(false)) {
        return;
    }
    CloseDevice();
    if(output_thread_.joinable()) {
        output_thread_.request_stop();
        output_thread_.join();
    }
}

bool DriverBridgeClient::IsConnected() const {
    std::scoped_lock lock(device_mutex_);
    return device_ != INVALID_HANDLE_VALUE;
}

bool DriverBridgeClient::SendInputReport(const std::span<const std::uint8_t> report) {
    if(report.empty() || report.size() > proto::kMaxReportSize) {
        return false;
    }

    HANDLE device_copy = INVALID_HANDLE_VALUE;
    {
        std::scoped_lock lock(device_mutex_);
        device_copy = device_;
    }
    if(device_copy == INVALID_HANDLE_VALUE) {
        return false;
    }

    proto::InputReportRequest packet{};
    packet.report_size = static_cast<std::uint32_t>(report.size());
    std::memcpy(packet.report, report.data(), report.size());

    DWORD bytes = 0;
    return DeviceIoControl(
        device_copy,
        proto::IOCTL_DSL_SUBMIT_INPUT_REPORT,
        &packet,
        sizeof(packet),
        nullptr,
        0,
        &bytes,
        nullptr) != FALSE;
}

void DriverBridgeClient::SetOutputReportHandler(OutputReportHandler handler) {
    std::scoped_lock lock(callback_mutex_);
    output_handler_ = std::move(handler);
}

bool DriverBridgeClient::EmitTestOutputReport() {
    HANDLE device_copy = INVALID_HANDLE_VALUE;
    {
        std::scoped_lock lock(device_mutex_);
        device_copy = device_;
    }
    if(device_copy == INVALID_HANDLE_VALUE) {
        return false;
    }

    proto::OutputReportPushRequest request{};
    request.report_size = 4;
    request.report[0] = 0x02;
    request.report[1] = 0xFF;
    request.report[2] = 0x40;
    request.report[3] = 0x40;

    DWORD bytes = 0;
    return DeviceIoControl(
        device_copy,
        proto::IOCTL_DSL_PUSH_OUTPUT_REPORT,
        &request,
        sizeof(request),
        nullptr,
        0,
        &bytes,
        nullptr) != FALSE;
}

bool DriverBridgeClient::OpenDevice() {
    const HANDLE device = CreateFileW(
        proto::kUserDevicePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if(device == INVALID_HANDLE_VALUE) {
        return false;
    }

    std::scoped_lock lock(device_mutex_);
    device_ = device;
    return true;
}

bool DriverBridgeClient::ValidateInterfaceVersion() {
    const HANDLE device_copy = CopyDeviceHandle();
    if(device_copy == INVALID_HANDLE_VALUE) {
        return false;
    }

    proto::DriverVersionResponse version{};
    DWORD bytes = 0;
    if(!DeviceIoControl(
           device_copy,
           proto::IOCTL_DSL_GET_VERSION,
           nullptr,
           0,
           &version,
           sizeof(version),
           &bytes,
           nullptr)) {
        return false;
    }
    return bytes >= sizeof(version) && version.interface_version == proto::kInterfaceVersion;
}

bool DriverBridgeClient::ReadOutputReport(HANDLE device, std::vector<std::uint8_t>* report) {
    if(device == INVALID_HANDLE_VALUE || report == nullptr) {
        return false;
    }

    proto::OutputReportResponse out_packet{};
    DWORD bytes = 0;
    const BOOL ok = DeviceIoControl(
        device,
        proto::IOCTL_DSL_GET_OUTPUT_REPORT,
        nullptr,
        0,
        &out_packet,
        sizeof(out_packet),
        &bytes,
        nullptr);
    if(!ok || out_packet.report_size == 0 || out_packet.report_size > proto::kMaxReportSize) {
        return false;
    }

    report->resize(out_packet.report_size);
    std::memcpy(report->data(), out_packet.report, out_packet.report_size);
    return true;
}

HANDLE DriverBridgeClient::CopyDeviceHandle() const {
    std::scoped_lock lock(device_mutex_);
    return device_;
}

DriverBridgeClient::OutputReportHandler DriverBridgeClient::CopyOutputHandler() const {
    std::scoped_lock lock(callback_mutex_);
    return output_handler_;
}

void DriverBridgeClient::ReadOutputLoop() {
    while(running_.load()) {
        const HANDLE device_copy = CopyDeviceHandle();
        if(device_copy == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            continue;
        }

        std::vector<std::uint8_t> report;
        if(!ReadOutputReport(device_copy, &report)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
            continue;
        }

        auto callback = CopyOutputHandler();
        if(callback) {
            callback(report);
        }
    }
}

void DriverBridgeClient::CloseDevice() {
    std::scoped_lock lock(device_mutex_);
    if(device_ != INVALID_HANDLE_VALUE) {
        CloseHandle(device_);
        device_ = INVALID_HANDLE_VALUE;
    }
}

} // namespace dsl::service
