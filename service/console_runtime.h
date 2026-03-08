#pragma once

#include "core/dualsense_parser.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>

namespace dsl::ipc {
class PipeServer;
}

namespace dsl::service {

class ControllerProxy;
class HidHideManager;
class IVirtualHidClient;

class ConsoleStatePrinter {
public:
    void SetLiveEnabled(bool enabled);
    void OnStateUpdated(const core::ParsedDualSenseState& state);
    void PrintSnapshot(const core::ParsedDualSenseState& state);

private:
    std::atomic<bool> live_enabled_{false};
    std::mutex mutex_;
    std::chrono::steady_clock::time_point last_emit_time_{};
};

void RunConsoleLoop(
    ControllerProxy& proxy,
    HidHideManager& hidhide_manager,
    const std::shared_ptr<IVirtualHidClient>& virtual_hid,
    ConsoleStatePrinter& printer,
    ipc::PipeServer& pipe_server,
    std::atomic<bool>& ui_live_enabled,
    std::atomic<bool>& hidhide_integration_enabled,
    std::atomic<bool>& shutdown_requested);

} // namespace dsl::service
