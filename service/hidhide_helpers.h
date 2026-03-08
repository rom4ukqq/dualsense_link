#pragma once

#include "ipc/commands.h"

#include <atomic>
#include <string>

namespace dsl::ipc {
class PipeServer;
}

namespace dsl::service {

class HidHideManager;
struct HidHideStatus;

ipc::HidHideStatusPayload BuildHidHideStatusPayload(const HidHideStatus& status, bool integration_enabled);
bool HandleHidHideConsoleCommand(
    const std::string& line,
    HidHideManager& hidhide_manager,
    ipc::PipeServer& pipe_server,
    std::atomic<bool>& hidhide_integration_enabled);

} // namespace dsl::service

