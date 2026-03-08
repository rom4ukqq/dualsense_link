#include "service/bluetooth_manager.h"

#include "shared/common_types.h"

#include <hidsdi.h>
#include <setupapi.h>

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace dsl::service {

namespace {
constexpr std::array<std::uint16_t, 2> kSupportedDualSensePids = {
    shared::kDualSensePid,
    shared::kDualSenseEdgePid
};

std::wstring GetInterfacePath(HDEVINFO info, const GUID& hid_guid, const DWORD index) {
    SP_DEVICE_INTERFACE_DATA interface_data{};
    interface_data.cbSize = sizeof(interface_data);
    if(!SetupDiEnumDeviceInterfaces(info, nullptr, &hid_guid, index, &interface_data)) {
        return {};
    }

    DWORD required = 0;
    SetupDiGetDeviceInterfaceDetailW(info, &interface_data, nullptr, 0, &required, nullptr);
    if(required == 0) {
        return {};
    }

    std::vector<std::byte> buffer(required);
    auto* detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(buffer.data());
    detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

    if(!SetupDiGetDeviceInterfaceDetailW(info, &interface_data, detail, required, nullptr, nullptr)) {
        return {};
    }
    return detail->DevicePath;
}
}

BluetoothManager::~BluetoothManager() {
    Stop();
}

bool BluetoothManager::Start() {
    if(running_.load()) {
        return true;
    }

    device_path_ = FindDualSenseDevicePath();
    if(device_path_.empty() || !OpenDevice(device_path_)) {
        return false;
    }

    running_ = true;
    read_thread_ = std::jthread([this](std::stop_token) { ReadLoop(); });

    ConnectedCallback connected_cb;
    {
        std::scoped_lock lock(callback_mutex_);
        connected_cb = on_connected_;
    }
    if(connected_cb) {
        connected_cb(device_path_);
    }
    return true;
}

void BluetoothManager::Stop() {
    if(!running_.exchange(false)) {
        return;
    }

    if(device_handle_ != INVALID_HANDLE_VALUE) {
        CancelIoEx(device_handle_, nullptr);
    }
    if(read_thread_.joinable()) {
        read_thread_.request_stop();
        read_thread_.join();
    }
    CloseDevice();
}

bool BluetoothManager::IsRunning() const noexcept {
    return running_.load();
}

void BluetoothManager::SetOnControllerConnected(ConnectedCallback callback) {
    std::scoped_lock lock(callback_mutex_);
    on_connected_ = std::move(callback);
}

void BluetoothManager::SetOnInputReportReceived(InputReportCallback callback) {
    std::scoped_lock lock(callback_mutex_);
    on_report_ = std::move(callback);
}

bool BluetoothManager::WriteOutputReport(std::span<const std::uint8_t> report) {
    if(device_handle_ == INVALID_HANDLE_VALUE || report.empty()) {
        return false;
    }

    OVERLAPPED overlap{};
    overlap.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if(overlap.hEvent == nullptr) {
        return false;
    }

    const BOOL started = WriteFile(
        device_handle_,
        report.data(),
        static_cast<DWORD>(report.size()),
        nullptr,
        &overlap);

    const DWORD error = started ? ERROR_SUCCESS : GetLastError();
    if(!started && error != ERROR_IO_PENDING) {
        CloseHandle(overlap.hEvent);
        return false;
    }

    const DWORD wait_result = WaitForSingleObject(overlap.hEvent, 200);
    DWORD written = 0;
    const bool ok = wait_result == WAIT_OBJECT_0 &&
                    GetOverlappedResult(device_handle_, &overlap, &written, FALSE) &&
                    written == report.size();

    CloseHandle(overlap.hEvent);
    return ok;
}

std::wstring BluetoothManager::FindDualSenseDevicePath() const {
    GUID hid_guid{};
    HidD_GetHidGuid(&hid_guid);

    HDEVINFO info = SetupDiGetClassDevsW(
        &hid_guid,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if(info == INVALID_HANDLE_VALUE) {
        return {};
    }

    // Enumerate all present HID interfaces and pick Sony DualSense VID/PID.
    for(DWORD i = 0;; ++i) {
        const auto device_path = GetInterfacePath(info, hid_guid, i);
        if(device_path.empty()) {
            if(GetLastError() == ERROR_NO_MORE_ITEMS) {
                break;
            }
            continue;
        }
        if(IsDualSensePath(device_path)) {
            SetupDiDestroyDeviceInfoList(info);
            return device_path;
        }
    }

    SetupDiDestroyDeviceInfoList(info);
    return {};
}

bool BluetoothManager::IsDualSensePath(const std::wstring& device_path) const {
    HANDLE handle = CreateFileW(
        device_path.c_str(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if(handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    const bool is_dualsense = IsDualSenseDevice(handle);
    CloseHandle(handle);
    return is_dualsense;
}

bool BluetoothManager::OpenDevice(const std::wstring& device_path) {
    device_handle_ = CreateFileW(
        device_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr);

    if(device_handle_ != INVALID_HANDLE_VALUE) {
        return true;
    }

    device_handle_ = CreateFileW(
        device_path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr);

    return device_handle_ != INVALID_HANDLE_VALUE;
}

void BluetoothManager::CloseDevice() {
    if(device_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(device_handle_);
        device_handle_ = INVALID_HANDLE_VALUE;
    }
    device_path_.clear();
}

void BluetoothManager::EmitInputReport(std::span<const std::uint8_t> report) {
    InputReportCallback report_cb;
    {
        std::scoped_lock lock(callback_mutex_);
        report_cb = on_report_;
    }
    if(report_cb) {
        report_cb(std::vector<std::uint8_t>(report.begin(), report.end()));
    }
}

void BluetoothManager::ReadLoop() {
    constexpr DWORD kBufferSize = 128;
    std::vector<std::uint8_t> buffer(kBufferSize);

    while(running_.load()) {
        OVERLAPPED overlap{};
        overlap.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if(overlap.hEvent == nullptr) {
            return;
        }

        // Overlapped read keeps the loop cancellable from Stop().
        const BOOL started = ReadFile(device_handle_, buffer.data(), kBufferSize, nullptr, &overlap);
        const DWORD error = started ? ERROR_SUCCESS : GetLastError();
        if(!started && error != ERROR_IO_PENDING) {
            CloseHandle(overlap.hEvent);
            continue;
        }

        const DWORD wait_result = WaitForSingleObject(overlap.hEvent, 250);
        if(wait_result != WAIT_OBJECT_0) {
            CancelIoEx(device_handle_, &overlap);
            CloseHandle(overlap.hEvent);
            continue;
        }

        DWORD bytes_read = 0;
        const bool ok = GetOverlappedResult(device_handle_, &overlap, &bytes_read, FALSE);
        CloseHandle(overlap.hEvent);
        if(!running_.load() || !ok || bytes_read == 0) {
            continue;
        }
        EmitInputReport(std::span<const std::uint8_t>(buffer.data(), bytes_read));
    }
}

bool BluetoothManager::IsDualSenseDevice(HANDLE handle) {
    HIDD_ATTRIBUTES attributes{};
    attributes.Size = sizeof(attributes);

    if(!HidD_GetAttributes(handle, &attributes)) {
        return false;
    }
    if(attributes.VendorID != shared::kSonyVendorId) {
        return false;
    }
    for(const auto pid : kSupportedDualSensePids) {
        if(attributes.ProductID == pid) {
            return true;
        }
    }
    return false;
}

} // namespace dsl::service
