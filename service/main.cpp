#include "service/bluetooth_manager.h"
#include "service/controller_proxy.h"

#include "core/dualsense_parser.h"

#include <atomic>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
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

void PrintState(const core::ParsedDualSenseState& state) {
    std::cout << "LX:" << static_cast<int>(state.raw.left_stick_x)
              << " LY:" << static_cast<int>(state.raw.left_stick_y)
              << " RX:" << static_cast<int>(state.raw.right_stick_x)
              << " RY:" << static_cast<int>(state.raw.right_stick_y)
              << " Battery:" << static_cast<int>(state.battery_percent) << "% "
              << "Cross:" << state.cross << " Circle:" << state.circle
              << " Square:" << state.square << " Triangle:" << state.triangle << '\n';
}

void PrintStartup() {
    std::cout << "DualSense Link service skeleton started.\n";
    std::cout << "Commands: status | rumble | quit\n";
}

bool StartBridge(BluetoothManager& bluetooth, ControllerProxy& proxy) {
    if(!bluetooth.Start()) {
        std::cerr << "No DualSense Bluetooth HID device found.\n";
        return false;
    }
    proxy.Start(bluetooth);
    return true;
}

void RunConsoleLoop(ControllerProxy& proxy, const std::shared_ptr<ConsoleVirtualHidClient>& virtual_hid) {
    for(std::string line; std::getline(std::cin, line);) {
        if(line == "quit" || line == "q") {
            break;
        }
        if(line == "status") {
            const auto state = proxy.GetLastState();
            PrintState(state);
            std::cout << "Virtual reports: " << virtual_hid->ReportCount()
                      << ", last size: " << virtual_hid->LastSize() << '\n';
        }
        if(line == "rumble") {
            virtual_hid->EmitTestRumbleCommand();
            std::cout << "Sent synthetic output report to proxy.\n";
        }
    }
}

} // namespace dsl::service

int main() {
    using namespace dsl::service;

    PrintStartup();

    BluetoothManager bluetooth;
    bluetooth.SetOnControllerConnected([](const std::wstring& path) {
        std::wcout << L"Controller connected: " << path << L'\n';
    });

    auto parser = std::make_shared<dsl::core::DualSenseParser>();
    auto virtual_hid = std::make_shared<ConsoleVirtualHidClient>();
    ControllerProxy proxy(parser, virtual_hid);
    proxy.SetOnStateUpdated([](const dsl::core::ParsedDualSenseState& state) {
        PrintState(state);
    });

    if(!StartBridge(bluetooth, proxy)) {
        return 1;
    }
    RunConsoleLoop(proxy, virtual_hid);

    proxy.Stop();
    bluetooth.Stop();
    std::cout << "Service stopped.\n";
    return 0;
}
