#pragma once

#include "ControllerDashboard.xaml.h"

#include <string_view>
#include <winrt/Microsoft.UI.Xaml.h>

namespace winrt::DualSenseLinkUI::implementation {

struct MainWindow : MainWindowT<MainWindow> {
    MainWindow();

    void UpdateControllerStatus(std::wstring_view connection_state, int battery_percent);
    void AppendRawInputLine(std::wstring_view raw_line);

private:
    winrt::DualSenseLinkUI::ControllerDashboard dashboard_{nullptr};
};

} // namespace winrt::DualSenseLinkUI::implementation
