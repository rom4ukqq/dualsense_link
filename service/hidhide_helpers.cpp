#include "service/hidhide_helpers.h"

#include "ipc/pipe_server.h"
#include "service/hidhide_manager.h"

#include <iostream>

namespace dsl::service {

ipc::HidHideStatusPayload BuildHidHideStatusPayload(
    const HidHideStatus& status,
    const bool integration_enabled) {
    ipc::HidHideStatusPayload payload{};
    payload.installed = status.installed ? 1 : 0;
    payload.service_running = status.running ? 1 : 0;
    payload.integration_enabled = integration_enabled ? 1 : 0;
    payload.app_whitelisted = status.app_whitelisted ? 1 : 0;
    payload.device_hidden = status.device_hidden ? 1 : 0;
    payload.operation_ok = status.operation_ok ? 1 : 0;
    return payload;
}

bool HandleHidHideConsoleCommand(
    const std::string& line,
    HidHideManager& hidhide_manager,
    ipc::PipeServer& pipe_server,
    std::atomic<bool>& hidhide_integration_enabled) {
    if(line == "hidhide on") {
        hidhide_integration_enabled.store(true);
        hidhide_manager.SetIntegrationEnabled(true);
    } else if(line == "hidhide off") {
        hidhide_integration_enabled.store(false);
        hidhide_manager.SetIntegrationEnabled(false);
    } else if(line != "hidhide status") {
        return false;
    }

    const auto status = hidhide_manager.QueryStatus();
    pipe_server.SendHidHideStatus(
        BuildHidHideStatusPayload(status, hidhide_integration_enabled.load()));
    std::cout << "HidHide installed: " << (status.installed ? "yes" : "no")
              << ", service: " << (status.running ? "running" : "stopped")
              << ", integration: " << (hidhide_integration_enabled.load() ? "on" : "off")
              << ", app: " << (status.app_whitelisted ? "yes" : "no")
              << ", hidden: " << (status.device_hidden ? "yes" : "no")
              << ", op: " << (status.operation_ok ? "ok" : "n/a") << '\n';
    return true;
}

} // namespace dsl::service
