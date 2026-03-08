#include "service/console_runtime.h"

#include "ipc/pipe_server.h"
#include "service/console_virtual_hid_client.h"
#include "service/controller_proxy.h"
#include "service/driver_bridge_client.h"
#include "service/hidhide_helpers.h"
#include "service/hidhide_manager.h"
#include "service/state_format.h"

#include <iostream>

namespace dsl::service {

void ConsoleStatePrinter::SetLiveEnabled(const bool enabled) {
    live_enabled_.store(enabled);
    std::scoped_lock lock(mutex_);
    std::cout << "Console live stream " << (enabled ? "enabled" : "disabled") << ".\n";
}

void ConsoleStatePrinter::OnStateUpdated(const core::ParsedDualSenseState& state) {
    if(!live_enabled_.load()) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    std::scoped_lock lock(mutex_);
    if(now - last_emit_time_ < std::chrono::milliseconds(200)) {
        return;
    }
    last_emit_time_ = now;
    std::cout << FormatStateLine(state) << '\n';
}

void ConsoleStatePrinter::PrintSnapshot(const core::ParsedDualSenseState& state) {
    std::scoped_lock lock(mutex_);
    std::cout << FormatStateLine(state) << '\n';
}

namespace {

bool HandleUiLiveConsoleCommand(const std::string& line, std::atomic<bool>& ui_live_enabled) {
    if(line == "ui live on") {
        ui_live_enabled.store(true);
        std::cout << "UI live stream enabled.\n";
        return true;
    }
    if(line == "ui live off") {
        ui_live_enabled.store(false);
        std::cout << "UI live stream disabled.\n";
        return true;
    }
    return false;
}

void PrintStatusLine(
    ControllerProxy& proxy,
    const std::shared_ptr<IVirtualHidClient>& virtual_hid,
    ConsoleStatePrinter& printer,
    const std::atomic<bool>& ui_live_enabled,
    const std::atomic<bool>& hidhide_integration_enabled) {
    printer.PrintSnapshot(proxy.GetLastState());
    if(const auto* console_hid = dynamic_cast<const ConsoleVirtualHidClient*>(virtual_hid.get())) {
        std::cout << "Virtual reports: " << console_hid->ReportCount() << ", last size: " << console_hid->LastSize()
                  << ", ui live: " << (ui_live_enabled.load() ? "on" : "off")
                  << ", hidhide: " << (hidhide_integration_enabled.load() ? "on" : "off") << '\n';
        return;
    }
    std::cout << "Virtual HID: driver-bridge, ui live: " << (ui_live_enabled.load() ? "on" : "off")
              << ", hidhide: " << (hidhide_integration_enabled.load() ? "on" : "off") << '\n';
}

void HandleRumble(const std::shared_ptr<IVirtualHidClient>& virtual_hid) {
    auto* console_hid = dynamic_cast<ConsoleVirtualHidClient*>(virtual_hid.get());
    if(console_hid != nullptr) {
        console_hid->EmitTestRumbleCommand();
        std::cout << "Sent synthetic output report to proxy.\n";
        return;
    }

    auto* driver_hid = dynamic_cast<DriverBridgeClient*>(virtual_hid.get());
    if(driver_hid != nullptr && driver_hid->EmitTestOutputReport()) {
        std::cout << "Injected test output report through driver bridge.\n";
    } else {
        std::cout << "Unable to inject test output report via driver bridge.\n";
    }
}

} // namespace

void RunConsoleLoop(
    ControllerProxy& proxy,
    HidHideManager& hidhide_manager,
    const std::shared_ptr<IVirtualHidClient>& virtual_hid,
    ConsoleStatePrinter& printer,
    ipc::PipeServer& pipe_server,
    std::atomic<bool>& ui_live_enabled,
    std::atomic<bool>& hidhide_integration_enabled,
    std::atomic<bool>& shutdown_requested) {
    const auto print_usage = []() {
        std::cout << "Unknown command. Use: status | rumble | live on | live off | ui live on | ui live off | "
                     "hidhide status | hidhide on | hidhide off | quit\n";
    };

    for(std::string line; !shutdown_requested.load() && std::getline(std::cin, line);) {
        if(line == "quit" || line == "q") { break; }
        if(line == "status") {
            PrintStatusLine(proxy, virtual_hid, printer, ui_live_enabled, hidhide_integration_enabled);
            continue;
        }
        if(line == "rumble") {
            HandleRumble(virtual_hid);
            continue;
        }
        if(line == "live on") { printer.SetLiveEnabled(true); continue; }
        if(line == "live off") { printer.SetLiveEnabled(false); continue; }
        if(HandleUiLiveConsoleCommand(line, ui_live_enabled)) { continue; }
        if(HandleHidHideConsoleCommand(line, hidhide_manager, pipe_server, hidhide_integration_enabled)) { continue; }
        print_usage();
    }
}

} // namespace dsl::service
