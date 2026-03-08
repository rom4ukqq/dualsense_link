#pragma once

#include "service/controller_proxy.h"

#include <Windows.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace dsl::service {

class DriverBridgeClient final : public IVirtualHidClient {
public:
    DriverBridgeClient() = default;
    ~DriverBridgeClient() override;

    bool Start();
    void Stop();
    [[nodiscard]] bool IsConnected() const;

    bool SendInputReport(std::span<const std::uint8_t> report) override;
    void SetOutputReportHandler(OutputReportHandler handler) override;
    bool EmitTestOutputReport();

private:
    bool OpenDevice();
    bool ValidateInterfaceVersion();
    bool ReadOutputReport(HANDLE device, std::vector<std::uint8_t>* report);
    void ReadOutputLoop();
    [[nodiscard]] HANDLE CopyDeviceHandle() const;
    [[nodiscard]] OutputReportHandler CopyOutputHandler() const;
    void CloseDevice();

    std::atomic<bool> running_{false};
    std::jthread output_thread_;
    HANDLE device_{INVALID_HANDLE_VALUE};
    mutable std::mutex device_mutex_;
    mutable std::mutex callback_mutex_;
    OutputReportHandler output_handler_;
};

} // namespace dsl::service
