#include "service/bluetooth_manager.h"
#include "service/controller_proxy.h"

#include "core/dualsense_parser.h"
#include "ipc/commands.h"
#include "ipc/pipe_server.h"
#include "shared/common_types.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace dsl::service {

class ConsoleVirtualHidClient final : public IVirtualHidClient {
public:
    bool SendInputReport(std::span<const std::uint8_t> report) override {
        std::scoped_lock lock(mutex_);
        last_size_ = report.size();
        ++report_count_;
        return true;
    }

    void SetOutputReportHandler(OutputReportHandler handler) override {
        std::scoped_lock lock(mutex_);
        output_handler_ = std::move(handler);
    }

    void EmitTestRumbleCommand() {
        OutputReportHandler handler_copy;
        {
            std::scoped_lock lock(mutex_);
            handler_copy = output_handler_;
        }
        if(handler_copy) {
            handler_copy({0x02, 0xFF, 0x40, 0x40});
        }
    }

    std::uint64_t ReportCount() const {
        std::scoped_lock lock(mutex_);
        return report_count_;
    }

    std::size_t LastSize() const {
        std::scoped_lock lock(mutex_);
        return last_size_;
    }

private:
    mutable std::mutex mutex_;
    OutputReportHandler output_handler_;
    std::uint64_t report_count_{0};
    std::size_t last_size_{0};
};

std::string FormatStateLine(const core::ParsedDualSenseState& state) {
    std::ostringstream line;
    line << "LX:" << static_cast<int>(state.raw.left_stick_x)
         << " LY:" << static_cast<int>(state.raw.left_stick_y)
         << " RX:" << static_cast<int>(state.raw.right_stick_x)
         << " RY:" << static_cast<int>(state.raw.right_stick_y)
         << " Battery:" << static_cast<int>(state.battery_percent) << "% "
         << "Cross:" << state.cross << " Circle:" << state.circle
         << " Square:" << state.square << " Triangle:" << state.triangle;
    return line.str();
}

std::uint16_t BuildButtonsMask(const core::ParsedDualSenseState& state) {
    std::uint16_t mask = 0;
    mask |= static_cast<std::uint16_t>(state.square) << 0;
    mask |= static_cast<std::uint16_t>(state.cross) << 1;
    mask |= static_cast<std::uint16_t>(state.circle) << 2;
    mask |= static_cast<std::uint16_t>(state.triangle) << 3;
    mask |= static_cast<std::uint16_t>(state.l1) << 4;
    mask |= static_cast<std::uint16_t>(state.r1) << 5;
    mask |= static_cast<std::uint16_t>(state.l2_button) << 6;
    mask |= static_cast<std::uint16_t>(state.r2_button) << 7;
    mask |= static_cast<std::uint16_t>(state.create) << 8;
    mask |= static_cast<std::uint16_t>(state.options) << 9;
    mask |= static_cast<std::uint16_t>(state.l3) << 10;
    mask |= static_cast<std::uint16_t>(state.r3) << 11;
    mask |= static_cast<std::uint16_t>(state.ps) << 12;
    mask |= static_cast<std::uint16_t>(state.touchpad_click) << 13;
    mask |= static_cast<std::uint16_t>(state.mic) << 14;
    return mask;
}

ipc::StatusPayload BuildStatusPayload(
    const core::ParsedDualSenseState& state,
    const shared::ConnectionState connection) {
    ipc::StatusPayload payload{};
    payload.connection_state = static_cast<std::uint8_t>(connection);
    payload.battery_percent = state.battery_percent;
    payload.left_stick_x = state.raw.left_stick_x;
    payload.left_stick_y = state.raw.left_stick_y;
    payload.right_stick_x = state.raw.right_stick_x;
    payload.right_stick_y = state.raw.right_stick_y;
    payload.buttons_mask = BuildButtonsMask(state);
    payload.dpad = static_cast<std::uint8_t>(state.dpad);
    return payload;
}

class EmitGate {
public:
    explicit EmitGate(const std::chrono::milliseconds interval) : interval_(interval) {
    }

    bool TryOpen() {
        const auto now = std::chrono::steady_clock::now();
        std::scoped_lock lock(mutex_);
        if(now - last_emit_ < interval_) {
            return false;
        }
        last_emit_ = now;
        return true;
    }

private:
    std::mutex mutex_;
    std::chrono::steady_clock::time_point last_emit_{};
    std::chrono::milliseconds interval_;
};

class ConsoleStatePrinter {
public:
    void SetLiveEnabled(const bool enabled) {
        live_enabled_.store(enabled);
        std::scoped_lock lock(mutex_);
        std::cout << "Console live stream " << (enabled ? "enabled" : "disabled") << ".\n";
    }

