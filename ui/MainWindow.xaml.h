#pragma once

#include "MainWindow.xaml.g.h"
#include "ControllerDashboard.xaml.h"
#include "PipeClient.h"
#include "service_process.h"

#include <string>
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
    void OnHidHideOnClicked(
        winrt::Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);
    void OnHidHideOffClicked(
        winrt::Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

private:
    void StartPipeBridge();
    void UpdateDashboardFromStatus(const dsl::ipc::StatusPayload& payload);
    void UpdateBridgeStatus(const dsl::ipc::BridgeStatusPayload& payload);
    void UpdateServiceText();
    void SetUiLive(bool enabled);
    void RequestRefresh();

    winrt::DualSenseLinkUI::ControllerDashboard dashboard_{nullptr};
    dualsense_link::ui::PipeClient pipe_client_;
    dualsense_link::ui::ServiceProcess service_process_;
    std::wstring service_state_text_{L"Service: checking..."};
    dsl::ipc::BridgeMode bridge_mode_{dsl::ipc::BridgeMode::Unknown};
};

} // namespace winrt::DualSenseLinkUI::implementation

namespace winrt::DualSenseLinkUI::factory_implementation {

struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
};

} // namespace winrt::DualSenseLinkUI::factory_implementation
