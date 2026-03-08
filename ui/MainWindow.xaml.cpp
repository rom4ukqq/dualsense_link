#include "MainWindow.xaml.h"

#include "ui_formatting.h"

#include <cstdint>
#include <string>
#include <utility>
#include <winrt/base.h>

namespace winrt::DualSenseLinkUI::implementation {

namespace {

std::wstring ConnectionLabel(const std::uint8_t connection_state) {
    if(connection_state == 2) {
        return L"Connected";
    }
    if(connection_state == 1) {
        return L"Connecting";
    }
    return L"Disconnected";
}

std::wstring DpadText(const std::uint8_t dpad) {
    switch(dpad) {
        case 0: return L"Up";
        case 1: return L"UpRight";
        case 2: return L"Right";
        case 3: return L"DownRight";
        case 4: return L"Down";
        case 5: return L"DownLeft";
        case 6: return L"Left";
        case 7: return L"UpLeft";
        default: return L"Centered";
    }
}

std::wstring FaceButtonsText(const std::uint16_t mask) {
    std::wstring text;
    if(mask & (1u << 0)) { text += L"Square "; }
    if(mask & (1u << 1)) { text += L"Cross "; }
    if(mask & (1u << 2)) { text += L"Circle "; }
    if(mask & (1u << 3)) { text += L"Triangle "; }
    if(text.empty()) {
        return L"None";
    }
    text.pop_back();
    return text;
}

std::wstring BridgeText(const dsl::ipc::BridgeMode mode) {
    if(mode == dsl::ipc::BridgeMode::DriverBridge) {
        return L"driver-bridge";
    }
    if(mode == dsl::ipc::BridgeMode::ConsoleFallback) {
        return L"fallback";
    }
    return L"unknown";
}

} // namespace

MainWindow::MainWindow() {
    InitializeComponent();
    dashboard_ = Dashboard();

    if(service_process_.EnsureRunning()) {
        service_state_text_ = service_process_.StatusText();
    } else {
        service_state_text_ = L"Service: start failed";
        AppendRawInputLine(L"[error] unable to start dsl_service --background");
    }

    UpdateServiceText();
    StartPipeBridge();
    RequestRefresh();
}

MainWindow::~MainWindow() {
    service_process_.ShutdownIfLaunched([this]() {
        return pipe_client_.RequestServiceShutdown();
    });
    pipe_client_.Stop();
}

void MainWindow::UpdateControllerStatus(const std::wstring_view connection_state, const int battery_percent) {
    ConnectionStateText().Text(winrt::hstring(connection_state));
    BatteryText().Text(L"Battery: " + std::to_wstring(battery_percent) + L"%");
}

void MainWindow::AppendRawInputLine(const std::wstring_view raw_line) {
    auto current = RawInputTextBox().Text();
    if(current.size() > 12000) {
        current = current.substr(current.size() - 6000);
    }
    current = current + winrt::hstring(raw_line) + L"\n";
    RawInputTextBox().Text(current);
}

void MainWindow::OnRefreshClicked(
    winrt::Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    service_process_.EnsureRunning();
    service_state_text_ = service_process_.StatusText();
    UpdateServiceText();
    RequestRefresh();
}

void MainWindow::OnLiveOnClicked(
    winrt::Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    SetUiLive(true);
}

void MainWindow::OnLiveOffClicked(
    winrt::Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    SetUiLive(false);
}

void MainWindow::OnHidHideOnClicked(
    winrt::Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    pipe_client_.SetHidHideEnabled(true);
    pipe_client_.RequestHidHideStatus();
}

void MainWindow::OnHidHideOffClicked(
    winrt::Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    pipe_client_.SetHidHideEnabled(false);
    pipe_client_.RequestHidHideStatus();
}

void MainWindow::StartPipeBridge() {
    pipe_client_.SetOnStatus([this](const dsl::ipc::StatusPayload& payload) {
        DispatcherQueue().TryEnqueue([this, payload]() {
            UpdateControllerStatus(ConnectionLabel(payload.connection_state), payload.battery_percent);
            UpdateDashboardFromStatus(payload);
        });
    });

    pipe_client_.SetOnRawInput([this](const std::string_view line) {
        std::string text(line);
        DispatcherQueue().TryEnqueue([this, text = std::move(text)]() {
            AppendRawInputLine(winrt::to_hstring(text));
        });
    });

    pipe_client_.SetOnInfo([this](const std::string_view line) {
        std::string text = "[info] ";
        text.append(line.data(), line.size());
        DispatcherQueue().TryEnqueue([this, text = std::move(text)]() {
            AppendRawInputLine(winrt::to_hstring(text));
        });
    });

    pipe_client_.SetOnHidHideStatus([this](const dsl::ipc::HidHideStatusPayload& payload) {
        DispatcherQueue().TryEnqueue([this, payload]() {
            HidHideStateText().Text(winrt::hstring(dualsense_link::ui::HidHideLabelText(payload)));
            AppendRawInputLine(winrt::to_hstring(dualsense_link::ui::HidHideStatusLine(payload)));
        });
    });

    pipe_client_.SetOnBridgeStatus([this](const dsl::ipc::BridgeStatusPayload& payload) {
        DispatcherQueue().TryEnqueue([this, payload]() {
            UpdateBridgeStatus(payload);
        });
    });

    pipe_client_.Start();
}

void MainWindow::UpdateDashboardFromStatus(const dsl::ipc::StatusPayload& payload) {
    dashboard_.SetDpadText(DpadText(payload.dpad));
    dashboard_.SetFaceButtonsText(FaceButtonsText(payload.buttons_mask));

    const auto sticks = L"LX " + std::to_wstring(payload.left_stick_x) +
                        L" LY " + std::to_wstring(payload.left_stick_y) +
                        L" RX " + std::to_wstring(payload.right_stick_x) +
                        L" RY " + std::to_wstring(payload.right_stick_y);
    dashboard_.SetSticksText(sticks);
}

void MainWindow::UpdateBridgeStatus(const dsl::ipc::BridgeStatusPayload& payload) {
    bridge_mode_ = payload.mode;
    UpdateServiceText();
}

void MainWindow::UpdateServiceText() {
    const std::wstring text = service_state_text_ + L" | Bridge: " + BridgeText(bridge_mode_);
    ServiceBridgeText().Text(winrt::hstring(text));
}

void MainWindow::SetUiLive(const bool enabled) {
    pipe_client_.SetLiveStreamEnabled(enabled);
    AppendRawInputLine(enabled ? L"[info] UI live stream ON" : L"[info] UI live stream OFF");
}

void MainWindow::RequestRefresh() {
    pipe_client_.RequestStatus();
    pipe_client_.RequestHidHideStatus();
}

} // namespace winrt::DualSenseLinkUI::implementation
