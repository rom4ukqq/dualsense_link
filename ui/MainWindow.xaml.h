#pragma once

#include "ControllerDashboard.xaml.h"
#include "PipeClient.h"

#include <string_view>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.h>

namespace winrt::DualSenseLinkUI::implementation {

struct MainWindow : MainWindowT<MainWindow> {
    MainWindow();
    ~MainWindow();

    void UpdateControllerStatus(std::wstring_view connection_state, int battery_percent);
    void AppendRawInputLine(std::wstring_view raw_line);
    void OnRefreshClicked(
        winrt::Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);
    void OnLiveOnClicked(
        winrt::Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);
    void OnLiveOffClicked(
        winrt::Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

private:
    void StartPipeBridge();
    void UpdateDashboardFromStatus(const dsl::ipc::StatusPayload& payload);
    void SetUiLive(bool enabled);

    winrt::DualSenseLinkUI::ControllerDashboard dashboard_{nullptr};
    dualsense_link::ui::PipeClient pipe_client_;
};

} // namespace winrt::DualSenseLinkUI::implementation