    void OnStateUpdated(const core::ParsedDualSenseState& state) {
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

    void PrintSnapshot(const core::ParsedDualSenseState& state) {
        std::scoped_lock lock(mutex_);
        std::cout << FormatStateLine(state) << '\n';
    }

private:
    std::atomic<bool> live_enabled_{false};
    std::mutex mutex_;
    std::chrono::steady_clock::time_point last_emit_time_{};
};

void PrintStartup() {
    std::cout << "DualSense Link service started.\n";
    std::cout << "Commands: status | rumble | live on | live off | ui live on | ui live off | quit\n";
}

bool StartBridge(BluetoothManager& bluetooth, ControllerProxy& proxy) {
    if(!bluetooth.Start()) {
        std::cerr << "No DualSense Bluetooth HID device found.\n";
        return false;
    }
    proxy.Start(bluetooth);
    return true;
}

void HandleUiCommand(
    const ipc::CommandMessage& message,
    ControllerProxy& proxy,
    ipc::PipeServer& pipe_server,
    std::atomic<bool>& ui_live_enabled,
    std::atomic<shared::ConnectionState>& connection_state) {
    if(message.header.type == ipc::CommandType::UiRequestStatus) {
        const auto state = proxy.GetLastState();
        pipe_server.SendStatus(BuildStatusPayload(state, connection_state.load()));
        return;
    }

    if(message.header.type == ipc::CommandType::UiSetLiveStream &&
       message.payload.size() >= sizeof(ipc::LiveStreamPayload)) {
        ipc::LiveStreamPayload payload{};
        std::memcpy(&payload, message.payload.data(), sizeof(payload));
        ui_live_enabled.store(payload.enabled != 0);
        pipe_server.SendInfo(ui_live_enabled.load() ? "ui-live:on" : "ui-live:off");
        return;
    }

    if(message.header.type == ipc::CommandType::UiShutdownService) {
        pipe_server.SendInfo("shutdown requested via UI; use console 'quit' in this build");
    }
}

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

void RunConsoleLoop(
    ControllerProxy& proxy,
    const std::shared_ptr<ConsoleVirtualHidClient>& virtual_hid,
    ConsoleStatePrinter& printer,
    std::atomic<bool>& ui_live_enabled) {
    for(std::string line; std::getline(std::cin, line);) {
        if(line == "quit" || line == "q") {
            break;
        }
        if(line == "status") {
            const auto state = proxy.GetLastState();
            printer.PrintSnapshot(state);
            std::cout << "Virtual reports: " << virtual_hid->ReportCount()
                      << ", last size: " << virtual_hid->LastSize()
                      << ", ui live: " << (ui_live_enabled.load() ? "on" : "off") << '\n';
            continue;
        }
        if(line == "rumble") {
            virtual_hid->EmitTestRumbleCommand();
            std::cout << "Sent synthetic output report to proxy.\n";
            continue;
        }
        if(line == "live on") {
            printer.SetLiveEnabled(true);
            continue;
        }
        if(line == "live off") {
            printer.SetLiveEnabled(false);
            continue;
        }
        if(HandleUiLiveConsoleCommand(line, ui_live_enabled)) {
            continue;
        }
        std::cout << "Unknown command. Use: status | rumble | live on | live off | ui live on | ui live off | quit\n";
    }
}

void WireServiceCallbacks(
    BluetoothManager& bluetooth,
    ControllerProxy& proxy,
    ConsoleStatePrinter& printer,
    ipc::PipeServer& pipe_server,
    EmitGate& ui_raw_gate,
    std::atomic<bool>& ui_live_enabled,
    std::atomic<shared::ConnectionState>& connection_state) {
    bluetooth.SetOnControllerConnected([&](const std::wstring& path) {
        connection_state.store(shared::ConnectionState::Connected);
        std::wcout << L"Controller connected: " << path << L'\n';
        pipe_server.SendInfo("controller-connected");
    });

    pipe_server.SetOnCommand([&](const ipc::CommandMessage& message) {
        HandleUiCommand(message, proxy, pipe_server, ui_live_enabled, connection_state);
    });

    proxy.SetOnStateUpdated([&](const dsl::core::ParsedDualSenseState& state) {
        printer.OnStateUpdated(state);
        pipe_server.SendStatus(BuildStatusPayload(state, connection_state.load()));
        if(ui_live_enabled.load() && ui_raw_gate.TryOpen()) {
            pipe_server.SendRawInput(FormatStateLine(state));
        }
    });
}

void StopRuntime(ControllerProxy& proxy, BluetoothManager& bluetooth, ipc::PipeServer& pipe_server) {
    proxy.Stop();
    bluetooth.Stop();
    pipe_server.Stop();
}

} // namespace dsl::service

int main() {
    using namespace dsl::service;

    PrintStartup();

    std::atomic<dsl::shared::ConnectionState> connection_state{dsl::shared::ConnectionState::Disconnected};
    std::atomic<bool> ui_live_enabled{true};
    EmitGate ui_raw_gate(std::chrono::milliseconds(120));

    dsl::ipc::PipeServer pipe_server;
    pipe_server.Start();
    pipe_server.SendInfo("service-online");

    BluetoothManager bluetooth;
    auto parser = std::make_shared<dsl::core::DualSenseParser>();
    auto virtual_hid = std::make_shared<ConsoleVirtualHidClient>();
    ControllerProxy proxy(parser, virtual_hid);
    ConsoleStatePrinter printer;
    WireServiceCallbacks(
        bluetooth,
        proxy,
        printer,
        pipe_server,
        ui_raw_gate,
        ui_live_enabled,
        connection_state);

    if(!StartBridge(bluetooth, proxy)) {
        pipe_server.Stop();
        return 1;
    }

    RunConsoleLoop(proxy, virtual_hid, printer, ui_live_enabled);

    StopRuntime(proxy, bluetooth, pipe_server);
    std::cout << "Service stopped.\n";
    return 0;
}
