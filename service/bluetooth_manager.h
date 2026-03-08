#pragma once

#include <Windows.h>

#include <atomic>
#include <functional>
#include <jthread>
#include <mutex>
#include <span>
#include <string>
#include <vector>

namespace dsl::service {

class BluetoothManager {
public:
    using ConnectedCallback = std::function<void(const std::wstring&)>;
    using InputReportCallback = std::function<void(const std::vector<std::uint8_t>&)>;

    BluetoothManager() = default;
    ~BluetoothManager();

    bool Start();
    void Stop();
    bool IsRunning() const noexcept;

    void SetOnControllerConnected(ConnectedCallback callback);
    void SetOnInputReportReceived(InputReportCallback callback);

    bool WriteOutputReport(std::span<const std::uint8_t> report);

private:
    std::wstring FindDualSenseDevicePath() const;
    bool OpenDevice(const std::wstring& device_path);
    bool IsDualSensePath(const std::wstring& device_path) const;
    void CloseDevice();
    void ReadLoop();
    void EmitInputReport(std::span<const std::uint8_t> report);
    static bool IsDualSenseDevice(HANDLE handle);

    std::atomic<bool> running_{false};
    HANDLE device_handle_{INVALID_HANDLE_VALUE};
    std::wstring device_path_;
    std::jthread read_thread_;

    ConnectedCallback on_connected_;
    InputReportCallback on_report_;
    mutable std::mutex callback_mutex_;
};

} // namespace dsl::service
