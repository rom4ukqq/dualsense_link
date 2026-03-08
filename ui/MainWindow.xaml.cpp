#include "MainWindow.xaml.h"

#include <string>
#include <utility>
#include <winrt/base.h>

namespace winrt::DualSenseLinkUI::implementation {

namespace {

std::wstring ConnectionStateText(const std::uint8_t connection_state) {
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

} // namespace

MainWindow::MainWindow() {
    InitializeComponent();
    dashboard_ = Dashboard();
    StartPipeBridge();
}

MainWindow::~MainWindow() {
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
    pipe_client_.RequestStatus();
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

void MainWindow::StartPipeBridge() {
    pipe_client_.SetOnStatus([this](const dsl::ipc::StatusPayload& payload) {
        DispatcherQueue().TryEnqueue([this, payload]() {
            UpdateControllerStatus(ConnectionStateText(payload.connection_state), payload.battery_percent);
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

    pipe_client_.Start();
    pipe_client_.RequestStatus();
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

void MainWindow::SetUiLive(const bool enabled) {
    pipe_client_.SetLiveStreamEnabled(enabled);
    AppendRawInputLine(enabled ? L"[info] UI live stream ON" : L"[info] UI live stream OFF");
}

} // namespace winrt::DualSenseLinkUI::implementation
