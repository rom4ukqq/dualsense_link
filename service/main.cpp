#include "service/bluetooth_manager.h"
#include "service/console_runtime.h"
#include "service/console_virtual_hid_client.h"
#include "service/controller_proxy.h"
#include "service/driver_bridge_client.h"
#include "service/hidhide_helpers.h"
#include "service/hidhide_manager.h"
#include "service/state_format.h"
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
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace dsl::service {

enum class ServiceMode {
    Background,
    Console
};

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

bool HasArg(const std::vector<std::string>& args, const std::string_view flag) {
    for(const auto& arg : args) {
        if(arg == flag) { return true; }
    }
    return false;
}

std::vector<std::string> CollectArgs(const int argc, char** argv) {
    std::vector<std::string> args;
    args.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0);
    for(int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    return args;
}

ServiceMode ResolveServiceMode(const std::vector<std::string>& args) {
    if(HasArg(args, "--console")) {
        return ServiceMode::Console;
    }
    return ServiceMode::Background;
}

void PrintStartup(const ServiceMode mode) {
    const bool background_mode = mode == ServiceMode::Background;
    std::cout << "DualSense Link service started (" << (background_mode ? "background" : "console")
              << " mode).\n";
    if(background_mode) {
        std::cout << "Waiting for UI commands.\n";
        return;
    }
    std::cout << "Commands: status | rumble | live on | live off | ui live on | ui live off | "
                 "hidhide status | hidhide on | hidhide off | quit\n";
}

bool StartBridge(BluetoothManager& bluetooth, ControllerProxy& proxy) {
    if(!bluetooth.Start()) {
        std::cerr << "No DualSense Bluetooth HID device found.\n";
        return false;
    }
    proxy.Start(bluetooth);
    return true;
}

void HandleUiStatusCommand(
    const ipc::CommandMessage& message,
    ControllerProxy& proxy,
    std::atomic<shared::ConnectionState>& connection_state,
    std::atomic<ipc::BridgeMode>& bridge_mode,
    ipc::PipeServer& pipe_server) {
    if(message.header.type != ipc::CommandType::UiRequestStatus) {
        return;
    }
    const auto state = proxy.GetLastState();
    ipc::BridgeStatusPayload bridge{};
    bridge.mode = bridge_mode.load();
    bridge.connected = bridge.mode == ipc::BridgeMode::DriverBridge ? 1 : 0;
    pipe_server.SendStatus(BuildStatusPayload(state, connection_state.load()));
    pipe_server.SendBridgeStatus(bridge);
}

void HandleUiLiveCommand(
    const ipc::CommandMessage& message,
    ipc::PipeServer& pipe_server,
    std::atomic<bool>& ui_live_enabled) {
    if(message.header.type != ipc::CommandType::UiSetLiveStream ||
       message.payload.size() < sizeof(ipc::LiveStreamPayload)) {
        return;
    }
    ipc::LiveStreamPayload payload{};
    std::memcpy(&payload, message.payload.data(), sizeof(payload));
    ui_live_enabled.store(payload.enabled != 0);
    pipe_server.SendInfo(ui_live_enabled.load() ? "ui-live:on" : "ui-live:off");
}

void HandleUiHidHideCommand(
    const ipc::CommandMessage& message,
    HidHideManager& hidhide_manager,
    ipc::PipeServer& pipe_server,
    std::atomic<bool>& hidhide_integration_enabled) {
    if(message.header.type == ipc::CommandType::UiRequestHidHideStatus) {
        const auto status = hidhide_manager.QueryStatus();
        pipe_server.SendHidHideStatus(
            BuildHidHideStatusPayload(status, hidhide_integration_enabled.load()));
        return;
    }
    if(message.header.type != ipc::CommandType::UiSetHidHideEnabled ||
       message.payload.size() < sizeof(ipc::HidHideTogglePayload)) {
        return;
    }
    ipc::HidHideTogglePayload payload{};
    std::memcpy(&payload, message.payload.data(), sizeof(payload));
    hidhide_integration_enabled.store(payload.enabled != 0);
    hidhide_manager.SetIntegrationEnabled(payload.enabled != 0);
    const auto status = hidhide_manager.QueryStatus();
    pipe_server.SendHidHideStatus(BuildHidHideStatusPayload(status, hidhide_integration_enabled.load()));
    pipe_server.SendInfo(hidhide_integration_enabled.load() ? "hidhide:on" : "hidhide:off");
}

void HandleUiShutdownCommand(
    const ipc::CommandMessage& message,
    ipc::PipeServer& pipe_server,
    std::atomic<bool>& shutdown_requested) {
    if(message.header.type != ipc::CommandType::UiShutdownService) {
        return;
    }
    shutdown_requested.store(true);
    pipe_server.SendInfo("shutdown requested via UI");
}

void HandleUiCommand(
    const ipc::CommandMessage& message,
    ControllerProxy& proxy,
    HidHideManager& hidhide_manager,
    ipc::PipeServer& pipe_server,
    std::atomic<bool>& ui_live_enabled,
    std::atomic<bool>& hidhide_integration_enabled,
    std::atomic<bool>& shutdown_requested,
    std::atomic<ipc::BridgeMode>& bridge_mode,
    std::atomic<shared::ConnectionState>& connection_state) {
    HandleUiStatusCommand(message, proxy, connection_state, bridge_mode, pipe_server);
    HandleUiLiveCommand(message, pipe_server, ui_live_enabled);
    HandleUiHidHideCommand(message, hidhide_manager, pipe_server, hidhide_integration_enabled);
    HandleUiShutdownCommand(message, pipe_server, shutdown_requested);
}

void RunBackgroundLoop(std::atomic<bool>& shutdown_requested) {
    while(!shutdown_requested.load()) { std::this_thread::sleep_for(std::chrono::milliseconds(120)); }
}

void WireServiceCallbacks(
    BluetoothManager& bluetooth,
    ControllerProxy& proxy,
    HidHideManager& hidhide_manager,
    ConsoleStatePrinter& printer,
    ipc::PipeServer& pipe_server,
    EmitGate& ui_raw_gate,
    std::atomic<bool>& ui_live_enabled,
    std::atomic<bool>& hidhide_integration_enabled,
    std::atomic<bool>& shutdown_requested,
    std::atomic<ipc::BridgeMode>& bridge_mode,
    std::atomic<shared::ConnectionState>& connection_state) {
    bluetooth.SetOnControllerConnected([&](const std::wstring& path) {
        connection_state.store(shared::ConnectionState::Connected);
        hidhide_manager.SetControllerInterfacePath(path);
        if(hidhide_integration_enabled.load()) {
            hidhide_manager.SetIntegrationEnabled(true);
        }
        std::wcout << L"Controller connected: " << path << L'\n';
        pipe_server.SendInfo("controller-connected");
    });

    pipe_server.SetOnCommand([&](const ipc::CommandMessage& message) {
        HandleUiCommand(
            message,
            proxy,
            hidhide_manager,
            pipe_server,
            ui_live_enabled,
            hidhide_integration_enabled,
            shutdown_requested,
            bridge_mode,
            connection_state);
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

int main(int argc, char** argv) {
    using namespace dsl::service;

    const auto args = CollectArgs(argc, argv);
    const auto mode = ResolveServiceMode(args);
    const bool background_mode = mode == ServiceMode::Background;
    const bool allow_fallback = mode == ServiceMode::Console || HasArg(args, "--allow-fallback");
    PrintStartup(mode);

    std::atomic<dsl::shared::ConnectionState> connection_state{dsl::shared::ConnectionState::Disconnected};
    std::atomic<bool> ui_live_enabled{true};
    std::atomic<bool> hidhide_integration_enabled{false};
    std::atomic<bool> shutdown_requested{false};
    std::atomic<dsl::ipc::BridgeMode> bridge_mode{dsl::ipc::BridgeMode::Unknown};
    EmitGate ui_raw_gate(std::chrono::milliseconds(120));

    dsl::ipc::PipeServer pipe_server;
    pipe_server.Start();
    pipe_server.SendInfo("service-online");

    BluetoothManager bluetooth;
    HidHideManager hidhide_manager;
    auto parser = std::make_shared<dsl::core::DualSenseParser>();
    auto driver_hid = std::make_shared<DriverBridgeClient>();
    std::shared_ptr<IVirtualHidClient> virtual_hid;
    if(driver_hid->Start()) {
        virtual_hid = driver_hid;
        bridge_mode.store(dsl::ipc::BridgeMode::DriverBridge);
        std::cout << "Virtual HID bridge: driver device connected.\n";
        dsl::ipc::BridgeStatusPayload payload{};
        payload.mode = dsl::ipc::BridgeMode::DriverBridge;
        payload.connected = 1;
        pipe_server.SendBridgeStatus(payload);
        pipe_server.SendInfo("virtual-hid:driver-bridge");
    } else {
        if(!allow_fallback) {
            std::cerr << "Driver bridge unavailable. Install/start driver or use --console --allow-fallback.\n";
            pipe_server.SendInfo("virtual-hid:error-driver-unavailable");
            pipe_server.Stop();
            return 2;
        }
        virtual_hid = std::make_shared<ConsoleVirtualHidClient>();
        bridge_mode.store(dsl::ipc::BridgeMode::ConsoleFallback);
        std::cout << "Virtual HID bridge: driver unavailable, using console fallback.\n";
        dsl::ipc::BridgeStatusPayload payload{};
        payload.mode = dsl::ipc::BridgeMode::ConsoleFallback;
        payload.connected = 0;
        pipe_server.SendBridgeStatus(payload);
        pipe_server.SendInfo("virtual-hid:console-fallback");
    }
    ControllerProxy proxy(parser, virtual_hid);
    ConsoleStatePrinter printer;
    WireServiceCallbacks(
        bluetooth,
        proxy,
        hidhide_manager,
        printer,
        pipe_server,
        ui_raw_gate,
        ui_live_enabled,
        hidhide_integration_enabled,
        shutdown_requested,
        bridge_mode,
        connection_state);
    HandleHidHideConsoleCommand(
        "hidhide status",
        hidhide_manager,
        pipe_server,
        hidhide_integration_enabled);

    if(!StartBridge(bluetooth, proxy)) {
        pipe_server.Stop();
        return 1;
    }

    if(mode == ServiceMode::Background) {
        RunBackgroundLoop(shutdown_requested);
    } else {
        RunConsoleLoop(
            proxy,
            hidhide_manager,
            virtual_hid,
            printer,
            pipe_server,
            ui_live_enabled,
            hidhide_integration_enabled,
            shutdown_requested);
    }

    StopRuntime(proxy, bluetooth, pipe_server);
    driver_hid->Stop();
    std::cout << "Service stopped.\n";
    return 0;
}
