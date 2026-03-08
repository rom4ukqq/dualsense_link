#pragma once

#include "core/dualsense_parser.h"
#include "service/bluetooth_manager.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

namespace dsl::service {

class IVirtualHidClient {
public:
    using OutputReportHandler = std::function<void(const std::vector<std::uint8_t>&)>;

    virtual ~IVirtualHidClient() = default;
    virtual bool SendInputReport(std::span<const std::uint8_t> report) = 0;
    virtual void SetOutputReportHandler(OutputReportHandler handler) = 0;
};

class ControllerProxy {
public:
    using StateCallback = std::function<void(const core::ParsedDualSenseState&)>;

    ControllerProxy(
        std::shared_ptr<core::DualSenseParser> parser,
        std::shared_ptr<IVirtualHidClient> virtual_hid);

    ~ControllerProxy();

    void Start(BluetoothManager& manager);
    void Stop();

    void SetOnStateUpdated(StateCallback callback);
    core::ParsedDualSenseState GetLastState() const;

private:
    void HandleBluetoothInputReport(const std::vector<std::uint8_t>& raw_packet);
    void HandleVirtualOutputReport(const std::vector<std::uint8_t>& usb_output_report);

    std::shared_ptr<core::DualSenseParser> parser_;
    std::shared_ptr<IVirtualHidClient> virtual_hid_;
    BluetoothManager* manager_{nullptr};

    mutable std::mutex state_mutex_;
    core::ParsedDualSenseState last_state_{};
    StateCallback on_state_updated_;
};

} // namespace dsl::service
